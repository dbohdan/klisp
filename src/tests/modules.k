;; check.k & test-helpers.k should be loaded
;;
;; Tests of module system.
;;

;; N.B. Module registry is a hidden global variable. Module imports
;; work anywhere in environment or continuation hierarchy.
;;
;; ($fresh A1 A2 ..) evaluates A1 A2... in a fresh environment.
;;
($define! $fresh
  ($vau args #ignore
    (eval (list* $sequence args) (make-kernel-standard-environment))))

;; XXX $provide-module! $import-module!

($check-predicate (operative? $provide-module!))
($check-predicate (operative? $import-module!))

($check-no-error
  ($provide-module! (mod-a) (#:export p q)
    ($define! p 1)
    ($define! q 2)
    ($define! r 3)))

($check-error mod-a)
($check-error p)
($check-error q)
($check-error r)

($check-no-error
  ($provide-module! (mod-b 1 2 x) (#:export p u (#:rename v w))
    ($import-module! (#:only (mod-a) p))
    ($define! u 4)
    ($define! v 5)))

($check-error mod-b)
($check-error p)
($check-error u)

($check-no-error
  ($provide-module! (mod-c) (#:export)
    ($define! w 6)))

($check-no-error
  ($provide-module! (mod-d) (#:export p q)
    ($define! p 7)
    ($define! q 2)))

($check-error w)

($check-no-error ($provide-module! (mod-e) (#:export)))

($check-error ($provide-module! (mod-a) (#:export) 1))
($check-error ($provide-module! () (#:export)))
($check-error ($provide-module! (mod-q) (a) ($define! a 1)))
($check-error ($provide-module! (mod-q) (#:export #:a) ($define! a 1)))
($check-error ($provide-module! (mod-q) (#:export a a) ($define! a 1)))
($check-error ($provide-module! (mod-q) (#:export (a b)) ($define! a 1)))
($check-error ($provide-module! (mod-q) (#:export (#:re-na-me a b)) ($define! a 1)))
($check-error ($provide-module! (mod-q) (#:export (#:rename b a)) ($define! a 1)))
($check-error ($provide-module! (mod-q) (#:export (#:rename a)) ($define! a 1)))
($check-error ($provide-module! (mod-q) (#:export (#:rename a 2)) ($define! a 1)))
($check-error ($provide-module! (mod-q) 1))
($check-error ($provide-module! (mod-q) 1 (#:export)))
($check-not-predicate ($registered-module? (mod-q)))

($check equal? ($fresh ($import-module! (mod-a)) p) 1)
($check equal? ($fresh ($import-module! (mod-a)) q) 2)
($check-error ($fresh ($import-module! (mod-a)) r))

($check equal? ($fresh ($import-module! (#:only (mod-a) p)) p) 1)
($check equal? ($fresh ($import-module! (#:only (mod-a) q)) q) 2)
($check equal? ($fresh ($import-module! (#:only (mod-a) p q)) p) 1)
($check equal? ($fresh ($import-module! (#:only (mod-a) p q)) q) 2)
($check-error ($fresh ($import-module! (#:only (mod-a) p)) q))
($check-error ($fresh ($import-module! (#:only (mod-a) q)) p))
($check-error ($fresh ($import-module! (#:only (mod-a) r))))
($check-error ($fresh ($import-module! (#:only (mod-a) p p))))
($check-error ($fresh ($import-module! (#:only (mod-a) "p"))))
($check-error ($fresh ($import-module! (#:only (mod-a) #:p))))
($check-error ($import-module! (#:only (mod-a))))

($check equal? ($fresh ($import-module! (#:except (mod-a) p)) q) 2)
($check equal? ($fresh ($import-module! (#:except (mod-a) q)) p) 1)
($check-error ($fresh ($import-module! (#:except (mod-a) p q)) p))
($check-error ($fresh ($import-module! (#:except (mod-a) p)) r))
($check-error ($fresh ($import-module! (#:except (mod-a) r))))
($check-error ($fresh ($import-module! (#:except (mod-a) p p))))
($check-error ($fresh ($import-module! (#:except (mod-a) "p"))))
($check-error ($fresh ($import-module! (#:except (mod-a) #:p))))
($check-error ($import-module! (#:except (mod-a))))

($check equal? ($fresh ($import-module! (#:prefix (mod-a) a-)) a-p) 1)
($check equal? ($fresh ($import-module! (#:prefix (mod-a) a-)) a-q) 2)
($check-error ($fresh ($import-module! (#:prefix (mod-a) a-)) a-r))
($check-error ($fresh ($import-module! (#:prefix (mod-a) a-)) p))
($check-error ($fresh ($import-module! (#:prefix (mod-a) a-)) q))
($check-error ($fresh ($import-module! (#:prefix (mod-a) a- b-))))
($check-error ($fresh ($import-module! (#:prefix (mod-a) "a"))))
($check-error ($fresh ($import-module! (#:prefix (mod-a) #:a))))
($check-error ($fresh ($import-module! (#:prefix (mod-a)))))

($check equal? ($fresh ($import-module! (#:rename (mod-a) (p pp))) pp) 1)
($check equal? ($fresh ($import-module! (#:rename (mod-a) (p pp))) q) 2)
($check equal? ($fresh ($import-module! (#:rename (mod-a) (p q) (q p))) p) 2)
($check equal? ($fresh ($import-module! (#:rename (mod-a) (p q) (q p))) q) 1)
($check-error ($fresh ($import-module! (#:rename (mod-a) (p pp))) r))
($check-error ($fresh ($import-module! (#:rename (mod-a) (1 2)))))
($check-error ($fresh ($import-module! (#:rename (mod-a) p))))
($check-error ($fresh ($import-module! (#:rename (mod-a)))))

($check equal? ($fresh ($import-module! (#:only (mod-b 1 2 x) u)) u) 4)
($check equal? ($fresh ($import-module! (#:only (mod-b 1 2 x) p)) p) 1)
($check equal? ($fresh ($import-module! (#:only (mod-b 1 2 x) w)) w) 5)
($check-error ($fresh ($import-module! (#:only (mod-b 1 2 x) v))))

($check equal?
  ($fresh ($import-module! (#:only (mod-b 1 2 x) u) (#:only (mod-a) q)) q)
  2)

($check equal?
  ($fresh
    ($import-module!
      (#:rename (#:rename (#:rename (mod-a) (p p1)) (p1 p2)) (p2 p3)))
    p3)
  1)

($check equal?
  ($fresh
    ($import-module! (#:prefix (#:only (#:rename (mod-a) (q z)) z) p-))
    p-z)
  2)

($check equal?
  ($fresh
    ($import-module! (#:rename (#:except (mod-a) p) (q r)))
    r)
  2)

($check-error ($import-module! ((mod-a))))
($check-error ($import-module! (#:only ((mod-a)) a)))
($check-error ($import-module! (#:prefix bad prefix)))
($check-error ($import-module! (#:replace (mod-a))))

($check-error ($import-module! (mod-a) (mod-d)))
($check-no-error ($fresh ($import-module! (#:only (mod-a) p) (#:only (mod-d) q))))
($check-no-error ($fresh ($import-module! (#:only (mod-a) q) (#:only (mod-d) q))))

;; XXX module? make-module get-module-export-list get-module-environment

($check-predicate
  (applicative?
    module? make-module get-module-export-list get-module-environment))

($check-predicate (module?))
($check-not-predicate (module? ()))
($check-not-predicate (module? "x"))

($check-predicate
  (module?
    ($get-registered-module (mod-a))
    ($get-registered-module (mod-b 1 2 x))
    ($get-registered-module (mod-c))))

($check equal?
  (get-module-export-list ($get-registered-module (mod-a)))
  ($quote (p q)))
($check equal?
  (get-module-export-list ($get-registered-module (mod-b 1 2 x)))
  ($quote (p u w)))
($check equal?
  (get-module-export-list ($get-registered-module (mod-c)))
  ())

($check equal?
  ($let ((env (get-module-environment ($get-registered-module (mod-a)))))
    (list
      ($binds? env p)
      ($binds? env q)
      ($binds? env r)
      ($binds? env $lambda)))
  (list #t #t #f #f))

($check equal?
  ($let ((m (make-module ())))
    (list
      (module? m)
      (null? (get-module-export-list m))
      (environment? (get-module-environment m))))
    (list #t #t #t))

($check equal?
  ($let ((m (make-module (list (cons ($quote a) 1)))))
    (list
      (module? m)
      (get-module-export-list m)
      ($remote-eval a (get-module-environment m))))
    (list #t (list ($quote a)) 1))

($check-error
  (make-module
    (list
      (list ($quote a) 1)
      (list ($quote a) 2))))

;; XXX $registered-module?

($check-predicate (operative? $registered-module?))
($check-predicate ($registered-module? (mod-a)))
($check-predicate ($registered-module? (mod-b 1 2 x)))
($check-predicate ($registered-module? (mod-c)))
($check-predicate ($registered-module? (mod-d)))
($check-predicate ($registered-module? (mod-e)))
($check-not-predicate ($registered-module? (mod-f)))
($check-error ($registered-module? "abc"))
($check-error ($registered-module?))
($check-error ($registered-module? (mod-a) (mod-b)))

;; XXX $get-registered-module

($check-predicate (operative? $get-registered-module))
($check-no-error ($get-registered-module (mod-a)))
($check-error ($get-registered-module (mod-f)))
($check-error ($get-registered-module))
($check-error ($get-registered-module (mod-a) (mod-c)))

;; $register-module!

($check-predicate (operative? $register-module!))
($check-no-error
  ($register-module!
    (mod-z) (make-module (list (cons ($quote z) #:z)))))
($check equal?
  ($let ((m ($get-registered-module (mod-z))))
    (list
      (module? m)
      (get-module-export-list m)
      ($remote-eval z (get-module-environment m))))
    (list #t (list ($quote z)) #:z))
($check equal? ($fresh ($import-module! (mod-z)) z) #:z)

($check-error ($register-module! badname (make-module ())))
($check-error ($register-module! (mod-q) ()))
($check-not-predicate ($registered-module? (mod-q)))

;; XXX $unregister-module!

($check-predicate (operative? $unregister-module!))
($check-no-error ($unregister-module! (mod-z)))
($check-not-predicate ($registered-module? (mod-z)))
($check-predicate ($registered-module? (mod-b 1 2 x)))
($check-no-error ($unregister-module! (mod-b 1 2 x)))
($check-not-predicate ($registered-module? (mod-z)))
($check-not-predicate ($registered-module? (mod-b 1 2 x)))
($check-error ($unregister-module! (mod-nonexistent)))
($check-error ($unregister-module! badname))

;; cleanup - unregister remaining testing modules

($check-no-error ($unregister-module! (mod-a)))
($check-no-error ($unregister-module! (mod-c)))
($check-no-error ($unregister-module! (mod-d)))
($check-no-error ($unregister-module! (mod-e)))
