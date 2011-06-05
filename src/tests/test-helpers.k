;;;
;;; Some helpers used in many modules
;;; (check.k should be loaded)
;;;

($define! not-eq? ($lambda (x y) (not? (eq? x y))))
($define! not-equal? ($lambda (x y) (not? (equal? x y))))
($define! $check-predicate ($vau (x) denv (eval (list $check eq? x #t) denv)))
($define! $check-not-predicate ($vau (x) denv (eval (list $check eq? x #f) denv)))

($define! mutable-pair?
  ($lambda (obj)
    ($and? (pair? obj)
	   (guard-dynamic-extent
	    ()
	    ($lambda ()
	      (set-car! obj (car obj))
	      #t)
	    ;; As per the report (section 4.7.1) setting the car of an 
	    ;; immutable pair (even if the value is the same) should 
	    ;; signal an error.
	    (list (list error-continuation
			($lambda (#ignore divert)
			  (apply divert #f))))))))

($define! immutable-pair?
  ($lambda (obj) ($and? (pair? obj) (not? (mutable-pair? obj)))))