;; check.k & test-helpers.k should be loaded
;;
;; Tests of features related to keyed variables.
;;

;; 10.1.1 make-keyed-dynamic-variable

($check-error (make-keyed-dynamic-variable #f))

($let*
  ( ((b1 a1) (make-keyed-dynamic-variable))
    ((b2 a2) (make-keyed-dynamic-variable))
    (r1 ($lambda () (a1)))
    (r2 ($lambda () (a2))))
  ($check-predicate (applicative? b1))
  ($check-predicate (applicative? a1))
  ($check-error (b1 1 "not-a-combiner"))
  ($check-error (b1 1 ($lambda ()) "extra-argument"))
  ($check-error (b1 1))
  ($check-error (a1 "extra-argument"))

  ($check-not-predicate (equal? b1 b2))
  ($check-not-predicate (equal? a1 a2))
  ($check-predicate
    (b1 1 ($vau () denv (not? ($binds? denv +)))))
  ($check-not-predicate
    (b1 1 ($vau () e1 (b2 2 ($vau () e2 (equal? e1 e2))))))

  ($check equal? (b1 "value" ($lambda () "result")) "result")
  ($check equal? (b1 0 r1) 0)
  ($check equal? (b1 1 ($lambda () (b1 2 r1))) 2)
  ($check equal? (b1 1 ($lambda () (b2 2 r1))) 1)
  ($check equal? (b1 1 ($lambda () (b2 2 r2))) 2)

  ($check-error (a1))
  ($check-error (b1 0 r2)))

;; 11.1.1 make-keyed-static-variable

($check-error (make-keyed-static-variable #f))

($let*
  ( ((b1 a1) (make-keyed-static-variable))
    ((b2 a2) (make-keyed-static-variable))
    (e11 (b1 1 (get-current-environment)))
    (e12 (b1 2 (get-current-environment)))
    (e21 (b2 1 (get-current-environment)))
    (e22 (b2 2 (get-current-environment)))
    (e11* (b1 1 (get-current-environment)))
    (r11 (eval ($quote ($lambda (a) (a))) e11))
    (r12 (eval ($quote ($lambda (a) (a))) e12))
    (r11_13
      (eval
        ($quote
          ($let ((e13 (b1 3 (get-current-environment))))
            (eval ($quote ($lambda (a) (a))) e13)))
        e11))
    (r11_22
      (eval
        ($quote
          ($let ((e22 (b2 2 (get-current-environment))))
            (eval ($quote ($lambda (a) (a))) e22)))
        e11)))
  ($check-predicate (applicative? b1))
  ($check-predicate (applicative? a1))
  ($check-error (b1 1 "not-an-environment"))
  ($check-error (b1 1 (get-current-environment) "extra"))
  ($check-error (b1 1))
  ($check-error (a1 "extra-argument"))

  ($check-not-predicate (equal? b1 b2))
  ($check-not-predicate (equal? a1 a2))
  ($check-predicate (environment? e11))
  ($check-not-predicate (equal? e11 e12))
  ($check-not-predicate (equal? e11 e21))
  ($check-not-predicate (equal? e11 e11*))

  ($check equal? (eval (list a1) e11) 1)
  ($check equal? (eval (list a1) e12) 2)
  ($check equal? (r11 a1) 1)
  ($check equal? (r12 a1) 2)
  ($check equal? (r11_13 a1) 3)
  ($check equal? (r11_22 a1) 1)
  ($check equal? (r11_22 a2) 2)

  ($check-error (a1))
  ($check-error (r11_13 a2)))
