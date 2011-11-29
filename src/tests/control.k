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


;; string-for-each
($check-predicate (applicative? string-for-each))
($check eq? (string-for-each char-upcase "abcd") #inert)
($check eq? (string-for-each char<? "abcd" "efgh") #inert)

($let ((p (cons () ())))
  ($check eq?
	  ($sequence (string-for-each (wrap ($vau #ignore env
				       (set-car! p env)))
			       "a")
		     (car p))
	  (get-current-environment)))
($let ((p (cons 0 ())))
  ($check eq?
	  ($sequence (string-for-each ($lambda (x)
					(set-car! p (+ (car p) 
						       (char->integer x))))
			       "abcd")
		     (car p))
	  (apply + (map char->integer (string->list "abcd")))))
($let ((p (cons 0 ())))
  ($check eq?
	  ($sequence (string-for-each ($lambda (x y )
				 (set-car! p (+ (car p) 
						 (char->integer x) 
						 (char->integer y))))
			       "abc"
			       "def")
		     (car p))
	  (apply + (map char->integer (string->list "abcdef")))))


($let ((p (cons 0 ())))
  ($check eq?
	  ($sequence (string-for-each ($lambda ls
				 (set-car! p (finite-list? ls)))
		     . #0=("abc"
			   "def"
			   . #0#))
		     (car p))
	  #f))


;; vector-for-each
($check-predicate (applicative? vector-for-each))
($check eq? (vector-for-each + (vector 1 2 3)) #inert)
($check eq? (vector-for-each <? (vector 1 2) (vector 3 4)) 
	#inert)

($let ((p (cons () ())))
  ($check eq?
	  ($sequence (vector-for-each (wrap ($vau #ignore env
				       (set-car! p env)))
			       (vector 1))
		     (car p))
	  (get-current-environment)))
($let ((p (cons 0 ())))
  ($check eq?
	  ($sequence (vector-for-each ($lambda (x)
					(set-car! p (+ (car p) x)))
			       (vector 1 2 3 4))
		     (car p))
	  10))
($let ((p (cons 0 ())))
  ($check eq?
	  ($sequence (vector-for-each ($lambda (x y )
				 (set-car! p (+ (car p) x y)))
			       (vector 1 2 3 4)
			       (vector 10 20 30 40))
		     (car p))
	  110))


($let ((p (cons 0 ())))
  ($check eq?
	  ($sequence (vector-for-each ($lambda ls
				 (set-car! p (finite-list? ls)))
		     . #0=((vector 1 2)
			   (vector 3 4)
			   . #0#))
		     (car p))
	  #f))

;; bytevector-for-each
($check-predicate (applicative? bytevector-for-each))
($check eq? (bytevector-for-each + (bytevector 1 2 3)) #inert)
($check eq? (bytevector-for-each <? (bytevector 1 2) (bytevector 3 4)) 
	#inert)

($let ((p (cons () ())))
  ($check eq?
	  ($sequence (bytevector-for-each (wrap ($vau #ignore env
				       (set-car! p env)))
			       (bytevector 1))
		     (car p))
	  (get-current-environment)))
($let ((p (cons 0 ())))
  ($check eq?
	  ($sequence (bytevector-for-each ($lambda (x)
					(set-car! p (+ (car p) x)))
			       (bytevector 1 2 3 4))
		     (car p))
	  10))
($let ((p (cons 0 ())))
  ($check eq?
	  ($sequence (bytevector-for-each ($lambda (x y )
				 (set-car! p (+ (car p) x y)))
			       (bytevector 1 2 3 4)
			       (bytevector 10 20 30 40))
		     (car p))
	  110))

($let ((p (cons 0 ())))
  ($check eq?
	  ($sequence (bytevector-for-each ($lambda ls
				 (set-car! p (finite-list? ls)))
		     . #0=((bytevector 1 2)
			   (bytevector 3 4)
			   . #0#))
		     (car p))
	  #f))

;; $when
($check-predicate (operative? $when))
($check-predicate (inert? ($when #t)))
($check-predicate (inert? ($when #f)))
($check-predicate (inert? ($when #t 1)))
($check-predicate (inert? ($when #f 1)))
($check-predicate (inert? ($when #t 1 2)))
($check-predicate (inert? ($when #f 1 2)))

($let ((p (cons () ())))
  ($check equal? ($sequence ($when #f (set-car! p 1))
			    (car p))
	  ()))

($let ((p (cons () ())))
  ($check eq? ($sequence ($when ($sequence 
				  (set-car! p (get-current-environment)) 
				  #f))
			(car p))
	 (get-current-environment)))

($let ((p (cons () ())))
  ($check eq? ($sequence ($when #t (set-car! p (get-current-environment)))
			 (car p))
	  (get-current-environment)))

;; check tail recursiveness
($let ((p (cons 1 2)))
  ($check-predicate ($sequence ($when #t ($let/cc cont1
					   (set-car! p cont1)
					   ($when #t
						  ($let/cc cont2
						    (set-cdr! p cont2)))))
			       (eq? (car p) (cdr p)))))

;; $unless
($check-predicate (operative? $unless))
($check-predicate (inert? ($unless #t)))
($check-predicate (inert? ($unless #f)))
($check-predicate (inert? ($unless #t 1)))
($check-predicate (inert? ($unless #f 1)))
($check-predicate (inert? ($unless #t 1 2)))
($check-predicate (inert? ($unless #f 1 2)))

($let ((p (cons () ())))
  ($check equal? ($sequence ($unless #t (set-car! p 1))
			    (car p))
	  ()))

($let ((p (cons () ())))
  ($check eq? ($sequence ($unless ($sequence 
				  (set-car! p (get-current-environment)) 
				  #t))
			(car p))
	 (get-current-environment)))

($let ((p (cons () ())))
  ($check eq? ($sequence ($unless #f (set-car! p (get-current-environment)))
			 (car p))
	  (get-current-environment)))

;; check tail recursiveness
($let ((p (cons 1 2)))
  ($check-predicate ($sequence ($unless #f ($let/cc cont1
					   (set-car! p cont1)
					   ($unless #f
						  ($let/cc cont2
						    (set-cdr! p cont2)))))
			       (eq? (car p) (cdr p)))))

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
($check-error (for-each list)) ; the list can't be empty

($check-error (for-each list (list 1 2) (list 1 2 3)))
($check-error (for-each list (list . #0=(1 2 . #0#)) (list 1 2 3)))

($check-error (for-each list #inert))
($check-error (for-each #inert (list 1 2)))
($check-error (for-each ((unwrap list) #inert) (list 1 2)))

($check-error (for-each list (list 1 2) #inert))
($check-error (for-each cons (list 1 2)))


;; string-for-each
($check-error (string-for-each))
($check-error (string-for-each char-upcase)) ; the list can't be empty

($check-error (string-for-each char<? "ab" "abc"))

($check-error (string-for-each char-upcase #inert))
($check-error (string-for-each #inert "abc"))
($check-error (string-for-each (unwrap char-upcase) "abc"))

($check-error (string-for-each char<? "abc" #inert))
($check-error (string-for-each cons "abc"))

;; vector-for-each
($check-error (vector-for-each))
($check-error (vector-for-each char-upcase)) ; the list can't be empty

($check-error (vector-for-each <? (vector 1 2) (vector 1 2 3)))

($check-error (vector-for-each char-upcase #inert))
($check-error (vector-for-each #inert (vector 1 2)))
($check-error (vector-for-each (unwrap char-upcase) (vector 1)))

($check-error (vector-for-each <? (vector 1 2) #inert))
($check-error (vector-for-each cons (vector 1 2 3)))

;; bytevector-for-each
($check-error (bytevector-for-each))
($check-error (bytevector-for-each +)) ; the list can't be empty

($check-error (bytevector-for-each <? (bytevector 1 2) 
				   (bytevector 1 2 3)))

($check-error (bytevector-for-each + #inert))
($check-error (bytevector-for-each #inert (bytevector 1 2 3)))
($check-error (bytevector-for-each (unwrap char-upcase) 
				   (bytevector 1 2)))

($check-error (bytevector-for-each <? (bytevector 1 2) #inert))
($check-error (bytevector-for-each cons
			       (bytevector 1 2 3)))


;; $when
($check-error ($when))
($check-error ($when #t . 3))
($check-error ($when #f . 3))
($check-error ($when #inert 1))

;; $unless
($check-error ($unless))
($check-error ($unless #t . 3))
($check-error ($unless #f . 3))
($check-error ($unless #inert 1))



