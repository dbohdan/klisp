;; check.k & test-helpers.k should be loaded

;;;
;;; Basic Functionality
;;;

;; operative?, applicative? & combiner?
($check-predicate (applicative? operative?))
($check-predicate (applicative? applicative?))
($check-predicate (applicative? combiner?))

($check-predicate (operative?))
($check-predicate (operative? $if $vau $lambda))
($check-predicate (operative? $cond . #0=($sequence . #0#)))

($check-predicate (applicative?))
($check-predicate (applicative? applicative? operative? combiner?))
($check-predicate (applicative? wrap . #0=(unwrap . #0#)))

($check-predicate (combiner?))
($check-predicate (combiner? $if $vau $lambda))
($check-predicate (combiner? $cond . #0=($sequence . #0#)))
($check-predicate (combiner?))
($check-predicate (combiner? applicative? operative? combiner?))
($check-predicate (combiner? wrap . #0=(unwrap . #0#)))

($check-not-predicate (operative? #t))
($check-not-predicate (operative? #f))
($check-not-predicate (operative? ((unwrap list) . symbol)))
($check-not-predicate (operative? ()))
($check-not-predicate (operative? (cons () ())))
($check-not-predicate (operative? #ignore))
($check-not-predicate (operative? (make-environment)))
($check-not-predicate (operative? #inert))
($check-not-predicate (operative? wrap))
($check-not-predicate (operative? (call/cc ($lambda (c) c))))
($check-not-predicate (operative? ($let (((enc . #ignore) 
					(make-encapsulation-type)))
				  (enc #inert))))
($check-not-predicate (operative? (memoize #inert)))
($check-not-predicate (operative? 1))
;($check-not-predicate (operative? 1.0))
($check-not-predicate (operative? #e+infinity))
;($check-not-predicate (operative? #i+infinity))
;($check-not-predicate (operative? #undefined))
;($check-not-predicate (operative? #real-with-no-primary-value))
($check-not-predicate (operative? "string"))
($check-not-predicate (operative? #\a))
($check-not-predicate (operative? (get-current-input-port)))

($check-not-predicate (applicative? #t))
($check-not-predicate (applicative? #f))
($check-not-predicate (applicative? ((unwrap list) . symbol)))
($check-not-predicate (applicative? ()))
($check-not-predicate (applicative? (cons () ())))
($check-not-predicate (applicative? #ignore))
($check-not-predicate (applicative? (make-environment)))
($check-not-predicate (applicative? #inert))
($check-not-predicate (applicative? $vau))
($check-not-predicate (applicative? (call/cc ($lambda (c) c))))
($check-not-predicate (applicative? ($let (((enc . #ignore) 
					(make-encapsulation-type)))
				  (enc #inert))))
($check-not-predicate (applicative? (memoize #inert)))
($check-not-predicate (applicative? 1))
;($check-not-predicate (applicative? 1.0))
($check-not-predicate (applicative? #e+infinity))
;($check-not-predicate (applicative? #i+infinity))
;($check-not-predicate (applicative? #undefined))
;($check-not-predicate (applicative? #real-with-no-primary-value))
($check-not-predicate (applicative? "string"))
($check-not-predicate (applicative? #\a))
($check-not-predicate (applicative? (get-current-input-port)))

($check-not-predicate (combiner? #t))
($check-not-predicate (combiner? #f))
($check-not-predicate (combiner? ((unwrap list) . symbol)))
($check-not-predicate (combiner? ()))
($check-not-predicate (combiner? (cons () ())))
($check-not-predicate (combiner? #ignore))
($check-not-predicate (combiner? (make-environment)))
($check-not-predicate (combiner? #inert))
($check-not-predicate (combiner? (call/cc ($lambda (c) c))))
($check-not-predicate (combiner? ($let (((enc . #ignore) 
					(make-encapsulation-type)))
				  (enc #inert))))
($check-not-predicate (combiner? (memoize #inert)))
($check-not-predicate (combiner? 1))
;($check-not-predicate (combiner? 1.0))
($check-not-predicate (combiner? #e+infinity))
;($check-not-predicate (combiner? #i+infinity))
;($check-not-predicate (combiner? #undefined))
;($check-not-predicate (combiner? #real-with-no-primary-value))
($check-not-predicate (combiner? "string"))
($check-not-predicate (combiner? #\a))
($check-not-predicate (combiner? (get-current-input-port)))

;; $vau
($check-predicate (operative? ($vau #ignore #ignore #inert)))
($check eq? (($vau #ignore #ignore)) #inert)
($check eq? (($vau #ignore #ignore 1)) 1)
($check equal? (($vau ls #ignore ls) 1 2) (list 1 2))
($check equal? (($vau #ignore env env)) (get-current-environment))
($check equal? (($vau (x y) #ignore (list y x)) 1 2) (list 2 1))
;; parameter trees (generalized parameter lists)
($check equal? (($vau ((x . y) (z)) #ignore (list z y x)) (1 . 2) (3)) (list 3 2 1))
($check equal? (($vau ((x y z)) #ignore (list z y x)) (1 2 3)) (list 3 2 1))
($check equal? (($vau ((x y . z)) #ignore (finite-list? z)) 
		#0=(1 2 3 . #0#)) #f)

;; test static scope of $vau, define an "inverted" $if and use it in the body
($let (($if ($vau (test a b) denv
	      (eval (list $if test b a)
		    denv))))
  ($check eq? 
	  (($vau () #ignore 
	     ($if #t 1 2))) 
	  2))

;; shared structure in ptree (but no shared symbols and no cycles)
($check equal?
	(($vau ((x . #0=(#ignore)) (y . #0#)) #ignore (list x y)) (1 4) (2 5))
	(list 1 2))

;; wrap
($check-predicate (applicative? wrap))
($check-predicate (applicative? (wrap ($vau #ignore #ignore #inert))))
($check-predicate (applicative? (wrap (wrap ($vau #ignore #ignore #inert)))))
($check-predicate (applicative? (wrap $if)))

;; unwrap
($check-predicate (applicative? unwrap))
($check-predicate (operative? 
		  (unwrap (wrap ($vau #ignore #ignore #inert)))))
($check-predicate (applicative? 
		  (unwrap (wrap (wrap ($vau #ignore #ignore #inert))))))
($check-predicate (operative? 
		  (unwrap list)))
($check-predicate (applicative? 
		  (unwrap (wrap list))))

;; $lambda
($check-predicate (applicative? ($lambda #ignore #inert)))
($check eq? (($lambda #ignore)) #inert)
($check eq? (($lambda #ignore 1)) 1)
($check equal? (($lambda ls ls) 1 2) (list 1 2))
($check equal? (($lambda (x y) (list y x)) 1 2) (list 2 1))
;; arguments should be eval'ed in the current environment
($check eq? 
	(($lambda (x) x) (get-current-environment)) 
	(get-current-environment))
;; parameter trees (generalized parameter lists)
($check equal? (($lambda ((x . y) (z)) (list z y x)) 
		(cons 1 2) (list 3)) (list 3 2 1))
($check equal? (($lambda ((x y z)) (list z y x)) (list 1 2 3)) (list 3 2 1))
($check equal? (($lambda ((x y . z)) (finite-list? z)) 
		(list . #0=(1 2 3 . #0#))) #f)

;; test static scope of $lambda, define an "inverted" $if and use it in the body
($let (($if ($vau (test a b) denv
	      (eval (list $if test b a)
		    denv))))
  ($check eq? 
	  (($lambda ()
	     ($if #t 1 2)))
	  2))
;; shared structure in ptree (but no shared symbols and no cycles)
($check equal?
	(($lambda ((x . #0=(#ignore)) (y . #0#)) (list x y)) 
	 (list 1 4) (list 2 5))
	(list 1 2))

;; apply
($check-predicate (applicative? apply))
($check equal? (apply cons (list 1 2)) (cons 1 2))
;; the underlying operative of list always returns its operand tree
;; so this is correct in kernel
($check eq? (apply list 3) 3)
;; the two argument case uses an empty environment
($check eq? (apply (wrap ($vau #ignore env ($binds? env $vau))) 
		   #inert) 
	#f)

($let ((empty-env (make-environment)))
  ($check eq? 
	  (apply (wrap ($vau #ignore env env)) #inert empty-env)
	  empty-env))

;; map
($check-predicate (applicative? map))
($check equal? (map + (list 1 2 3 4)) (list 1 2 3 4))
($check equal? (map cons (list 1 2 3 4) (list 10 20 30 40)) 
	(list (cons 1 10) (cons 2 20) (cons 3 30) (cons 4 40)))

($let ((p (cons () ())))
  ($check eq?
	  ($sequence (map (wrap ($vau #ignore env
				       (set-car! p env)))
			       (list 1))
		     (car p))
	  (get-current-environment)))

($let ((p (cons 0 ())))
  ($check eq?
	  ($sequence (map ($lambda (x)
				 (set-car! p (+ (car p) x)))
			       (list 1 2 3 4))
		     (car p))
	  10))


#;($check equal?
	(map ($lambda (x)
	       (- 0 x))
	     (list 1 . #0=(2 3 4 . #0#)))
	(list -1 . #1=(-2 -3 -4 . #1#)))

($check equal?
	(map ($lambda ls
	       (finite-list? ls))
	     . #0=((list 1 2 3 4)
		   (list 10 20 30 40)
		   . #0#))
	(list #f #f #f #f))


;;;
;;; Error Checking and Robustness
;;;

;; operative?, applicative? & combiner?
($check-error (operative? $vau . $cond))
($check-error (applicative? wrap . unwrap))
($check-error (combiner? $vau . wrap))

;; $vau
($check-error ($vau))
($check-error ($vau #ignore))
($check-error ($vau #ignore #ignore . #inert))

;; repeated symbols aren't allowed
($check-error ($vau (x x) #ignore x))
;; same symbol in ptree and env aren't allowed either
($check-error ($vau (x) x x))
;; this counts as repeating symbols
($check-error ($vau ((x . #0=(z)) (y . #0#)) #ignore (list x y z)))
;; cycles aren't allowed either (containing symbols or not)
($check-error ($vau (x . #0=(#ignore . #0#)) #ignore x))

($check-error ($vau #ignore #t #inert))
($check-error ($vau #f #ignore #inert))
($check-error ($vau #inert #inert #inert))

;; wrap
($check-error (wrap))
($check-error (wrap $vau $vau))
($check-error (wrap . #0=($vau . #0#)))

($check-error (wrap 1))
($check-error (wrap #t))
($check-error (wrap #inert))
($check-error (wrap #ignore))
($check-error (wrap ()))
($check-error ($let/cc cont (wrap cont)))
($check-error (wrap (memoize #inert)))

;; unwrap
($check-error (unwrap))
($check-error (unwrap list list))
($check-error (unwrap . #0=(list . #0#)))

($check-error (unwrap 1))
($check-error (unwrap #t))
($check-error (unwrap #inert))
($check-error (unwrap #ignore))
($check-error ($let/cc cont (unwrap cont)))
($check-error (unwrap (memoize #inert)))

;; $lambda
($check-error ($lambda))
($check-error ($lambda #ignore . #inert))

;; repeated symbols aren't allowed
($check-error ($lambda (x x) x))
;; this counts as repeating symbols
($check-error ($lambda ((x . #0=(z)) (y . #0#)) (list x y z)))
;; cycles aren't allowed either (containing symbols or not)
($check-error ($lambda (x . #0=(#ignore . #0#)) x))

($check-error ($lambda #t #inert))
($check-error ($lambda #f #inert))
($check-error ($lambda #inert #inert))

;; apply
($check-error (apply))
($check-error (apply list))
($check-error (apply list 1 (get-current-environment #inert)))
($check-error (apply . #0=(list 1 (get-current-environment) . #0#)))

($check-error (apply (unwrap list) 1))
($check-error (apply #ignore 1))
($check-error (apply 1 1))
($check-error (apply list 1 #inert))
($check-error (apply list 1 1))
($check-error (apply cons (list 1)))

;; map
($check-error (map))
;; the list can't be empty
($check-error (map list))

($check-error (map list (list 1 2) (list 1 2 3)))
($check-error (map list (list . #0=(1 2 . #0#)) (list 1 2 3)))

($check-error (map list #inert))
($check-error (map #inert (list 1 2)))
($check-error (map ((unwrap list) #inert) (list 1 2)))

($check-error (map list (list 1 2) #inert))
($check-error (map cons (list 1 2)))
