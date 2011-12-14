;; check.k & test-helpers.k should be loaded
;;
;; Tests of vector (heterogenous array) features.
;;

;; (R7RS 3rd draft 6.3.6) vector?

($check-predicate (applicative? vector?))
($check-predicate (vector?))
($check-predicate (vector? (make-vector 0)))
($check-predicate (vector? (make-vector 1)))

($check-not-predicate (vector? 0))
($check-not-predicate (vector? ""))
($check-not-predicate (vector? ()))
($check-not-predicate (vector? (make-bytevector 0)))
($check-not-predicate (vector? (make-bytevector 1)))

;; XXX immutable-vector? mutable-vector?

($check-predicate (applicative? immutable-vector? mutable-vector?))

($check-predicate (immutable-vector?))
($check-predicate (immutable-vector? (make-vector 0)))
($check-not-predicate (immutable-vector? (make-vector 1)))
($check-not-predicate (immutable-vector? (make-bytevector 0)))

($check-predicate (mutable-vector?))
($check-predicate (mutable-vector? (make-vector 1)))
($check-not-predicate (mutable-vector? (make-vector 0)))
($check-not-predicate (mutable-vector? (make-bytevector 1)))

;; (R7RS 3rd draft, section 6.3.6) make-vector vector-length

($check-predicate (applicative? make-vector vector-length))
($check equal? (vector-length (make-vector 0)) 0)
($check equal? (vector-length (make-vector 0 "value")) 0)
($check equal? (vector-length (make-vector 1)) 1)
($check equal? (vector-length (make-vector 1 (list 1 2 3))) 1)
($check equal? (vector-length (make-vector 8192)) 8192)

;; (R7RS 3rd draft, section 6.3.6) vector

($check-predicate (applicative? vector?))
($check-predicate (vector? (vector)))
($check-predicate (immutable-vector? (vector)))
($check equal? (vector-length (vector)) 0)
($check-predicate (mutable-vector? (vector "x" "y")))
($check equal? (vector-length (vector "x" "y")) 2)

;; (R7RS 3rd draft, section 6.3.6) vector-ref

($check-predicate (applicative? vector-ref))
($check equal? (vector-ref (make-vector 10 #t) 1) #t)
($check equal? (vector-ref (make-vector 10 "abc") 5) "abc")
($check equal? (vector-ref (make-vector 10 1/2) 9) 1/2)
($check equal? (vector-ref (vector 1/2 1/3 1/4) 2) 1/4)

;; (R7RS 3rd draft, section 6.3.6) vector-set!
;; additional property: returns #inert
;; additional property: destination must be mutable

($check-predicate (applicative? vector-set!))

($let*
    ((v (make-vector 10))
     (w (vector->immutable-vector v)))
  ($check equal? (vector-set! v 0 1) #inert)
  ($check equal? (vector-ref v 0) 1)
  ($check equal? (vector-set! v 0 "abc") #inert)
  ($check equal? (vector-ref v 0) "abc")
  ($check equal? (vector-set! v 6 v) #inert)
  ($check equal? (vector-ref v 0) "abc")
  ($check eq? (vector-ref v 6) v)
  ($check-error (vector-ref v -1))
  ($check-error (vector-ref v 10))
  ($check-error (vector-ref v 12345))
  ($check-error (vector-set! v -1 0))
  ($check-error (vector-set! v 10 1/2))
  ($check-error (vector-set! w 0 #t)))

;; (R7RS 3rd draft, section 6.3.6) list->vector, vector->list

($check-predicate (applicative? list->vector))
($check-predicate (immutable-vector? (list->vector ())))
($check-predicate (mutable-vector? (list->vector (list "a" "b"))))

;; (R7RS 3rd draft, section 6.3.6) vector-copy
($check equal? (vector-copy (vector 1 2 3)) (vector 1 2 3))
($check equal? (vector-copy (vector (vector 1 2 3) (vector 4 5 6)))
        (vector (vector 1 2 3) (vector 4 5 6)))
($check-predicate (mutable-vector? (vector-copy (vector 1 2 3))))

($check-predicate
 (mutable-vector?
  (vector-copy (vector->immutable-vector (vector 1 2 3)))))

;; XXX bytevector->vector

($check equal? (bytevector->vector (u8)) (vector))
($check equal? (bytevector->vector (u8 0 1 2)) (vector 0 1 2))

($check-not-predicate
 ($let*
     ((bb (u8 0 1 2))
      (x (bytevector->vector bb))
      (y (bytevector->vector bb)))
   (eq? x y)))

($check-predicate (mutable-vector? (bytevector->vector (u8 0 1 2))))

;; XXX vector->bytevector

($check equal? (vector->bytevector (vector)) (u8))
($check equal? (vector->bytevector (vector 0 1 2)) (u8 0 1 2))

($check-not-predicate
 ($let*
     ((cs (vector 0 1 2))
      (x (vector->bytevector cs))
      (y (vector->bytevector cs)))
   (eq? x y)))

($check-predicate (mutable-bytevector? (vector->bytevector (vector 0 1))))


;; errors
($check-error (vector->bytevector (vector -1)))
($check-error (vector->bytevector (vector 256)))
($check-error (vector->bytevector (vector (integer->char 41))))

;; XXX vector-copy!
;; additional property: returns #inert
;; additional property: destination must be mutable
;;
($let ((v (make-vector 5 0)))
  ($check equal? (vector-copy! (vector 1 2 3 4 5) v) #inert)
  ($check equal? v (vector 1 2 3 4 5))
  ($check-no-error (vector-copy! (vector->immutable-vector (vector 9 9)) v))
  ($check equal? v (vector 9 9 3 4 5))
  ($check-error (vector-copy! (vector 1 2 3 4 5 6) v))
  ($check-error
   (vector-copy!
    (vector 1)
    (vector->immutable-vector (vector 1)))))

;; (R7RS 3rd draft, ) vector-copy-partial

($check equal? (vector-copy-partial (vector 1 2 3) 0 0) (vector))
($check equal? (vector-copy-partial (vector 1 2 3) 0 2) (vector 1 2))
($check equal? (vector-copy-partial (vector 1 2 3) 2 3) (vector 3))
($check equal? (vector-copy-partial (vector 1 2 3) 3 3) (vector))
($check-error (vector-copy-partial (vector 1 2 3) 2 4))
($check-error (vector-copy-partial (vector 1 2 3) -1 0))

;; R7RS 3rd draft, vector-copy-partial!
;; additional property: returns #inert
;; additional property: destination must be mutable
;;
($let*
    ((v (make-vector 5 9))
     (w (vector->immutable-vector v)))
  ($check equal? (vector-copy-partial! (vector 1 2) 0 2 v 0) #inert)
  ($check equal? v (vector 1 2 9 9 9))
  ($check equal? (vector-copy-partial! (vector 5 6) 1 2 v 4) #inert)
  ($check equal? v (vector 1 2 9 9 6))
  ($check-error (vector-copy-partial! (vector 1 2) 0 2 v -1))
  ($check-error (vector-copy-partial! (vector 1 2) 0 2 v 4))
  ($check-error (vector-copy-partial! (vector 1 2) 2 3 v 0))
  ($check-error (vector-copy-partial! (vector 1 2) -1 0 v 0))
  ($check-error (vector-copy-partial! (vector 1 2) 0 2 w 0)))


;; XXX vector-fill!
($check-predicate (inert? (vector-fill! (vector 1 2) 0)))
($check equal? ($let ((v (vector 1 2 3)))
                 (vector-fill! v "str")
                 v)
        (vector "str" "str" "str"))

;; XXX vector->immutable-vector

($check-predicate (applicative? vector->immutable-vector))

($check-predicate
 (immutable-vector? (vector->immutable-vector (vector 1 2))))
($check-not-predicate
 (mutable-vector? (vector->immutable-vector (vector 1 2))))
