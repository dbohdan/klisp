;; check.k & test-helpers.k should be loaded

;;;
;;; Basic Functionality
;;;

;; 4.8.1 environment?

($check-predicate (applicative? environment?))
($check-predicate (environment?))
($check-predicate (environment? (get-current-environment)))
($check-not-predicate (environment? ()))

;; 4.8.2 ignore?

($check-predicate (applicative? ignore?))
($check-predicate (ignore?))
($check-predicate (ignore? #ignore))
($check-not-predicate (ignore? #f))
($check-not-predicate (ignore? 0))
($check-not-predicate (ignore? ()))
($check-not-predicate (ignore? #inert))
($check-not-predicate (ignore? #undefined))

;; 4.8.3 eval

($check-predicate (applicative? eval))
($check-error (eval))
($check-error (eval 0))
($check-error (eval 0 1))
($check-error (eval 0 (get-current-environment) 2))

($let*
    ((env (make-environment))
     ((encapsulate #ignore #ignore) (make-encapsulation-type))
     (encapsulation (encapsulate 0))
     (promise ($lazy (+ 1 1)))
     (bytevector (make-bytevector 1)))
  ($check eq? (eval #t env) #t)
  ($check eq? (eval #inert env) #inert)
  ($check eq? (eval () env) ())
  ($check eq? (eval #ignore env) #ignore)
  ($check eq? (eval env env) env)
  ($check eq? (eval eval env) eval)
  ($check eq? (eval $vau env) $vau)
  ($check eq? (eval root-continuation env) root-continuation)
  ($check eq? (eval encapsulation env) encapsulation)
  ($check eq? (eval promise env) promise)
  ($check eq? (eval 0 env) 0)
  ($check eq? (eval "string" env) "string")
  ($check eq? (eval #\c env) #\c)
  ($check eq? (eval (get-current-input-port) env) (get-current-input-port))
  ($check eq? (eval bytevector env) bytevector)
  ($check-error (eval (string->symbol "eval") env))
  ($check eq? (eval (list $quote 1) env) 1)
  ($check equal? (eval (list + 1 1) env) 2)
  ($check-error (eval (list* not? #t) env))
  ($check-error (eval (list 1) env)))

($let ((env ($bindings->environment (+ *))))
  ($check equal? (eval ($quote (+ 1 1)) env) 1))

;; 4.8.4 make-environment

($check-predicate (applicative? make-environment))
($check-predicate (environment? (make-environment)))
($let*
    ((x 0)
     (e1 (make-environment))
     (e2 (make-environment (get-current-environment)))
     (e3 (make-environment e1))
     (e4 (make-environment e2))
     (es (list e1 e2 e3 e4)))
  ($check-not-predicate ($binds? e1 x))
  ($check-predicate ($binds? e2 x))
  ($check-not-predicate ($binds? e3 x))
  ($check-predicate ($binds? e4 x))
  (encycle! es 1 3)
  ($check-predicate ($binds? (apply make-environment es))))

($check-not-predicate (eq? (make-environment) (make-environment)))
($check-not-predicate (equal? (make-environment) (make-environment)))
($check-not-predicate (equal? (make-environment) (get-current-environment)))

;; 5.10.1 $let

($check-predicate (operative? $let))
($check equal? ($let () #t) #t)
($check-error ($let (sym) #inert))
($check-error ($let (sym 0) #inert))
($check-error ($let loop ((x 0)) #inert))
($check-error ($let ((sym 0 1)) #inert))

($check-predicate
 ($let
     ((a (and?
          (not? ($binds? (get-current-environment) a))
          (not? ($binds? (get-current-environment) b))))
      (b (and?
          (not? ($binds? (get-current-environment) a))
          (not? ($binds? (get-current-environment) b))))
      (f ($lambda ()
           (and?
            (not? ($binds? (get-current-environment) f))
            (not? ($binds? (get-current-environment) g)))))
      (g ($lambda ()
           (and?
            (not? ($binds? (get-current-environment) f))
            (not? ($binds? (get-current-environment) g))))))
   (and? a b (f) (g))))

;; 6.7.1 $binds?

($check-predicate (operative? $binds?))
($check-predicate ($binds? (make-environment)))

;; 6.7.2 get-current-environment

($check-predicate (applicative? get-current-environment))
($check-predicate (environment? (get-current-environment)))
($check-not-predicate ($binds? (get-current-environment) x))
($let ((x 0))
  ($check-predicate ($binds? (get-current-environment) x)))

;; 6.7.3 make-kernel-standard-environment

($check-predicate (applicative? make-kernel-standard-environment))

($let ((x 0))
  ($check-not-predicate
   ($binds? (make-kernel-standard-environment) x)))

;; symbols defined in the Kernel Report

($check-predicate
 ($binds? (make-kernel-standard-environment)
          ;; 4.1 - 4.10
          boolean?
          eq?
          equal?
          symbol?
          inert? $if
          pair? null? cons
          set-car! set-cdr! copy-es-immutable
          environment? ignore? eval make-environment
          $define!
          operative? applicative? $vau wrap unwrap
          ;; 5.1 - 5.10
          $sequence
          list list*
          $vau $lambda
          car cdr
          caar cadr cdar cddr
          caaar caadr cadar caddr cdaar cdadr cddar cdddr
          caaaar caaadr caadar caaddr cadaar cadadr caddar cadddr
          cdaaar cdaadr cdadar cdaddr cddaar cddadr cdddar cddddr
          apply
          $cond
          get-list-metrics list-tail
          encycle!
          map
          $let
          ;; 6.1 - 6.4, 6.7 - 6.9
          not? and? or? $and? $or?
          combiner?
          length list-ref append list-neighbors filter
          assoc member? finite-list? countable-list? reduce
          append! copy-es assq memq?
          $binds? get-current-environment make-kernel-standard-environment
          $let* $letrec $letrec* $let-redirect $let-safe $remote-eval
          $bindings->environment
          $set! $provide! $import!
          for-each
          ;; 7.1 - 7.3
          continuation? call/cc extend-continuation guard-continuation
          continuation->applicative root-continuation error-continuation
          apply-continuation $let/cc guard-dynamic-extent exit
          ;; 8.1
          make-encapsulation-type
          ;; 9.1
          promise? force $lazy memoize
          ;; 10.1
          make-keyed-dynamic-variable
          ;; 11.1
          make-keyed-static-variable
          ;; 12.1 - 12.10
          number? finite? integer?
          =? <? <=? >=? >?
          + * -
          zero?
          div mod div-and-mod
          div0 mod0 div0-and-mod0
          positive? negative?
          odd? even?
          abs
          max min
          lcm gcd
          exact? inexact? robust? undefined?
          get-real-internal-bounds get-real-exact-bounds
          get-real-internal-primary get-real-exact-primary
          make-inexact
          real->inexact real->exact
          with-strict-arithmetic get-strict-arithmetic?
          ;; not implemented: with-narrow-arithmetic get-narrow-arithmetic?
          rational?
          /
          numerator denominator
          floor ceiling truncate round
          rationalize simplest-rational
          real?
          exp log
          sin cos tan asin acos atan
          sqrt expt
          ;; not implemented: complex?
          ;; not implemented: make-rectangular real-part imag-part
          ;; not implemented: make-polar magnitude angle
          ;; 13.1
          string->symbol
          ;; 15.1 - 15.2
          port?
          input-port? output-port?
          with-input-from-file with-output-to-file
          get-current-input-port get-current-output-port
          open-input-file open-output-file
          close-input-file close-output-file
          read
          write
          call-with-input-file call-with-output-file
          load
          get-module))

;; Additional symbols defined in klisp.

($check-predicate
 ($binds? (make-kernel-standard-environment)
          ;; symbols
          symbol->string
          ;; strings
          string?
          symbol->string
          ;; TODO
          ;; chars
          char?
          char=? char<? char<=? char>=? char>?
          char->integer integer->char
          ;; TODO
          ;; ports
          textual-port? binary-port?
          with-error-to-file
          get-current-error-port
          open-binary-input-file open-binary-output-file
          close-input-port close-output-port close-port
          eof-object?
          read-char peek-char char-ready? write-char
          newline
          display
          read-u8 peek-u8 u8-ready? write-u8
          flush-output-port
          file-exists? delete-file rename-file
          ;; system functions
          current-second current-jiffy jiffies-per-second
          ;; bytevectors
          bytevector?
          ;; error handling
          error system-error-continuation))

;; 6.7.4 $let*

($check-predicate (operative? $let*))
($check equal? ($let* () #f) #f)
($check equal? ($let* () #f #t) #t)
($check-error ($let* (sym) #inert))
($check-error ($let* (sym 0) #inert))
($check-error ($let* loop ((x 0)) #inert))
($check-error ($let* ((sym 0 1)) #inert))

($check-predicate
 ($let*
     ((a (and?
          (not? ($binds? (get-current-environment) a))
          (not? ($binds? (get-current-environment) b))
          (not? ($binds? (get-current-environment) c))))
      (b (and?
          ($binds? (get-current-environment) a)
          (not? ($binds? (get-current-environment) b))
          (not? ($binds? (get-current-environment) c))))
      (c (and?
          ($binds? (get-current-environment) a)
          ($binds? (get-current-environment) b)
          (not? ($binds? (get-current-environment) c))))
      (f ($lambda ()
           (and?
            ($binds? (get-current-environment) a)
            ($binds? (get-current-environment) b)
            ($binds? (get-current-environment) c)
            (not? ($binds? (get-current-environment) f))
            (not? ($binds? (get-current-environment) g)))))
      (g ($lambda ()
           (and?
            ($binds? (get-current-environment) a)
            ($binds? (get-current-environment) b)
            ($binds? (get-current-environment) c)
            ($binds? (get-current-environment) f)
            (not? ($binds? (get-current-environment) g))))))
   (and? a b c (f) (g))))

;; 6.7.5 $letrec

($check-predicate (operative? $letrec))
($check-no-error ($letrec () #inert))

($check-predicate
 ($letrec ((x (not? ($binds? (get-current-environment) x)))) x))

($check-predicate
 ($letrec
     ((f ($lambda ()
           (and?
            ($binds? (get-current-environment) f)
            ($binds? (get-current-environment) g))))
      (g ($lambda ()
           (and?
            ($binds? (get-current-environment) f)
            ($binds? (get-current-environment) g)))))
   (and? (f) (g))))

;; 6.7.6 $letrec*

($check-predicate (operative? $letrec*))
($check equal? ($letrec* () 123) 123)

($check-predicate
 ($letrec* ((x (not? ($binds? (get-current-environment) x)))) x))

($check-predicate
 ($letrec*
     ((a 1)
      (f ($lambda ()
           (and?
            ($binds? (get-current-environment) a)
            ($binds? (get-current-environment) f)))))
   (f)))

($check-predicate
 ($letrec*
     ((f ($lambda ()
           ($binds? (get-current-environment) f)))
      (g ($lambda ()
           (and?
            ($binds? (get-current-environment) f)
            ($binds? (get-current-environment) g)))))
   (and? (f) (g))))

($check-predicate
 ($letrec*
     ((a 1)
      (b 2)
      (f ($lambda () ($binds? (get-current-environment) f))))
   (f)))

;; 6.7.7 $let-redirect

($check-predicate (operative? $let-redirect))
($check equal? ($let-redirect (make-environment) () 42) 42)

($let
    ((a 1)
     (env ($let ((a 2)) (get-current-environment))))
  ($check equal? ($let-redirect (get-current-environment) () a) 1)
  ($check equal? ($let-redirect env () a) 2)
  ($check equal? ($let-redirect env ((a 3)) a) 3)
  ($check equal? ($let-redirect env ((a a)) a) 1))

;; 6.7.8 $let-safe

($check-predicate (operative? $let-safe))
($check equal? ($let-safe () 42) 42)
($let
    (($lambda 42))
  ($check equal? ($let-safe ((x $lambda)) (($lambda () x))) 42)
  ($check-error ($let ((x $lambda)) (($lambda () x)))))

;; 6.7.9 $remote-eval

($check-predicate (operative? $remote-eval))
($check equal? ($remote-eval 42 (make-environment)) 42)

($let
    ((e0 (make-kernel-standard-environment))
     (e1 ($let ((or? not?)) (get-current-environment))))
  ($check equal? ($remote-eval (or? #t) e0) #t)
  ($check equal? ($remote-eval (or? #t) e1) #f))

;; 6.7.10 $bindings->environment

($check-predicate (operative? $bindings->environment))
($check-predicate (environment? ($bindings->environment)))
($let
    ((env ($bindings->environment (a 1) (b 2))))
  ($check-predicate ($binds? env a b))
  ($check equal? (eval ($quote a) env) 1)
  ($check equal? (eval ($quote b) env) 2))
