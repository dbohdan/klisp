;; check.k & test-helpers.k should be loaded
;;
;; Tests of encapsulation features.
;;

;; 8.1.1 make-encapsulation-type

($let*
  ( ((e1 p1? d1) (make-encapsulation-type))
    ((e2 p2? d2) (make-encapsulation-type))
    (v1 "test")
    (v2 (list 1 2 3))
    (r11 (e1 v1))
    (r12 (e1 v2))
    (r21 (e2 v1))
    (r22 (e2 v2))
    (r11* (e1 v1)))

  ($check-not-predicate (equal? e1 e2))
  ($check-not-predicate (equal? p1? p2?))
  ($check-not-predicate (equal? d1 d2))

  ($check-not-predicate (p1? v1))
  ($check-not-predicate (p1? v2))
  ($check-not-predicate (p1? e1))
  ($check-not-predicate (p1? p1?))
  ($check-not-predicate (p1? d2))
  ($check-not-predicate (p1? 0))
  ($check-not-predicate (p1? #f))
  ($check-not-predicate (p1? #\x))
  ($check-not-predicate (p1? (make-encapsulation-type)))

  ($check-not-predicate (eq? r11 r12))
  ($check-not-predicate (eq? r11 r21))
  ($check-not-predicate (eq? r11 r22))
  ($check-not-predicate (eq? r11 r11*))

  ($check-not-predicate (equal? r11 r12))
  ($check-not-predicate (equal? r11 r21))
  ($check-not-predicate (equal? r11 r22))
  ($check-not-predicate (equal? r11 r11*))

  ($check-predicate (p1?))
  ($check-predicate (p1? r11))

  ($check-predicate (p1? r11 r12))
  ($check-predicate (p2? r21 r22))
  ($check-not-predicate (p1? r21))
  ($check-not-predicate (p2? r11))

  ($check eq? (d1 r11) v1)
  ($check eq? (d1 r12) v2)
  ($check eq? (d2 r21) v1)
  ($check eq? (d2 r22) v2)

  ($check-error (d1 r21))
  ($check-error (d1 r22))
  ($check-error (d2 r11))
  ($check-error (d2 r12)))
