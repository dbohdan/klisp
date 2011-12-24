;; check.k & test-helpers.k should be loaded
;;
;; Tests of features related to continuations.
;;

;; R(-1)KR 7.2.1 continuation?

($check-predicate (applicative? continuation?))
($check-predicate (continuation?))
($check-predicate (continuation? root-continuation error-continuation))
($check-not-predicate (continuation? ($lambda () ())))
($check-not-predicate (continuation? ()))
($check-not-predicate (continuation? (get-current-environment)))

;; 7.2.2 call/cc

($check-predicate (applicative? call/cc))
($check equal? (call/cc ($lambda (c) (apply-continuation c 1))) 1)
($check-error (call/cc))
($check-error (call/cc 1))
($check-error (call/cc ($lambda (c) c) ($lambda (c) c)))

($check equal?
  (call/cc
    ($lambda (c)
      (list
        (continuation? c)
        (eq? c root-continuation)
        (eq? c error-continuation)
        (eq? c (call/cc ($lambda (c) c))))))
  (list #t #f #f #f))

($check equal?
  ($let
    ((b1 ($vau #ignore denv ($binds? denv the-var)))
     (b2 (wrap ($vau #ignore denv ($binds? denv the-var)))))
    (list
      (list (b1) (b2) (call/cc b1) (call/cc b2))
      ($let ((the-var 1))
        (list (b1) (b2) (call/cc b1) (call/cc b2)))))
  (list (list #f #f #f #f) (list #t #t #t #t)))

($check equal?
  (call/cc
    ($lambda (abort)
      ($let
        ((f ($lambda (k) ($when (=? k 2) (apply-continuation abort k)))))
        (f 1)
        (f 2)
        (f 3))))
  2)

($check equal?
  ($let ()
    ($define! r ())
    ($define! c (call/cc ($lambda (c) c)))
    ($set! (get-current-environment) r (cons (length r) r))
    ($if (<? (length r) 5)
      (apply-continuation c c)
      r))
  (list 4 3 2 1 0))

;; 7.2.3 extend-continuation

($check-predicate (applicative? extend-continuation))
($check-predicate
  (continuation?
    (extend-continuation root-continuation abs)
    (extend-continuation root-continuation abs (get-current-environment))))

($check-error (extend-continuation))
($check-error (extend-continuation root-continuation))
($check-error (extend-continuation root-continuation abs abs))
($check-error (extend-continuation root-continuation abs (get-current-environment) 123))
($check-error (extend-continuation abs root-continuation abs))

($check equal?
  (call/cc
    ($lambda (c)
      (apply-continuation (extend-continuation c abs) (list -10))))
  10)

($check equal?
  (call/cc
    ($lambda (c)
      (apply-continuation
        (extend-continuation c
          (wrap ($vau #ignore denv
            ($binds? denv c))))
        ())))
  #f)

($check equal?
  ($let
    ((comb
      (wrap
        ($vau x denv
          (string-append x ($remote-eval suffix denv))))))
    (call/cc
      ($lambda (k0)
        ($let*
          ((k1 (extend-continuation k0 comb
                 ($bindings->environment (suffix "a"))))
           (k2 (extend-continuation k1 comb
                 ($bindings->environment (suffix "b"))))
           (k3 (extend-continuation k2 comb
                 ($bindings->environment (suffix "c")))))
          (apply-continuation k3 "0")))))
  "0cba")

;; 7.2.4 guard-continuation

($check-predicate (applicative? guard-continuation))
($check-predicate (continuation? (guard-continuation () root-continuation  ())))
($check-error (guard-continuation))
($check-error (guard-continuation () root-continuation))
($check-error (guard-continuation () root-continuation () ()))
($check-error (guard-continuation ($lambda () ()) root-continuation))
($check-error (guard-continuation () ($lambda () ()) ()))
($check-error (guard-continuation () root-continuation ($lambda () ())))

($check equal?
  (call/cc
    ($lambda (c)
      (apply-continuation (guard-continuation () c ()) "arg")))
  "arg")

($check equal?
  (call/cc ($lambda (k1)
    (apply-continuation
      (guard-continuation
        (list
          (list error-continuation
            ($lambda (obj divert)
              (string-append "entry-1-" obj)))
          (list root-continuation
            ($lambda (obj divert)
              (string-append "entry-2-" obj)))
          (list root-continuation
            ($lambda (obj divert)
              (string-append "entry-3-" obj))))
        k1
        ())
      "arg")))
  "entry-2-arg")

($check equal?
  (call/cc ($lambda (k1)
    (apply-continuation
      (extend-continuation
        (guard-continuation
          (list
            (list root-continuation
              ($lambda (obj divert)
                (string-append "entry-" obj))))
          k1
          ())
        ($lambda arg
          (string-append "extension-" arg)))
      "arg")))
  "extension-entry-arg")

($check equal?
  (call/cc ($lambda (k1)
    (apply-continuation
      (extend-continuation
        (guard-continuation
          (list
            (list root-continuation
              ($lambda (obj divert)
                (apply divert "diverted"))))
          k1
          (list
            (list root-continuation
              ($lambda (obj divert)
                (apply divert "never")))))
        ($lambda arg "result"))
      "arg")))
  "diverted")

($check equal?
  (call/cc ($lambda (k1)
    (apply-continuation
      (extend-continuation
        (guard-continuation
          ()
          k1
          (list
            (list root-continuation
              ($lambda (obj divert)
                (string-append "exit-" obj)))))
          ($lambda arg
            (string-append "extension-" arg)))
      "arg")))
  "extension-arg")

($check equal?
  (call/cc ($lambda (k1)
    (apply-continuation
      (extend-continuation
        (guard-continuation
          ()
          k1
          (list
            (list root-continuation
              ($lambda (obj divert)
                (string-append "exit-" obj)))))
          ($lambda arg
            (apply-continuation k1 "result")))
      "arg")))
  "exit-result")

($check equal?
  (call/cc ($lambda (a)
    ($let*
      ((b1 (extend-continuation a ($lambda x (cons "b1" x))))
       (b2 (extend-continuation b1 ($lambda x (cons "b2" x))))
       (c1 (extend-continuation a ($lambda x (cons "c1" x))))
       (c2 (extend-continuation c1 ($lambda x (cons "c2" x)))))
      (apply-continuation
        (extend-continuation
          (guard-continuation
            ()
            b2
            (list
              (list b2 ($lambda (x divert) (apply divert (cons "catch-b2" x))))
              (list a ($lambda (x divert) (apply divert (cons "catch-a" x))))
              (list b1 ($lambda (x divert) (apply divert (cons "catch-b1" x))))))
          ($lambda arg
            (apply-continuation c1 (cons "body" arg))))
        (list "arg")))))
  (list "b1" "b2" "catch-a" "body" "arg"))

($check equal?
  (call/cc ($lambda (a)
    ($let*
      ((b1 (extend-continuation a ($lambda x (cons "b1" x))))
       (b2 (extend-continuation b1 ($lambda x (cons "b2" x))))
       (c1 (extend-continuation a ($lambda x (cons "c1" x))))
       (c2 (extend-continuation c1 ($lambda x (cons "c2" x)))))
      (apply-continuation
        (extend-continuation
          (guard-continuation
            ()
            b2
            (list
              (list b2 ($lambda (x divert) (cons "catch-b2" x)))
              (list a ($lambda (x divert) (cons "catch-a" x)))
              (list b1 ($lambda (x divert) (cons "catch-b1" x)))))
          ($lambda arg
            (apply-continuation c1 (cons "body" arg))))
        (list "arg")))))
  (list "c1" "catch-a" "body" "arg"))

;; 7.2.5 continuation->applicative

($check-predicate (applicative? continuation->applicative))
($check-predicate (applicative? (continuation->applicative root-continuation)))

($check-error (continuation->applicative))
($check-error (continuation->applicative ($lambda () ())))
($check-error (continuation->applicative root-continuation 0))

($check equal?
  (call/cc ($lambda (k1)
    (call/cc ($lambda (k2)
      (apply (continuation->applicative k1) "x")
      "y"))))
   "x")

;; 7.2.6 root-continuation
;; tested in test-interpreter.sh

;; 7.2.7 error-continuation
;; tested in error.k

;; 7.3.1 apply-continuation
;; sufficiently tested above

;; 7.3.2 $let/cc

($check-predicate (operative? $let/cc))
($check-error ($let/cc))
($check equal? ($let/cc sym) #inert)
($check-error ($let/cc 1 0))

($check equal?
  ($let/cc sym
    (list
      (continuation? sym)
      (eq? sym root-continuation)
      (eq? sym error-continuation)))
    (list #t #f #f))

($check equal?
  ($let/cc abort
    (apply-continuation abort "aborted")
    "not aborted")
  "aborted")

;; 7.3.3 guard-dynamic-extent

($check-predicate (applicative? guard-dynamic-extent))
($check equal? (guard-dynamic-extent () ($lambda x x) ()) ())
($check-error (guard-dynamic-extent))
($check-error (guard-dynamic-extent ($lambda x x) ($lambda x x) ()))
($check-error (guard-dynamic-extent () ($lambda x x) ($lambda x x)))
($check-error (guard-dynamic-extent () #t ()))

($check equal?
  ($let ((comb ($vau #ignore denv ($remote-eval var denv))))
    ($let ((var "v"))
      (guard-dynamic-extent () comb ())))
  "v")

($check equal?
  (guard-dynamic-extent
    ()
    ($lambda x x)
    (list
      (list root-continuation ($lambda (obj divert) "catch"))))
  ())

($check equal?
  (guard-dynamic-extent
    (list
      (list root-continuation
        ($lambda (obj divert) (apply divert "catch"))))
    ($lambda x x)
    ())
  ())

($check equal?
  (guard-dynamic-extent
    ()
    ($lambda #ignore (error "error"))
    (list
      (list error-continuation
        ($lambda (obj divert) (apply divert "catch")))))
  "catch")

($check equal?
  ($letrec ((r ()) (k #f) (env (get-current-environment)))
    ($set! env r
      (guard-dynamic-extent
        (list
          (list root-continuation
            ($lambda (obj divert) (apply divert (list* "catch" obj r)))))
        ($lambda ()
          ($let/cc k0
            ($set! env k k0)
            (cons "body" r)))
        ()))
    ($when (<? (length r) 5)
      (apply-continuation k "x"))
    r)
  (list "catch" "x" "catch" "x" "body"))

($check equal?
  ($letrec ((r ()) (k #f) (env (get-current-environment)))
    ($set! env r
      (guard-dynamic-extent
        (list
          (list root-continuation
            ($lambda (obj divert) (list* "catch" obj))))
        ($lambda ()
          (cons "next"
            ($let/cc k0
              ($set! env k k0)
              (cons "first" r))))
        ()))
    ($when (<? (length r) 8)
      (apply-continuation k (cons "redo" r)))
    r)
  (list "next" "catch" "redo" "next" "catch" "redo" "next" "first"))

;; 7.3.4 exit
;; effects tested in test-interpreter.sh
($check-predicate (applicative? exit))
($check-error (exit "too many" "args"))

