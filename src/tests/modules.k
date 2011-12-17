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


;; **** SIGSEGV ****
#|
($check-no-error
  ($provide-module! (mod-c) (#:export)
    ($define! w 6)))
|#

($check-error w)

;;; **** FREEZE THE INTERPRETER ****
#|
($check equal? ($fresh ($import-module! (mod-a)) p) 1)
($check equal? ($fresh ($import-module! (mod-a)) q) 2)
|#

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
    #| ($get-registered-module  (mod-c)) |#))

($check equal?
  (get-module-export-list ($get-registered-module (mod-a)))
  ($quote (p q)))
($check equal?
  (get-module-export-list ($get-registered-module (mod-b 1 2 x)))
  ($quote (p u w)))

;($check equal?
;  (get-module-export-list ($get-registered-module (mod-c)))
;  ())

($check equal?
  ($let ((env (get-module-environment ($get-registered-module (mod-a)))))
    (list
      ($binds? env p)
      ($binds? env q)
      ($binds? env r)
      ($binds? env $lambda)))
  (list #t #t #f #f))

;; **** SIGSEGV ***
; ($check-predicate (module? (make-module ())))
; ....

;; XXX $registered-module? $get-registered-module
;; XXX $register-module $unregister-module
;; XXX $register-module
;; TODO

;; cleanup - unregister testing modules

($check-no-error ($unregister-module! (mod-a)))
($check-no-error ($unregister-module! (mod-b 1 2 x)))
;;($check-no-error ($unregister-module! (mod-c)))
