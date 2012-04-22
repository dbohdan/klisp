;; check.k & test-helpers.k should be loaded
;;
;; Tests of hash table features.
;;

;; XXX make-hash-table hash-table?

($check-predicate (applicative? hash-table? make-hash-table))
($check-predicate (hash-table?))
($check-predicate (hash-table? (make-hash-table)))

($check-not-predicate (hash-table? 0))
($check-not-predicate (hash-table? ""))
($check-not-predicate (hash-table? ()))
($check-not-predicate (hash-table? (make-bytevector 0)))
($check-not-predicate (hash-table? (make-vector 1)))
($check-not-predicate (hash-table? (make-environment)))

($check-error (make-hash-table eq?))
($check-error (make-hash-table 32))
($check-error (make-hash-table ($lambda (x) 1)))

;; XXX hash-table-set! hash-table-ref hash-table-exists? hash-table-delete!

($check-predicate
  (applicative? hash-table-set! hash-table-ref
                hash-table-exists? hash-table-delete!))

($check equal?
  ($let ((t (make-hash-table)))
    (hash-table-set! t 0 "a")
    (hash-table-set! t 1 "b")
    (hash-table-set! t -30 "c")
    (hash-table-set! t "x" "y")
    (hash-table-set! t #\u 15)
    (hash-table-set! t #:kwd 42)
    (hash-table-set! t #t #f)
    (hash-table-set! t #inert #\i)
    (list
      (map ($lambda (k) (hash-table-ref t k))
        (list -30 #:kwd 0 1 #t #\u #inert))
      (list
        (hash-table-exists? t 0 1 #t)
        (hash-table-exists? t)
        (hash-table-exists? t #inert #ignore))))
  (list
    (list "c" 42 "a" "b" #f 15 #\i)
    (list #t #t #f)))

($check equal?
  ($let ((t (make-hash-table)))
    (hash-table-set! t 42 "a")
    (hash-table-set! t 13 "b")
    (hash-table-set! t -5 "c")
    (hash-table-set! t 42 "d")
    (hash-table-set! t 13 "e")
    (list
      (hash-table-ref t 13)
      (hash-table-ref t 42)
      (hash-table-exists? t 0)
      (hash-table-exists? t -5)))
  (list "e" "d" #f #t))

($check equal?
  ($let ((t (make-hash-table)))
    (hash-table-set! t 42 "a")
    (hash-table-delete! t 13)
    (hash-table-delete! t 42 13)
    (hash-table-set! t 13 "c")
    (list (hash-table-ref t 13) (hash-table-exists? t 42)))
  (list "c" #f))

($check equal?
  ($let
    ((vx ($vau () denv ($binds? denv x)))
     (vy ($vau () denv ($binds? denv y))))
    ($let ((t (make-hash-table)) (x -5))
      (list
        (hash-table-ref t "a" ($lambda () "d"))
        (hash-table-ref t "b" vx)
        (hash-table-ref t "c" vy))))
  (list "d" #t #f))

($check-error (hash-table-ref (make-hash-table) 0))
($check-error
  ($let ((t (make-hash-table)))
    (hash-table-set! t "a" "b")
    (hash-table-ref t "b")))

($let ((t (make-hash-table)))
  (hash-table-set! t 3 "x")
  ($check-error (hash-table-ref))
  ($check-error (hash-table-ref () 2))
  ($check-error (hash-table-ref t))
  ($check-error (hash-table-ref t 2))
  ($check-no-error (hash-table-ref t 3))
  ($check-error (hash-table-ref t 3 5))
  ($check-error (apply hash-table-ref (list* t 3 ($lambda () ()))))
  ($check-error (hash-table-ref t 3 ($lambda () ()) 5))
  ($check-error (hash-table-set!))
  ($check-error (hash-table-set! t))
  ($check-error (hash-table-set! t 1))
  ($check-no-error (hash-table-set! t 1 2))
  ($check-error (hash-table-set! () 1 2))
  ($check-error (hash-table-set! t 1 3 4))
  ($check-error (hash-table-exists?))
  ($check-no-error (hash-table-exists? t))
  ($check-error (hash-table-exists? ()))
  ($check-error (hash-table-delete!))
  ($check-error (hash-table-delete! () 1 2 3))
  ($check-no-error (hash-table-delete! t)))

;; XXX hash-table-length

($check-predicate (applicative? hash-table-length))
($check equal? (hash-table-length (make-hash-table)) 0)
($check equal?
  ($let ((t (make-hash-table)))
    (hash-table-set! t "a" "b")
    (hash-table-set! t "c" "d")
    (hash-table-set! t "e" "f")
    (hash-table-delete! t "c")
    (hash-table-length t))
  2)
($check-error (hash-table-length))
($check-error (hash-table-length ()))
($check-error (apply hash-table-length 1))
($check-error (hash-table-length (make-hash-table) (make-hash-table)))

;; XXX hash-table

($check-predicate (applicative? hash-table))
($check-predicate (hash-table? (hash-table)))
($check equal?
  ($let ((t (hash-table 0 #f 1 #t)))
    (list
      (hash-table? t)
      (hash-table-length t)
      (hash-table-exists? t 0)
      (hash-table-exists? t 1)
      (hash-table-exists? t 2)
      (hash-table-ref t 0)
      (hash-table-ref t 1)))
  (list #t 2 #t #t #f #f #t))

($check-error (hash-table 1))
($check-error (hash-table 1 2 3))
($check-error (hash-table 1 2 3 4 5))
($check-error
  ($let ((ls (list 1 2 3 4 5 6)))
    (encycle! ls 3 3)
    (apply hash-table ls)))

;; XXX alist->hash-table

($check-predicate (applicative? alist->hash-table))
($check-predicate (hash-table? (alist->hash-table ())))
($check equal?
  ($let
    ((t
      (alist->hash-table
        ($quote (("x" . "y") (1 . 2) (3 . 4))))))
    (list
      (hash-table? t)
      (hash-table-length t)
      (hash-table-exists? t "x")
      (hash-table-exists? t "y")
      (hash-table-exists? t 1)
      (hash-table-exists? t 2)
      (hash-table-ref t 1)
      (hash-table-ref t 3)))
  (list #t 3 #t #f #t #f 2 4))

($check-no-error (alist->hash-table ()))
($check-error (alist->hash-table))
($check-error (alist->hash-table () ()))
($check-error ((unwrap alist->hash-table) 0))
($check-error (alist->hash-table 1))
($check-error (alist->hash-table (list 1 2)))
($check-error (alist->hash-table (list (cons 1 2) 3)))

;; XXX hash-table-keys hash-table-values hash-table->alist

($provide! (list-set-equal?)
  ($define! list-subset?
    ($lambda (a b)
      ($if (null? a)
        #t
        ($and? (member? (car a) b) (list-subset? (cdr a) b)))))
  ($define! list-set-equal?
    ($lambda (a b)
      ($and?
        (=? (length a) (length b))
        (list-subset? a b)
        (list-subset? b a)))))

($check-predicate
  (applicative? hash-table-keys hash-table-values hash-table->alist))

($check list-set-equal?
  (hash-table-keys (hash-table "a" 1 "b" 2 "c" 3))
  (list "a" "b" "c"))

($check list-set-equal?
  (hash-table-values (hash-table "a" 1 "b" 2 "c" 3))
  (list 1 2 3))

($check list-set-equal?
  (hash-table->alist (hash-table "a" 1 "b" 2 "c" 3))
  (list (cons "a" 1) (cons "b" 2) (cons "c" 3)))

($let ((t (hash-table 1 2 3 4)))
  ($check-error (hash-table-keys))
  ($check-error (hash-table-keys ()))
  ($check-error (hash-table-keys t t))
  ($check-error (hash-table-values))
  ($check-error (hash-table-values ()))
  ($check-error (hash-table-values t t))
  ($check-error (hash-table->alist))
  ($check-error (hash-table->alist ()))
  ($check-error (hash-table->alist t t)))

;; XXX hash-table-merge hash-table-copy hash-table-merge!

($check-predicate
  (applicative? hash-table-merge hash-table-copy hash-table-merge!))

($check equal?
  (hash-table->alist (hash-table-merge))
  ())

($check list-set-equal?
  (hash-table->alist
    (hash-table-merge
      (hash-table 1 "a" 2 "b")
      (hash-table 1 "c" 3 "d")
      (hash-table 1 "f" 5 "z")))
  (list (cons 1 "f") (cons 2 "b") (cons 3 "d") (cons 5 "z")))

($check list-set-equal?
  (hash-table->alist
    (hash-table-copy
      (hash-table 1 "a" 2 "b")))
  (list (cons 1 "a") (cons 2 "b")))

($check list-set-equal?
  ($let ((t (hash-table 1 "a" 2 "b")))
    (hash-table-merge! t (hash-table "x" "y" 2 "w"))
    (hash-table->alist t))
  (list
    (cons 1 "a") (cons 2 "w") (cons "x" "y")))

($check equal?
  ($let*
    ((t1 (hash-table 1 "a" 2 "b"))
     (t2 (hash-table 2 "c" 3 "d"))
     (t3 (hash-table-merge t1 t2))
     (t4 (hash-table-copy t2)))
    (list
      (eq? t1 t2) (eq? t1 t3) (eq? t1 t4)
      (eq? t2 t3) (eq? t2 t4)
      (eq? t3 t4)))
  (list #f #f #f #f #f #f))

($let*
  ((t (hash-table 1 2 3 4))
   (ls1 (list t))
   (ls2 (list t t)))
  (encycle! ls1 0 1)
  (encycle! ls2 1 1)
  ($check-no-error (hash-table-merge))
  ($check-no-error (hash-table-merge t))
  ($check-error (hash-table-merge t ()))
  ($check-error (hash-table-merge () t))
  ($check-no-error (hash-table-merge t t t t))
  ($check-no-error (apply hash-table-merge ls1))
  ($check-no-error (apply hash-table-merge ls2))
  ($check-error ((unwrap hash-table-merge) 1))
  ($check-error (hash-table-merge!))
  ($check-no-error (hash-table-merge! t))
  ($check-error (hash-table-merge! t ()))
  ($check-error (hash-table-merge! () t))
  ($check-no-error (hash-table-merge! t t t t))
  ($check-no-error (apply hash-table-merge! ls1))
  ($check-no-error (apply hash-table-merge! ls2))
  ($check-error ((unwrap hash-table-merge!) 1))
  ($check-error (hash-table-copy))
  ($check-no-error (hash-table-copy t))
  ($check-error (hash-table-copy t ()))
  ($check-error (hash-table-copy () t))
  ($check-no-error (apply hash-table-copy ls1))
  ($check-error (apply hash-table-copy ls2))
  ($check-error (hash-table-copy t t t t))
  ($check-error ((unwrap hash-table-copy) 1)))
