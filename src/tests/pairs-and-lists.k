;; check.k & test-helpers.k should be loaded

;; TODO use mutable-pair to verify that cons, list append etc construct with 

;;;
;;; Basic Functionality
;;;

;; null? & pair?
($check-predicate (null?))
($check-predicate (null? ()))
($check-predicate (null? () () ()))
($check-predicate (null? () . #0=(() . #0#)))

($check-predicate (pair?))
($check-predicate (pair? (cons () ())))
($check-predicate (pair? (cons () ()) 
                         (copy-es-immutable (cons () ())) (cons () ())))
($check-predicate (pair? (cons () ()) . 
                         #0=((copy-es-immutable (cons () ())) . #0#)))

($check-not-predicate (null? #t))
($check-not-predicate (null? ((unwrap list) . symbol)))
($check-not-predicate (null? (cons () ())))
($check-not-predicate (null? #ignore))
($check-not-predicate (null? (make-environment)))
($check-not-predicate (null? #inert))
($check-not-predicate (null? $vau))
($check-not-predicate (null? wrap))
($check-not-predicate (null? (call/cc ($lambda (c) c))))
($check-not-predicate (null? ($let (((enc . #ignore) 
                                     (make-encapsulation-type)))
                               (enc #inert))))
($check-not-predicate (null? (memoize #inert)))
($check-not-predicate (null? 1))
($check-not-predicate (null? 1.0))
($check-not-predicate (null? #e+infinity))
($check-not-predicate (null? #i+infinity))
($check-not-predicate (null? #undefined))
($check-not-predicate (null? #real))
($check-not-predicate (null? "string"))
($check-not-predicate (null? #\a))
($check-not-predicate (null? (get-current-input-port)))

($check-not-predicate (pair? #t))
($check-not-predicate (pair? ((unwrap list) . symbol)))
($check-not-predicate (pair? ()))
($check-not-predicate (pair? #ignore))
($check-not-predicate (pair? (make-environment)))
($check-not-predicate (pair? #inert))
($check-not-predicate (pair? $vau))
($check-not-predicate (pair? wrap))
($check-not-predicate (pair? (call/cc ($lambda (c) c))))
($check-not-predicate (pair? ($let (((enc . #ignore) 
                                     (make-encapsulation-type)))
                               (enc #inert))))
($check-not-predicate (pair? (memoize #inert)))
($check-not-predicate (pair? 1))
($check-not-predicate (pair? 1.0))
($check-not-predicate (pair? #e+infinity))
($check-not-predicate (pair? #i+infinity))
($check-not-predicate (pair? #undefined))
($check-not-predicate (pair? #real))
($check-not-predicate (pair? "string"))
($check-not-predicate (pair? #\a))
($check-not-predicate (pair? (get-current-input-port)))

;; basic eq?-ness
($check eq? () ())
($let ((p (cons () ())))
  ($check eq? p p))
($check not-eq? (cons () ()) (cons () ()))
($check not-eq? (cons () ()) ())

;; basic equal?-ness
($check equal? (cons () ()) (cons () ()))
($check equal? () ())
($check not-equal? (cons () ()) (cons (cons () ()) ()))
($check not-equal? (cons () ()) ())

;; list & list*
($check equal? (list) ())
($check equal? (list 1 2 3) (cons 1 (cons 2 (cons 3 ()))))
($check equal? ((unwrap list) 1 2 . 3) (list* 1 2 3))
($check equal? ((unwrap list) . #inert) (list* #inert))
($check equal? (list* #inert) #inert)
($check equal? (list* 1 2 3) (cons 1 (cons 2 3)))
($check equal? (list 1 . #0=(2 3 . #0#)) ((unwrap list) 1 . #0#))

;; car, cdr & co
($check equal? (car (cons 1 2)) 1)
($check equal? (cdr (cons 1 2)) 2)

($let* ((tree2 (cons 1 2))
        (tree4 (cons tree2 (cons 3 4)))
        (tree8 (cons tree4 (cons (cons 5 6) (cons 7 8))))
        (tree16 (cons tree8 (cons (cons (cons 9 10) (cons 11 12))
                                  (cons (cons 13 14) (cons 15 16))))))
  ($check eq? (car tree2) 1)
  ($check eq? (cdr tree2) 2)

  ($check eq? (caar tree4) 1)
  ($check eq? (cdar tree4) 2)
  ($check eq? (cadr tree4) 3)
  ($check eq? (cddr tree4) 4)

  ($check eq? (caaar tree8) 1)
  ($check eq? (cdaar tree8) 2)
  ($check eq? (cadar tree8) 3)
  ($check eq? (cddar tree8) 4)
  ($check eq? (caadr tree8) 5)
  ($check eq? (cdadr tree8) 6)
  ($check eq? (caddr tree8) 7)
  ($check eq? (cdddr tree8) 8)

  ($check eq? (caaaar tree16) 1)
  ($check eq? (cdaaar tree16) 2)
  ($check eq? (cadaar tree16) 3)
  ($check eq? (cddaar tree16) 4)
  ($check eq? (caadar tree16) 5)
  ($check eq? (cdadar tree16) 6)
  ($check eq? (caddar tree16) 7)
  ($check eq? (cdddar tree16) 8)
  ($check eq? (caaadr tree16) 9)
  ($check eq? (cdaadr tree16) 10)
  ($check eq? (cadadr tree16) 11)
  ($check eq? (cddadr tree16) 12)
  ($check eq? (caaddr tree16) 13)
  ($check eq? (cdaddr tree16) 14)
  ($check eq? (cadddr tree16) 15)
  ($check eq? (cddddr tree16) 16))

;; make-list
($check-predicate (null? (make-list 0)))
($check-predicate (mutable-pair? (make-list 1)))
($check equal? (make-list 2) (list #inert #inert))
($check equal? (make-list 3 "val") (list "val" "val" "val"))

;; list-copy
($check-predicate (null? (list-copy ())))
($check-predicate (mutable-pair? (list-copy (list 1))))
($check-predicate (mutable-pair? (list-copy (copy-es-immutable (list 1)))))
($check equal? (list-copy (list 1 2 3)) (list 1 2 3))
($check equal? (list-copy (list . #1=(1 2 . #1#))) (list . #2=(1 2 . #2#)))

;; reverse
($check-predicate (null? (reverse ())))
($check-predicate (mutable-pair? (reverse (list 1))))
($check-predicate (mutable-pair? (reverse (copy-es-immutable (list 1)))))
($check equal? (reverse (list 1)) (list 1))
($check equal? (reverse (list 1 2 3)) (list 3 2 1))

;; get-list-metrics
($check equal? (get-list-metrics ()) (list 0 1 0 0))
($check equal? (get-list-metrics #inert) (list 0 0 0 0))
($check equal? (get-list-metrics (list 1)) (list 1 1 1 0))
($check equal? (get-list-metrics (list* 1 2)) (list 1 0 1 0))
($check equal? (get-list-metrics (list 1 2 . #0=(3 4 5 . #0#))) (list 5 0 2 3))
($check equal? (get-list-metrics (list . #0=(1 2 . #0#))) (list 2 0 0 2))

;; list-tail
($check equal? (list-tail (list 1 2 3 4 5) 0) (list 1 2 3 4 5))
($check equal? (list-tail (list 1 2 3 4 5) 1) (list 2 3 4 5))
($check equal? (list-tail (list* 1 2 3) 2) 3)
($check equal? (list-tail (list . #0=(1 2 3 4 5 . #0#)) 10) (list . #0#))

;; length
($check =? (length ()) 0)
($check =? (length "string") 0) ; in Kernel improper lists have length too
($check =? (length (list 1 2 3 4 5)) 5)
($check =? (length (list* 1 2 3 4 5 "string")) 5)
($check =? (length (list 1 2 . #0=(3 4 5 . #0#))) #e+infinity)

;; list-ref
($check =? (list-ref (list 1 2 3 4 5) 0) 1)
($check =? (list-ref (list 1 2 3 4 5) 1) 2)
;; see kgpairs_lists.c for rationale on allowing
;; improper lists as argument to list-ref 
($check =? (list-ref (list* 1 2 3 4) 2) 3)
($check =? (list-ref (list . #0=(1 2 3 4 5 . #0#)) 10) 1)

;; append
($check equal? (append) ())
($check equal? (append ()) ())
($check equal? (append () ()) ())
($check equal? (append (list 1 2 3) (list 4 5) (list 6)) (list 1 2 3 4 5 6))
($check equal? (append (list 1 2) (list 3 4 5 6) ()) (list 1 2 3 4 5 6))
($check equal? (append () (list 1) (list 2 3 4)) (list 1 2 3 4))
($check equal? (append (list 1 2) (list 3 4) 5) (list* 1 2 3 4 5))
($let ((l1 (list 1 2)) (l2 (list 3 4))) ; the last list isn't copied
  ($check eq? (cddr (append l1 l2)) l2))
($let ((l1 (list 1 2)) (l2 (list 3 4))) ; here the last list is copied
  ($check not-eq? (cddr (append l1 l2 ())) l2))
($check equal?
        (append (list 1 2) (list 3 4) . #0=((list 5 6) . #0#))
        (list 1 2 3 4 . #1=(5 6 . #1#)))
($check equal?
        (append () () . #0=(() (list 1 2) () . #0#))
        (list . #1=(1 2 . #1#)))
($check equal?
        (append (list 1 2) (list 3 4) . #0=(() () . #0#))
        (list 1 2 3 4))

;; list-neighbors
($check equal? (list-neighbors ()) ())
($check equal? (list-neighbors (list 1)) ())
($check equal? (list-neighbors (list 1 2)) (list (list 1 2)))
($check equal? 
        (list-neighbors (list 1 2 3 4))
        (list (list 1 2) (list 2 3) (list 3 4)))
($check equal? 
        (list-neighbors (list . #0=(1 2 3 4 . #0#)))
        (list . #1=((list 1 2) (list 2 3) (list 3 4) (list 4 1) . #1#)))
($check equal? 
        (list-neighbors (list 1 2 . #0=(3 4 . #0#)))
        (list (list 1 2) (list 2 3) . #1=((list 3 4) (list 4 3) . #1#)))

;; filter
($check equal? (filter number? ()) ())
($check equal? (filter number? (list #t #f #t)) ())
($check equal? (filter number? (list 1 2 3)) (list 1 2 3))
($check equal? (filter number? (list 1 #t 2 #f)) (list 1 2))
($check equal? 
        (filter number? (list 1 #t . #0=(2 #f . #0#)))
        (list 1 . #1=(2 . #1#)))
($check equal? 
        (filter number? (list #t 1 #f . #0=(#t #f . #0#)))
        (list 1))
($check equal? 
        (filter number? (list #t #f . #0=(#t #f . #0#)))
        ())

($check equal? ; filter should use an empty environment
        (filter (wrap ($vau #ignore denv ($binds? denv $if)))
                (list 1 2 3))
        ())

;; filter + continuation capturing and mutation
;; TODO

;; assoc
($check equal? (assoc #inert ()) ())
($check equal? (assoc 3 (list (list 1 10) (list 2 20))) ())
($check equal? (assoc 1 (list (list 1 10) (list 2 20))) (list 1 10))
($check equal? (assoc 1 (list (list 1 10) (list 2 20)) =?) (list 1 10))
($check equal?
        (assoc 1 (list . #0=((list 1 10) (list 2 20) (list 1 15) . #0#)))
        (list 1 10))
($check equal?
        (assoc 4 (list . #0=((list 1 10) (list 2 20) (list 1 15) . #0#)))
        ())
($check equal?
        (assoc (list 1) (list (list (list 1) 1) (list (list 2) 2)))
        (list (list 1) 1))

($check equal?
        (assoc 4 (list . #0=((list 1 10) (list 2 20) (list 1 15) . #0#))
               =?)
        ())
;; member?
($check-predicate (member? 1 (list 1 2)))
($check-predicate (member? 2 (list 1 2)))
($check-not-predicate (member? 1 ()))
($check-not-predicate (member? 3 (list 1 2)))
($check-predicate (member? (list 1) (list (list 1) 2)))
($check-predicate (member? (list 2) (list 1 (list 2))))
($check-predicate
 (member? (list 1 3) (list . #0=(1 2 (list 1 3) . #0#))))
($check-not-predicate
 (member? 4 (list . #0=(1 2 1 . #0#))))

($check-predicate (member? -1 (list 1 2) ($lambda (x y) (=? x (- 0 y)))))
($check-not-predicate (member? 1 (list 1 2 . #0=(3 4 . #0#)) 
                               ($lambda (x y) (=? x (- 0 y)))))

;; finite-list?
($check-predicate (finite-list? ()))
($check-predicate (finite-list? (list 1)))
($check-predicate (finite-list? (list 1 2)))
($check-predicate (finite-list? (list 1 2) (list 1 2 3) ()))
($check-predicate (finite-list? (list 1 2) . #0=((list 1 2 3) () . #0#)))
($check-not-predicate (finite-list? 1))
($check-not-predicate (finite-list? () (list 1 2 . #0=(3 . #0#))))
($check-not-predicate (finite-list? () 1))
($check-not-predicate (finite-list? (list 1 2) . #0=(1 () . #0#)))

;; countable-list?
($check-predicate (countable-list? ()))
($check-predicate (countable-list? (list 1 2)))
($check-predicate (countable-list? (list 1 . #0=(2 . #0#))))
($check-predicate (countable-list? (list 1 2) (list 1 . #0=(2 . #0#)) ()))
($check-predicate (countable-list? 
                   () . #0=((list 1 . #1=(2 . #1#)) () . #0#)))

($check-not-predicate (countable-list? 1))
($check-not-predicate (countable-list? () 1))
($check-not-predicate (countable-list? (list 1 2) . #0=(#inert () . #0#)))
($check-not-predicate (countable-list? 
                       () . #0=((list 1 . #1=(2 . #1#)) 3 . #0#)))

;; reduce
($let ((ac-+ ($lambda ls (reduce ls + 0)))
       (c-+
        ;; the idea of the cycle treatment is to carry a flag indicating
        ;; if all elements so far in the cycle were actually zero, if so
        ;; the sum of the cycle is zero otherwise it can be undefined or
        ;; (* +infinity (acyclic-sum)) which in the integer case is +infinity 
        ;; or -infinity
        ($let ((precycle ($lambda (x)
                           (cons x (zero? x))))
               (incycle ($lambda ((x . x-zero?) (y . y-zero?))
                          (cons (+ x y) 
                                (and? x-zero? y-zero?))))
               (postcycle ($lambda ((result . all-zero?))
                            ($if all-zero?
                                 0
                                 (* #e+infinity result)))))
          ($lambda ls
            (reduce ls + 0 precycle incycle postcycle)))))
  ($check equal? (ac-+) 0)
  ($check equal? (ac-+ 1) 1)
  ($check equal? (ac-+ 1 2) 3)
  ($check equal? (ac-+ 1 2 3) 6)

  ($check equal? (c-+) 0)
  ($check equal? (c-+ 1) 1)
  ($check equal? (c-+ 1 2) 3)
  ($check equal? (c-+ 1 2 . #0=(0 0 . #0#)) 3)
  ($check equal? (c-+ 1 2 . #2=(-3 -4 . #2#)) #e-infinity))


;;;
;;; Error Checking and Robustness
;;;

;; null? & pair?
($check-error (null? () . #inert))
($check-error (pair? (cons () ()) . #inert))

;; list & list*
($check-error (list #inert . 1))
($check-error (list* . 1))
($check-error (list*))
($check-error (list* 1 #0=(2 3 . #0#)))

;; car, cdr & co
($check-error (car))
($check-error (cdr))
($check-error (car ()))
($check-error (cdr ()))
($check-error (car 1))
($check-error (cdr 1))
($check-error (car (cons 1 2) (cons 3 4)))
($check-error (cdr (cons 1 2) (cons 3 4)))

($let* ((tree2 (cons 1 2))
        (tree4 (cons tree2 (cons 3 4)))
        (tree8 (cons tree4 (cons (cons 5 6) (cons 7 8)))))
  ($check-error (caar tree2))
  ($check-error (cdar tree2))
  ($check-error (cadr tree2))
  ($check-error (cddr tree2))

  ($check-error (caaar tree4))
  ($check-error (cdaar tree4))
  ($check-error (cadar tree4))
  ($check-error (cddar tree4))
  ($check-error (caadr tree4))
  ($check-error (cdadr tree4))
  ($check-error (caddr tree4))
  ($check-error (cdddr tree4))

  ($check-error (caaaar tree8))
  ($check-error (cdaaar tree8))
  ($check-error (cadaar tree8))
  ($check-error (cddaar tree8))
  ($check-error (caadar tree8))
  ($check-error (cdadar tree8))
  ($check-error (caddar tree8))
  ($check-error (cdddar tree8))
  ($check-error (caaadr tree8))
  ($check-error (cdaadr tree8))
  ($check-error (cadadr tree8))
  ($check-error (cddadr tree8))
  ($check-error (caaddr tree8))
  ($check-error (cdaddr tree8))
  ($check-error (cadddr tree8))
  ($check-error (cddddr tree8)))

;; make-list
($check-error (make-list))
($check-error (make-list "str"))
($check-error (make-list 1 "str" "str2"))
($check-error (make-list -2))
($check-error (make-list 3/4))
($check-error (make-list #e+infinity))

;; list-copy
($check-error (list-copy))
($check-error (list-copy () ()))
($check-error (list-copy #inert))
($check-error (list-copy (list* 1 2 3)))

;; reverse
($check-error (reverse))
($check-error (reverse () ()))
($check-error (reverse #inert))
($check-error (reverse (list* 1 2 3)))
($check-error (reverse (list 1 . #1=(2 . #1#))))

;; get-list-metrics
($check-error (get-list-metrics))
($check-error (get-list-metrics () ()))

;; list-tail
($check-error (list-tail))
($check-error (list-tail (list 1 2 3)))
($check-error (list-tail (list 1 2 3) 3 4))
($check-error (list-tail (list 1 2 3) 4))
($check-error (list-tail (list 1 2 3) #e+infinity))
                                        ;($check-error (list-tail (list 1 2 3) 3.4))
($check-error (list-tail (list 1 2 3) -1))
($check-error (list-tail (list 1 2 3) #f))

;; length
($check-error (length))
($check-error (length () ()))
($check-error (length . #0=(() . #0#)))

;; list-ref
($check-error (list-ref))
($check-error (list-ref (list 1 2 3)))
($check-error (list-ref (list 1 2 3) 3 4))
($check-error (list-ref (list 1 2 3) 4))
($check-error (list-ref (list 1 2 3) #e+infinity))
                                        ;($check-error (list-ref (list 1 2 3) 3.4))
($check-error (list-ref (list 1 2 3) -1))
($check-error (list-ref (list 1 2 3) #f))

;; append
($check-error (append (list 1 2) (list . #0=(3 4 . #0#)) (list 5 6)))
($check-error (append (list . #0=(1 2 . #0#)) ()))
($check-error (append (list 1 2) 3 (list 4 5)))
($check-error (append (list 1 2) 3 ()))

;; list-neighbors
($check-error (list-neighbors))
($check-error (list-neighbors (list 1 2) (list 3 4)))
($check-error (list-neighbors 1))
($check-error (list-neighbors (list* 1 2 3 4)))

;; filter
($check-error (filter))
($check-error (filter number?))
($check-error (filter (list 1)))
($check-error (filter number? #inert))
($check-error (filter number? (list* 1 2 3)))
($check-error (filter number? (list 1 2 3) #inert))
($check-error (filter (unwrap number?) (list 1 2 3)))
($check-error (filter + (list 1 2 3)))
($check-error (filter car (list 1 2 3)))

;; asooc
($check-error (assoc))
($check-error (assoc 2))
($check-error (assoc 2 (list (list 1 1) (list 2 2)) () ()))
($check-error (assoc . #0=(2 (list (list 1 1) (list 2 2)) . #0#)))
($check-error (assoc 2 (list (list 1 1) (list 2 2)) () (unwrap equal?)))

($check-error (assoc 2 (list* (list 1 1) 2)))
($check-error (assoc 2 (list* (list 1 1) (list 2 2) #inert)))
($check-error (assoc 2 (list* 1 2) equal?))
($check-error (assoc 4 (list (list 1 1) (list 2 2) #inert (list 4 4))))
($check-error (assoc 2 (list (list 1 1) (list 2 2) #inert (list 4 4))))
($check-error (assoc 2 (list (list 1 1) (list 2 2) #inert (list 4 4)) 
                     equal?))

;; member?
($check-error (member?))
($check-error (member? 2))
($check-error (member? 2 (list 1 2) () ()))
($check-error (member? . #0=(2 (list 1 2) . #0#)))
($check-error (member? 2 (list 1 2) (unwrap equal?)))

($check-error (member? 2 (list* 1 2)))
($check-error (member? 2 (list* 1 2 3)))
($check-error (member? 2 (list* 1 2) equal?))

;; finite-list?
($check-error (countable-list? (cons () ()) . #inert))

;; countable-list?
($check-error (countable-list? (list . #0=(1 . #0#)) . #inert))

;; reduce
($check-error (reduce))
($check-error (reduce (list 1 2)))
($check-error (reduce (list 1 2) +))
($check-error (reduce #inert + 0))
($check-error (reduce (list 1 2) #inert 0))
($check-error (reduce (list 1 2 #0=(3 . #0#)) + 0))

($check-error (reduce (list 1 2 #0=(3 . #0#)) + 0 +))
($check-error (reduce (list 1 2 #0=(3 . #0#)) + 0 + +))
($check-error (reduce (list 1 2 #0=(3 . #0#)) + 0 + + + +))
($check-error (reduce (list 1 2 #0=(3 . #0#)) + 0 + + #inert))
($check-error (reduce (list 1 2 #0=(3 . #0#)) + 0 + #inert +))
($check-error (reduce (list 1 2 #0=(3 . #0#)) + 0 #inert + +))
