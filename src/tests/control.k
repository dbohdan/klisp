;; check.k & test-helpers.k should be loaded

;;;
;;; Basic Functionality
;;;

;; inert?
($check-predicate (applicative? inert?))
($check-predicate (inert?))
($check-predicate (inert? #inert))
($check-predicate (inert? #inert #inert #inert))
($check-predicate (inert? #inert . #0=(#inert . #0#)))

($check-not-predicate (inert? ((unwrap list) . symbol)))
($check-not-predicate (inert? ()))
($check-not-predicate (inert? (cons () ())))
($check-not-predicate (inert? #ignore))
($check-not-predicate (inert? (make-environment)))
($check-not-predicate (inert? #t))
($check-not-predicate (inert? #f))
($check-not-predicate (inert? $vau))
($check-not-predicate (inert? wrap))
($check-not-predicate (inert? (call/cc ($lambda (c) c))))
($check-not-predicate (inert? ($let (((enc . #ignore) 
					(make-encapsulation-type)))
				  (enc #inert))))
($check-not-predicate (inert? (memoize #inert)))
($check-not-predicate (inert? 1))
;($check-not-predicate (inert? 1.0))
($check-not-predicate (inert? #e+infinity))
;($check-not-predicate (inert? #i+infinity))
;($check-not-predicate (inert? #undefined))
;($check-not-predicate (inert? #real-with-no-primary-value))
($check-not-predicate (inert? "string"))
($check-not-predicate (inert? #\a))
($check-not-predicate (inert? (get-current-input-port)))

;; basic eq?-ness
($check eq? #inert #inert)

;; basic equal?-ness and not?
($check equal? #inert #inert)

;; $if
($check-predicate (operative? $if))
($check eq? ($if #t #t #f) #t)
($check eq? ($if #f #t #f) #f)
($check eq? ($if #t (get-current-environment) #f) (get-current-environment))
($check eq? ($if #f #t (get-current-environment)) (get-current-environment))
($let ((p (cons () ())))
  ($check eq? ($if (($vau #ignore env
		      (set-car! p env)
		      #t)) (car p) #f) 
	  (get-current-environment)))

;; $sequence
($check-predicate (operative? $sequence))
($check eq? ($sequence) #inert)
($check eq? ($sequence 1) 1)
($check eq? ($sequence 1 2 3) 3)
($check eq? ($sequence (get-current-environment)) (get-current-environment))
($check eq? ($sequence #inert #inert (get-current-environment)) 
	(get-current-environment))

($let ((p (cons 0 ())))
  ($check eq? 
	  ($let/cc cont
	    ($sequence . #0=(($if (=? (car p) 3)
				  (apply-continuation cont #t)
				  (set-car! p (+ (car p) 1)))
			     . #0#)))
	  #t))

;; $cond
($check-predicate (operative? $cond))
($check eq? ($cond) #inert)
($check eq? ($cond (#f 1) (#f 2) (#f 3)) #inert)
($check eq? ($cond (#t 1) (#t 2) (#t 3)) 1)

($check eq? ($cond (#t (get-current-environment))) (get-current-environment))
($let ((p (cons () ())))
  ($check eq? 
	  ($cond (#f)
		 (($sequence (set-car! p (get-current-environment))
			     #t)
		  (car p))
		 (#f))
	  (get-current-environment)))
($check eq? ($cond . #0=((#f) (#t 1) . #0#)) 1)
($let ((p (cons 0 ())))
  ($check eq? 
	  ($cond . #0=(((=? (car p) 3) 3)
		       (($sequence (set-car! p (+ (car p) 1))
				   #f)
			0)
		       (#f)
		       . #0#))
	  3))


;; for-each
($check-predicate (applicative? for-each))
($check eq? (for-each + (list 1 2 3 4)) #inert)
($check eq? (for-each cons (list 1 2 3 4) (list 10 20 30 40)) #inert)
($let ((p (cons () ())))
  ($check eq?
	  ($sequence (for-each (wrap ($vau #ignore env
				       (set-car! p env)))
			       (list 1))
		     (car p))
	  (get-current-environment)))
($let ((p (cons 0 ())))
  ($check eq?
	  ($sequence (for-each ($lambda (x)
				 (set-car! p (+ (car p) x)))
			       (list 1 2 3 4))
		     (car p))
	  10))
($let ((p (cons 0 ())))
  ($check eq?
	  ($sequence (for-each ($lambda (x y )
				 (set-car! p (+ (car p) x y)))
			       (list 1 2 3 4)
			       (list 10 20 30 40))
		     (car p))
	  110))

($let ((p (cons 0 ())))
  ($check eq?
	  ($let/cc cont
	    (for-each ($lambda (x)
			($if (=? (car p) 10)
			     (apply-continuation cont 10)
			     (set-car! p (+ (car p) 1))))
		      (list 1 . #0=(2 3 4 . #0#))))
	  #inert))

($let ((p (cons 0 ())))
  ($check eq?
	  ($sequence (for-each ($lambda ls
				 (set-car! p (finite-list? ls)))
		     . #0=((list 1 2 3 4)
			   (list 10 20 30 40)
			   . #0#))
		     (car p))
	  #f))


;;;
;;; Error Checking and Robustness
;;;

;; inert?
($check-error (inert? #inert . #inert))
($check-error (inert? #t . #inert))

;; $if
($check-error ($if))
($check-error ($if #t))

;; this short form isn't allowed in Kernel
($check-error ($if #f #t))
($check-error ($if #t #t))

($check-error ($if #t #t #t #t))
($check-error ($if . #0=(#t . #0#)))

($check-error ($if 0 #t #f))
($check-error ($if () #t #f))
($check-error ($if #inert #t #f))
($check-error ($if #ignore #t #f))
($check-error ($if (cons #t #f) #t #f))
($check-error ($if (cons #t #f) #t #f))

;; $sequence
($check-error ($sequence . #inert))
($check-error ($sequence #inert #inert . #inert))

;; $cond
($check-error ($cond . #inert))
($check-error ($cond (#t #t) . #inert))
($check-error ($cond #inert))
($check-error ($cond (1 1) (#t #t)))

;; for-each

($check-error (for-each))
;; the list can't be empty
($check-error (for-each list))

($check-error (for-each list (list 1 2) (list 1 2 3)))
($check-error (for-each list (list . #0=(1 2 . #0#)) (list 1 2 3)))

($check-error (for-each list #inert))
($check-error (for-each #inert (list 1 2)))
($check-error (for-each ((unwrap list) #inert) (list 1 2)))

($check-error (for-each list (list 1 2) #inert))
($check-error (for-each cons (list 1 2)))
