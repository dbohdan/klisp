;; check.k & test-helpers.k should be loaded

;;;
;;; Basic Functionality
;;;

;; set-car! & set-cdr!
($let ((pair (cons () ())))
  ($check-predicate (inert? (set-car! pair 1)))
  ($check-predicate (inert? (set-cdr! pair 2)))
  ($check equal? (car pair) 1)
  ($check equal? (cdr pair) 2)
  (set-car! pair pair)
  (set-cdr! pair pair)
  ($check eq? (car pair) pair)
  ($check eq? (cdr pair) pair))

;; copy-es-immutable
($let* ((orig (list (cons 1 2) (cons 3 4)))
	(copy (copy-es-immutable orig))
	(copy2 (copy-es-immutable copy)))
  ($check equal? orig copy)
  ($check-predicate (mutable-pair? orig))
  ($check-predicate (immutable-pair? copy))
  ($check equal? orig copy2)
  ($check-predicate (immutable-pair? copy2)))

;; encycle!
($check equal? ($let ((l 1)) (encycle! l 0 0) l)
	1)
($check equal? ($let ((l (list 1 2 3 4 5))) (encycle! l 4 0) l)
	(list 1 2 3 4 5))
($check equal? ($let ((l (list 1 2 3 4 5))) (encycle! l 2 3) l)
	(list 1 2 . #0=(3 4 5 . #0#)))
($check equal? ($let ((l (list* 1 2 3 4 5))) (encycle! l 0 3) l)
	(list . #0=(1 2 3 . #0#)))

;; append!
($let ()
  ($define! l1 (list 1 2))
  ($define! l2 (list 3 4))
  ($define! l3 (list 5 6))

  ($check equal? ($sequence (append! l1 ()) l1) (list 1 2))
  ($check equal? ($sequence (append! l1 () ()) l1) (list 1 2))
  ($check equal? ($sequence (append! l1 l2) l1) (list 1 2 3 4))
  ($check equal? ($sequence (append! l1 () () l3 ()) l1) (list 1 2 3 4 5 6))

  ($define! l1 (list 1 2))
  ($define! l2 (list 3 4))
  ($define! l3 (list . #0=(5 6 . #0#)))

  (append! l1 l2 l3)
  ($check equal? l1 (list 1 2 3 4 . #2=(5 6 . #2#)))
  ($check eq? (cddddr l1) l3)

  ($define! l1 (list 1 2))
  ($define! l2 (list 3 4))
  ($define! l3 (list 5 6))

  ($check equal? 
	  ($sequence (append! l1 . #3=(l2 l3 . #3#)) l1) 
	  (list 1 2 . #4=(3 4 5 6 . #4#)))

  ($define! l1 (list 1 2))
  ($define! l2 (list 3 4))
  ($define! l3 (list 5 6))

  ($check equal? 
   	  ($sequence (append! l1 l2 l3 . #5=(() () . #5#)) l1)
   	  (list 1 2 3 4 5 6))

  ($define! l1 (list 1 2))
  ($define! l2 (list 3 4))
  ($define! l3 (list 5 6))

  ($check equal? 
  	  ($sequence (append! l1 () . #6=(()  l2 () l3 () . #6#)) l1) 
  	  (list 1 2 . #7=(3 4 5 6 . #7#))))

;; copy-es
($let* ((orig (list (cons 1 2) (cons 3 4)))
	(copy (copy-es orig)))
  ($check equal? orig copy)
  ($check-predicate (mutable-pair? orig))
  ($check-predicate (mutable-pair? copy))
  ($check not-eq? orig copy))

;; assq
($check equal? (assq #inert ()) ())
($check equal? (assq 3 (list (list 1 10) (list 2 20))) ())
($check equal? (assq 1 (list (list 1 10) (list 2 20))) (list 1 10))
($check equal?
	(assq 1 (list . #0=((list 1 10) (list 2 20) (list 1 15) . #0#)))
	(list 1 10))
($check equal?
	(assq 4 (list . #0=((list 1 10) (list 2 20) (list 1 15) . #0#)))
	())
($check equal?
	(assq (list 1) (list (list (list 1) 1) (list (list 2) 2)))
	())

;; memq
($check-predicate (memq? 1 (list 1 2)))
($check-predicate (memq? 2 (list 1 2)))
($check-not-predicate (memq? 1 ()))
($check-not-predicate (memq? 3 (list 1 2)))
($check-not-predicate (memq? (list 1) (list (list 1) 2)))
($check-not-predicate (memq? (list 2) (list 1 (list 2))))
($check-predicate
 (memq? 3 (list . #0=(1 2 3 . #0#))))
($check-not-predicate
	(memq? 4 (list . #0=(1 2 1 . #0#))))


;;;
;;; Error Checking and Robustness
;;;

;; set-car! & set-cdr!
($check-error (set-car!))
($check-error (set-car! (cons () ())))
($check-error (set-car! (cons () ()) #inert #inert))

($check-error (set-car! () #inert))
($check-error (set-car! 1 #inert))
($check-error (set-car! (get-current-environment) #inert))
($check-error (set-car! ($lambda #ignore) #inert))
($check-error (set-car! ($vau #ignore #ignore) #inert))

($check-error (set-cdr!))
($check-error (set-cdr! (cons () ())))
($check-error (set-cdr! (cons () ()) #inert #inert))

($check-error (set-cdr! () #inert))
($check-error (set-cdr! 1 #inert))
($check-error (set-cdr! (get-current-environment) #inert))
($check-error (set-cdr! ($lambda #ignore) #inert))
($check-error (set-cdr! ($vau #ignore #ignore) #inert))

($let ((imm-pair (copy-es-immutable (cons () ()))))
  ($check-error (set-car! imm-pair #inert))
  ($check-error (set-cdr! imm-pair #inert))
  ($check-predicate (null? (car imm-pair)))
  ($check-predicate (null? (cdr imm-pair))))

;; copy-es-immutable
($check-error (copy-es-immutable))
($check-error (copy-es-immutable (cons () ()) (cons () ())))

;; encycle!
($check-error (encycle!))
($check-error (encycle! (list 1 2 3)))
($check-error (encycle! (list 1 2 3) 1))
($check-error (encycle! (list 1 2 3) 1 2 3))

($check-error (encycle! (list 1 2 3) 2 2))
($check-error (encycle! (list 1 2 3) -1 2))
($check-error (encycle! (list 1 2 3) 0 -2))
($check-error (encycle! (list 1 2 3) 0 #e+infinity))

;; append!
;; ASK does the report assert that the lists remains unmodified??
;; probably should for robust implementations

($check-error (append!))
($check-error (append! ()))
($check-error (append! (list . #0=(1 2 . #0#)) ()))
($check-error (append! (list 1 2) 3 (list 4 5)))
($check-error (append! (list 1 2) 3 ()))

($check-error (append! ((unwrap list) . (1 2 . #0=(3))) 
		       ((unwrap list) . (4 5 . #0#)) 
		       ()))

;; ASK if this is valid or not
;; ($check-error (append! ((unwrap list) . (1 2 . #0=(3)))
;; 		       ((unwrap list) . (4 5 . #0#))))


;; copy-es
($check-error (copy-es))
($check-error (copy-es (cons () ()) (cons () ())))

;; assq
($check-error (assq))
($check-error (assq 2))
($check-error (assq 2 (list (list 1 1) (list 2 2)) ()))
($check-error (assq . #0=(2 (list (list 1 1) (list 2 2)) . #0#)))

($check-error (assq 2 (list* (list 1 1) 2)))
($check-error (assq 2 (list* (list 1 1) (list 2 2) #inert)))
($check-error (assq 4 (list (list 1 1) (list 2 2) #inert (list 4 4))))
($check-error (assq 2 (list (list 1 1) (list 2 2) #inert (list 4 4))))

;; memq
($check-error (memq?))
($check-error (memq? 2))
($check-error (memq? 2 (list 1 2) ()))
($check-error (memq? . #0=(2 (list 1 2) . #0#)))

($check-error (memq? 2 (list* 1 2)))
($check-error (memq? 2 (list* 1 2 3)))

