;;;
;;; Some helpers used in many modules
;;; (check.k should be loaded)
;;;

($define! not-eq? ($lambda (x y) (not? (eq? x y))))
($define! not-equal? ($lambda (x y) (not? (equal? x y))))
($define! $check-predicate ($vau (x) denv (eval (list $check eq? x #t) denv)))
($define! $check-not-predicate ($vau (x) denv (eval (list $check eq? x #f) denv)))
($define! $check-no-error ($vau (x) denv 
			    (eval (list $check 
					($lambda (#ignore #ignore) #t)
					x 
					#inert) 
				  denv)))

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

($define! nonempty-mutable-string?
  ($lambda (obj)
    ($and?
      (string? obj)
      (>? (string-length obj) 0)
      (guard-dynamic-extent
        ()
        ($lambda () (string-fill! obj #\x) #t)
        (list
          (list error-continuation
            ($lambda (#ignore divert) (apply divert #f))))))))

($define! immutable-string?
  ($lambda (obj) ($and? (string? obj) (not? (nonempty-mutable-string? obj)))))

;; XXX/TODO Some of these could be removed if we had eager comprehension in
;; check.k (which would also complete the srfi-78 implementation). The problem
;; is that the design adaptation from macros to operatives should be taken
;; with some care. I intended to do it before the remaining tests, but since
;; Oto Havle went ahead and wrote the tests without it, they have lost some
;; priority. Andres Navarro 

;; ($false-for-all? P XS) evaluates to #t iff (P X) evaluates
;; to #f for all members X of the list XS.
;;
($define! $false-for-all?
  ($vau (p . xs) denv
    (apply and?
      (map ($lambda (x) (not? (eval (list p x) denv))) xs))))

;; (cartesian-product XS YS) returns list of all pairs (X Y),
;; where X is a member of the list XS and Y is a member of list YS.
;;
;; for example
;;  (cartesian-product (1 2) (3 4)) ===> ((1 3) (1 4) (2 3) (2 4))
;;
($define! cartesian-product
  ($lambda (xs ys)
    (apply append
      (map ($lambda (x) (map ($lambda (y) (list x y)) ys)) xs))))

;; ($true-for-all-combinations? BIN (X1 X2...) (Y1 Y1...))
;; evaluates to #t, iff (BIN X Y) evaluates to #t for all X and Y.
;;
($define! $true-for-all-combinations?
  ($vau (p xs ys) denv
    (apply and?
      (map ($lambda ((x y)) (eval (list p x y) denv))
        (cartesian-product xs ys)))))

;; ($false-for-all-combinations? BIN (X1 X2...) (Y1 Y2...))
;; evaluates to #t, iff (BIN X Y) evaluates to #f for all X and Y.
;;
($define! $false-for-all-combinations?
  ($vau (p xs ys) denv
    (apply and?
      (map ($lambda ((x y)) (not? (eval (list p x y) denv)))
        (cartesian-product xs ys)))))

;; ($quote V) evaluates to V. The value V itself is not evaluated.
;; See section 5.5.1, page 67 of the Kernel Report.
($define! $quote ($vau (x) #ignore x))
