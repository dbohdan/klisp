;; check.k & test-helpers.k should be loaded

;;;
;;; Basic Functionality
;;;

;; boolean?
($check-predicate (applicative? boolean?))
($check-predicate (boolean?))
($check-predicate (boolean? #t))
($check-predicate (boolean? #f))
($check-predicate (boolean? #t #t #f #f))
($check-predicate (boolean? #f . #0=(#t . #0#)))

($check-not-predicate (boolean? ((unwrap list) . symbol)))
($check-not-predicate (boolean? ()))
($check-not-predicate (boolean? (cons () ())))
($check-not-predicate (boolean? #ignore))
($check-not-predicate (boolean? (make-environment)))
($check-not-predicate (boolean? #inert))
($check-not-predicate (boolean? $vau))
($check-not-predicate (boolean? wrap))
($check-not-predicate (boolean? (call/cc ($lambda (c) c))))
($check-not-predicate (boolean? ($let (((enc . #ignore) 
					(make-encapsulation-type)))
				  (enc #inert))))
($check-not-predicate (boolean? (memoize #inert)))
($check-not-predicate (boolean? 1))
($check-not-predicate (boolean? -1/2))
($check-not-predicate (boolean? 1.0))
($check-not-predicate (boolean? #e+infinity))
($check-not-predicate (boolean? #i+infinity))
($check-not-predicate (boolean? #undefined))
($check-not-predicate (boolean? #real))
($check-not-predicate (boolean? "string"))
($check-not-predicate (boolean? #\a))
($check-not-predicate (boolean? (get-current-input-port)))

;; basic eq?-ness and not?
($check-predicate (applicative? not?))

($check eq? #t #t)
($check eq? #f #f)
($check not-eq? #t #f)
($check not-eq? #f #t)
($check eq? (not? #t) #f)
($check eq? (not? #f) #t)

;; basic equal?-ness and not?
($check equal? #t #t)
($check equal? #f #f)
($check not-equal? #t #f)
($check not-equal? #f #t)
($check equal? (not? #t) #f)
($check equal? (not? #f) #t)

;; and? & or?
($check-predicate (applicative? and?))
($check-predicate (and?))
($check-predicate (and? #t))
($check-predicate (and? #t #t))
($check-predicate (and? #t #t #t))
($check-predicate (and? #t . #0=(#t . #0#)))
($check-not-predicate (and? #f))
($check-not-predicate (and? #t #t #f))

($check-predicate (applicative? or?))
($check-predicate (or? #t))
($check-predicate (or? #f #t))
($check-predicate (or? #f #f #t))
($check-predicate (or? #f . #0=(#t . #0#)))
($check-not-predicate (or?))
($check-not-predicate (or? #f))
($check-not-predicate (or? #f #f #f))

;; $and? & $or?
($check-predicate (operative? $and?))
($check-predicate ($and?))
($check-predicate ($and? #t))
($check-predicate ($and? (eq? #t #t) #t)) ;; test some evaluation too!
($check-predicate ($and? #t (eq? #f #f) #t))
($check-not-predicate ($and? #t . #0=((eq? #t #f) . #0#)))
($check-not-predicate ($and? #f))
($check-not-predicate ($and? #t #t #f))
($check-not-predicate ($and? #f (/ 1 0))) ;; test conditional evaluation

;; check tail recursiveness
($let ((p (cons 1 2)))
  ($check-predicate ($sequence ($and? ($let/cc cont1
					(set-car! p cont1)
					($and? ($let/cc cont2
						(set-cdr! p cont2)
						#t))))
			       (eq? (car p) (cdr p)))))

($check-predicate (operative? $or?))
($check-predicate ($or? #t))
($check-predicate ($or? #f (eq? #t #t) #t)) ;; test some evaluation too!
($check-predicate ($or? #f #f #t))
($check-predicate ($or? #t (/ 1 0)))
($check-predicate ($or? #f . #0=(#t . #0#)))
($check-not-predicate ($or? #f))
($check-not-predicate ($or?))

($let ((p (cons 1 2)))
  ($check-predicate ($sequence ($or? ($let/cc cont1
					(set-car! p cont1)
					($or? ($let/cc cont2
						(set-cdr! p cont2)
						#t))))
			       (eq? (car p) (cdr p)))))

($let ((p (cons 1 2)))
  ($check-predicate ($sequence ($and? ($let/cc cont1
					(set-car! p cont1)
					($or? ($let/cc cont2
						(set-cdr! p cont2)
						#t))))
			       (eq? (car p) (cdr p)))))

($let ((p (cons 1 2)))
  ($check-predicate ($sequence ($or? ($let/cc cont1
					(set-car! p cont1)
					($and? ($let/cc cont2
						(set-cdr! p cont2)
						#t))))
			       (eq? (car p) (cdr p)))))

;;;
;;; Error Checking and Robustness
;;;

;; boolean?
($check-error (boolean? #t . #f))

;; not?
($check-error (not?))
($check-error (not? 1))
($check-error (not? #inert))
($check-error (not? #t #f))

;; and? & or?
($check-error (and? #t #f 0))
($check-error (or? #f #t 0))
($check-error (and? #t . #f))

;; $and? & $or?
($check-error ($and? #t 0 #t))
($check-error ($or? #f 0 #f))

;; check boolean in last operand
($check-error ($and? #t 0))
($check-error ($or? #f 0))

