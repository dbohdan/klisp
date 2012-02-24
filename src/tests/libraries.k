;; check.k & test-helpers.k should be loaded
;;
;; Tests of library system.
;;

;; N.B. Library registry is a hidden global variable. Library imports
;; work anywhere in environment or continuation hierarchy.
;;
;; ($fresh A1 A2 ..) evaluates A1 A2... in a fresh environment.
;;
($define! $fresh
  ($vau args #ignore
    (eval (list* $sequence args) (make-kernel-standard-environment))))

;; XXX $provide-library! $import-library!

($check-predicate (operative? $provide-library!))
($check-predicate (operative? $import-library!))

($check-no-error
  ($provide-library! (mod-a) (#:export p q)
    ($define! p 1)
    ($define! q 2)
    ($define! r 3)))

($check-error mod-a)
($check-error p)
($check-error q)
($check-error r)

($check-no-error
  ($provide-library! (mod-b 1 2 x) (#:export p u (#:rename v w))
    ($import-library! (#:only (mod-a) p))
    ($define! u 4)
    ($define! v 5)))

($check-error mod-b)
($check-error p)
($check-error u)

($check-no-error
  ($provide-library! (mod-c) (#:export)
    ($define! w 6)))

($check-no-error
  ($provide-library! (mod-d) (#:export p q)
    ($define! p 7)
    ($define! q 2)))

($check-error w)

($check-no-error ($provide-library! (mod-e) (#:export)))

($check-error ($provide-library! (mod-a) (#:export) 1))
($check-error ($provide-library! () (#:export)))
($check-error ($provide-library! (mod-q) (a) ($define! a 1)))
($check-error ($provide-library! (mod-q) (#:export #:a) ($define! a 1)))
($check-error ($provide-library! (mod-q) (#:export a a) ($define! a 1)))
($check-error ($provide-library! (mod-q) (#:export (a b)) ($define! a 1)))
($check-error ($provide-library! (mod-q) (#:export (#:re-na-me a b)) ($define! a 1)))
($check-error ($provide-library! (mod-q) (#:export (#:rename b a)) ($define! a 1)))
($check-error ($provide-library! (mod-q) (#:export (#:rename a)) ($define! a 1)))
($check-error ($provide-library! (mod-q) (#:export (#:rename a 2)) ($define! a 1)))
($check-error ($provide-library! (mod-q) 1))
($check-error ($provide-library! (mod-q) 1 (#:export)))
($check-not-predicate ($registered-library? (mod-q)))

($check equal? ($fresh ($import-library! (mod-a)) p) 1)
($check equal? ($fresh ($import-library! (mod-a)) q) 2)
($check-error ($fresh ($import-library! (mod-a)) r))

($check equal? ($fresh ($import-library! (#:only (mod-a) p)) p) 1)
($check equal? ($fresh ($import-library! (#:only (mod-a) q)) q) 2)
($check equal? ($fresh ($import-library! (#:only (mod-a) p q)) p) 1)
($check equal? ($fresh ($import-library! (#:only (mod-a) p q)) q) 2)
($check-error ($fresh ($import-library! (#:only (mod-a) p)) q))
($check-error ($fresh ($import-library! (#:only (mod-a) q)) p))
($check-error ($fresh ($import-library! (#:only (mod-a) r))))
($check-error ($fresh ($import-library! (#:only (mod-a) p p))))
($check-error ($fresh ($import-library! (#:only (mod-a) "p"))))
($check-error ($fresh ($import-library! (#:only (mod-a) #:p))))
($check-error ($import-library! (#:only (mod-a))))

($check equal? ($fresh ($import-library! (#:except (mod-a) p)) q) 2)
($check equal? ($fresh ($import-library! (#:except (mod-a) q)) p) 1)
($check-error ($fresh ($import-library! (#:except (mod-a) p q)) p))
($check-error ($fresh ($import-library! (#:except (mod-a) p)) r))
($check-error ($fresh ($import-library! (#:except (mod-a) r))))
($check-error ($fresh ($import-library! (#:except (mod-a) p p))))
($check-error ($fresh ($import-library! (#:except (mod-a) "p"))))
($check-error ($fresh ($import-library! (#:except (mod-a) #:p))))
($check-error ($import-library! (#:except (mod-a))))

($check equal? ($fresh ($import-library! (#:prefix (mod-a) a-)) a-p) 1)
($check equal? ($fresh ($import-library! (#:prefix (mod-a) a-)) a-q) 2)
($check-error ($fresh ($import-library! (#:prefix (mod-a) a-)) a-r))
($check-error ($fresh ($import-library! (#:prefix (mod-a) a-)) p))
($check-error ($fresh ($import-library! (#:prefix (mod-a) a-)) q))
($check-error ($fresh ($import-library! (#:prefix (mod-a) a- b-))))
($check-error ($fresh ($import-library! (#:prefix (mod-a) "a"))))
($check-error ($fresh ($import-library! (#:prefix (mod-a) #:a))))
($check-error ($fresh ($import-library! (#:prefix (mod-a)))))

($check equal? ($fresh ($import-library! (#:rename (mod-a) (p pp))) pp) 1)
($check equal? ($fresh ($import-library! (#:rename (mod-a) (p pp))) q) 2)
($check equal? ($fresh ($import-library! (#:rename (mod-a) (p q) (q p))) p) 2)
($check equal? ($fresh ($import-library! (#:rename (mod-a) (p q) (q p))) q) 1)
($check-error ($fresh ($import-library! (#:rename (mod-a) (p pp))) r))
($check-error ($fresh ($import-library! (#:rename (mod-a) (1 2)))))
($check-error ($fresh ($import-library! (#:rename (mod-a) p))))
($check-error ($fresh ($import-library! (#:rename (mod-a)))))
($check-error ($fresh ($import-library! (#:rename (mod-a) (p q))) r))

($check equal? ($fresh ($import-library! (#:only (mod-b 1 2 x) u)) u) 4)
($check equal? ($fresh ($import-library! (#:only (mod-b 1 2 x) p)) p) 1)
($check equal? ($fresh ($import-library! (#:only (mod-b 1 2 x) w)) w) 5)
($check-error ($fresh ($import-library! (#:only (mod-b 1 2 x) v))))

($check equal?
  ($fresh ($import-library! (#:only (mod-b 1 2 x) u) (#:only (mod-a) q)) q)
  2)

($check equal?
  ($fresh
    ($import-library!
      (#:rename (#:rename (#:rename (mod-a) (p p1)) (p1 p2)) (p2 p3)))
    p3)
  1)

($check equal?
  ($fresh
    ($import-library! (#:prefix (#:only (#:rename (mod-a) (q z)) z) p-))
    p-z)
  2)

($check equal?
  ($fresh
    ($import-library! (#:rename (#:except (mod-a) p) (q r)))
    r)
  2)

($check-error ($import-library! ((mod-a))))
($check-error ($import-library! (#:only ((mod-a)) a)))
($check-error ($import-library! (#:prefix bad prefix)))
($check-error ($import-library! (#:replace (mod-a))))

($check-error ($import-library! (mod-a) (mod-d)))
($check-no-error ($fresh ($import-library! (#:only (mod-a) p) (#:only (mod-d) q))))
($check-no-error ($fresh ($import-library! (#:only (mod-a) q) (#:only (mod-d) q))))

;; XXX library? make-library get-library-export-list get-library-environment

($check-predicate
  (applicative?
    library? make-library get-library-export-list get-library-environment))

($check-predicate (library?))
($check-not-predicate (library? ()))
($check-not-predicate (library? "x"))

($check-predicate
  (library?
    ($get-registered-library (mod-a))
    ($get-registered-library (mod-b 1 2 x))
    ($get-registered-library (mod-c))))

($check equal?
  (get-library-export-list ($get-registered-library (mod-a)))
  ($quote (p q)))
($check equal?
  (get-library-export-list ($get-registered-library (mod-b 1 2 x)))
  ($quote (p u w)))
($check equal?
  (get-library-export-list ($get-registered-library (mod-c)))
  ())

($check equal?
  ($let ((env (get-library-environment ($get-registered-library (mod-a)))))
    (list
      ($binds? env p)
      ($binds? env q)
      ($binds? env r)
      ($binds? env $lambda)))
  (list #t #t #f #f))

($check equal?
  ($let ((m (make-library ())))
    (list
      (library? m)
      (null? (get-library-export-list m))
      (environment? (get-library-environment m))))
    (list #t #t #t))

($check equal?
  ($let ((m (make-library (list (cons ($quote a) 1)))))
    (list
      (library? m)
      (get-library-export-list m)
      ($remote-eval a (get-library-environment m))))
    (list #t (list ($quote a)) 1))

($check-error
  (make-library
    (list
      (list ($quote a) 1)
      (list ($quote a) 2))))

;; XXX $registered-library?

($check-predicate (operative? $registered-library?))
($check-predicate ($registered-library? (mod-a)))
($check-predicate ($registered-library? (mod-b 1 2 x)))
($check-predicate ($registered-library? (mod-c)))
($check-predicate ($registered-library? (mod-d)))
($check-predicate ($registered-library? (mod-e)))
($check-not-predicate ($registered-library? (mod-f)))
($check-error ($registered-library? "abc"))
($check-error ($registered-library?))
($check-error ($registered-library? (mod-a) (mod-b)))

;; XXX $get-registered-library

($check-predicate (operative? $get-registered-library))
($check-no-error ($get-registered-library (mod-a)))
($check-error ($get-registered-library (mod-f)))
($check-error ($get-registered-library))
($check-error ($get-registered-library (mod-a) (mod-c)))

;; $register-library!

($check-predicate (operative? $register-library!))
($check-no-error
  ($register-library!
    (mod-z) (make-library (list (cons ($quote z) #:z)))))
($check equal?
  ($let ((m ($get-registered-library (mod-z))))
    (list
      (library? m)
      (get-library-export-list m)
      ($remote-eval z (get-library-environment m))))
    (list #t (list ($quote z)) #:z))
($check equal? ($fresh ($import-library! (mod-z)) z) #:z)

($check-error ($register-library! badname (make-library ())))
($check-error ($register-library! (mod-q) ()))
($check-not-predicate ($registered-library? (mod-q)))

;; XXX $unregister-library!

($check-predicate (operative? $unregister-library!))
($check-no-error ($unregister-library! (mod-z)))
($check-not-predicate ($registered-library? (mod-z)))
($check-predicate ($registered-library? (mod-b 1 2 x)))
($check-no-error ($unregister-library! (mod-b 1 2 x)))
($check-not-predicate ($registered-library? (mod-z)))
($check-not-predicate ($registered-library? (mod-b 1 2 x)))
($check-error ($unregister-library! (mod-nonexistent)))
($check-error ($unregister-library! badname))

;; cleanup - unregister remaining testing libraries

($check-no-error ($unregister-library! (mod-a)))
($check-no-error ($unregister-library! (mod-c)))
($check-no-error ($unregister-library! (mod-d)))
($check-no-error ($unregister-library! (mod-e)))
