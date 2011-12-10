;; check.k & test-helpers.k should be loaded
;;
;; Tests of eq? and equal?.
;;
;; The form ($check-predicate ($let ... (equal? ...)))
;; is preferred over ($let ... ($check-predicate (equal? ...))),
;; because it prints more detailed error message.
;;

;;;
;;; Basic Functionality
;;;

($check-predicate (applicative? eq?))
($check-predicate (applicative? equal?))

;;
;; no arguments
;;
($check-predicate (eq?))
($check-predicate (equal?))

;;
;; 1 argument eq?
;;
($check-predicate (eq? ((unwrap list) . symbol)))
($check-predicate (eq? ()))
($check-predicate (eq? (cons () ())))
($check-predicate (eq? #ignore))
($check-predicate (eq? (make-environment)))
($check-predicate (eq? #inert))
($check-predicate (eq? $vau))
($check-predicate (eq? wrap))
($check-predicate (eq? (call/cc ($lambda (c) c))))
($check-predicate (eq? ($let (((enc . #ignore) 
					(make-encapsulation-type)))
				  (enc #inert))))
($check-predicate (eq? (memoize #inert)))
($check-predicate (eq? 1))
($check-predicate (eq? -1/2))
($check-predicate (eq? 1.0))
($check-predicate (eq? #e+infinity))
($check-predicate (eq? #i+infinity))
($check-predicate (eq? #undefined))
($check-predicate (eq? #real))
($check-predicate (eq? "string"))
($check-predicate (eq? #\a))
($check-predicate (eq? (get-current-input-port)))
($check-predicate (eq? (bytevector 1 2 3)))
($check-predicate (eq? (vector 1 2 3)))
($check-predicate (eq? #:keyword))

;;
;; 1 argument equal?
;;
($check-predicate (equal? ((unwrap list) . symbol)))
($check-predicate (equal? ()))
($check-predicate (equal? (cons () ())))
($check-predicate (equal? #ignore))
($check-predicate (equal? (make-environment)))
($check-predicate (equal? #inert))
($check-predicate (equal? $vau))
($check-predicate (equal? wrap))
($check-predicate (equal? (call/cc ($lambda (c) c))))
($check-predicate (equal? ($let (((enc . #ignore) 
					(make-encapsulation-type)))
				  (enc #inert))))
($check-predicate (equal? (memoize #inert)))
($check-predicate (equal? 1))
($check-predicate (equal? -1/2))
($check-predicate (equal? 1.0))
($check-predicate (equal? #e+infinity))
($check-predicate (equal? #i+infinity))
($check-predicate (equal? #undefined))
($check-predicate (equal? #real))
($check-predicate (equal? "string"))
($check-predicate (equal? #\a))
($check-predicate (equal? (get-current-input-port)))
($check-predicate (equal? (bytevector 1 2 3)))
($check-predicate (equal? (vector 1 2 3)))
($check-predicate (equal? #:keyword))

;;
;; two-argument eq?
;;
($check-predicate (eq? ((unwrap list) . symbol) ((unwrap list) . symbol)))
($check-predicate (eq? () ()))
($let ((p (cons () ())))
  ($check-predicate (eq? p p)))
($check-not-predicate (eq? (cons () ()) (cons () ())))
($check-predicate (eq? #ignore #ignore))
($let ((e (make-environment)))
  ($check-predicate (eq? e e)))
($check-not-predicate (eq? (make-environment) (make-environment)))
($check-predicate (eq? #inert #inert))
($check-predicate (eq? $vau $vau))
($check-predicate (eq? wrap wrap))
($let/cc c
  ($check-predicate (eq? c c)))
($let* (((enc . #ignore) 
	(make-encapsulation-type))
	(e (enc #inert)))
  ($check-predicate (eq? e e))
  ($check-not-predicate (eq? e (enc #inert))))
($let ((p (memoize #inert)))
  ($check-predicate (eq? p p))
  ($check-not-predicate (eq? p #inert)))
($check-predicate (eq? 1 1))
($check-predicate (eq? -1/2 -1/2))
($check-predicate (eq? 1.0 1.0))
($check-not-predicate (eq? 1 1.0))
($check-not-predicate (eq? 1/2 0.5))
($check-predicate (eq? #e+infinity #e+infinity))
($check-predicate (eq? #i+infinity #i+infinity))
($check-not-predicate (eq? #e+infinity #i+infinity))
($check-predicate (eq? #undefined #undefined))
($check-predicate (eq? #real #real))
($check-not-predicate (eq? #undefined #real))
($check-predicate ($let ((s "string")) (eq? s s)))
($check-not-predicate (eq? (string #\c) (string #\c)))
($check-predicate (eq? #\a #\a))
($check-predicate (eq? (get-current-input-port) (get-current-input-port)))
($check-predicate ($let ((v (vector 1 2))) (eq? v v)))
($check-predicate ($let ((v (bytevector 1 2))) (eq? v v)))

;;
;; two-argument equal? - opaque types
;;
($check-predicate (equal? () ()))
($check-predicate (equal? #ignore #ignore))
($check-predicate ($let ((e (make-environment))) (equal? e e)))
($check-not-predicate (equal? (make-environment) (make-environment)))
($check-predicate (equal? #inert #inert))
($check-predicate (equal? $vau $vau))
($check-predicate (equal? wrap wrap))
($check-predicate (equal? (get-current-input-port) (get-current-input-port)))
($check-predicate ($let/cc c (equal? c c)))

($let* (((enc . #ignore) 
        (make-encapsulation-type))
        (e (enc #inert)))
  ($check-predicate (equal? e e))
  ($check-not-predicate (equal? e (enc #inert))))
($let ((p (memoize #inert)))
  ($check-predicate (equal? p p))
  ($check-not-predicate (equal? p #inert)))

;;
;; two-argument equal? - number-like types
;;
($check-predicate (equal? 1 1))
($check-not-predicate (equal? 1 2))
($check-predicate (equal? -1/2 -1/2))
($check-not-predicate (equal? -1/2 1/2))
($check-predicate (equal? 1.0 1.0))
($check-not-predicate (equal? 1.0 2.0))
($check-not-predicate (equal? 1 1.0))
($check-not-predicate (equal? 1/2 0.5))
($check-predicate (equal? #e+infinity #e+infinity))
($check-predicate (equal? #i+infinity #i+infinity))
($check-not-predicate (equal? #e+infinity #i+infinity))
($check-predicate (equal? #undefined #undefined))
($check-predicate (equal? #real #real))
($check-not-predicate (equal? #undefined #real))
($check-predicate (equal? #\a #\a))
($check-not-predicate (equal? #\a #\b))

;;
;; two-argument equal? - string-like types
;;
($check-predicate ($let ((s ($quote symbol))) (equal? s s)))
($check-predicate (equal? ($quote symbol) ($quote symbol)))
($check-predicate (equal? ($quote symbol) ($quote SYMBOL)))
($check-not-predicate (equal? ($quote symbol) ($quote other-symbol)))

($check-predicate ($let ((s "string")) (equal? s s)))
($check-predicate (equal? "string" "string"))
($check-predicate (equal? (string #\c) (string #\c)))
($check-predicate (equal? "string" (string #\s #\t #\r #\i #\n #\g)))
($check-not-predicate (equal? "string" "another-string"))
($check-not-predicate (equal? "string" "str"))
($check-not-predicate (equal? "string" "STRING"))

($check-predicate ($let ((k #:keyword)) (equal? k k)))
($check-predicate (equal? #:keyword #:keyword))
($check-not-predicate (equal? #:keyword #:another-keyword))
($check-not-predicate (equal? #:keyword #:key))
($check-predicate (equal? #:keyword #:KEYWORD))

($check-predicate ($let ((v (bytevector 1 2 3))) (equal? v v)))
($check-predicate (equal? (bytevector 1 2) (bytevector 1 2)))
($check-not-predicate (equal? (bytevector 1 2) (bytevector 3 4)))
($check-not-predicate (equal? (bytevector 1) (bytevector)))

;;
;; two-argument equal? - lists and vectors
;;
($check-predicate (equal? (cons () ()) (cons () ())))
($check-predicate (equal? (list 1 2 3) (list 1 2 3)))
($check-not-predicate (equal? (list 1 2 3) (list 4 5 6)))
($check-not-predicate (equal? (list 1 2 3) (list 1 2)))
($check-not-predicate (equal? (list 1 2) (list 3 4)))

($check-predicate
  ($let ((p1 (list 1 2 1 2))
         (p2 (list 1 2)))
  (encycle! p1 2 2)
  (encycle! p2 0 2)
  (equal? p1 p2)))

($check-predicate
  ($let* ((L1 (list 1))
          (L2 (list L1))
          (L3 (list L1 L2)))
    (equal? L3 (list (list 1) (list (list 1))))))

($check-not-predicate
  ($let* ((L1 (list 1))
          (L2 (list L1))
          (L3 (list L1 L2)))
    (equal? L3 (list (list 1) (list (list 2))))))

($check-predicate
  ($let* ((a (cons #t 0))
          (b (cons #f 0))
          (c (cons #t 0))
          (d (cons #f 0)))
    (set-cdr! a b)
    (set-cdr! b c)
    (set-cdr! c d)
    (set-cdr! d a)
    (equal? a c)))

($check-not-predicate
  ($let* ((a (cons #t 0))
          (b (cons #f 0))
          (c (cons #t 0))
          (d (cons #f 0)))
    (set-cdr! a b)
    (set-cdr! b c)
    (set-cdr! c d)
    (set-cdr! d a)
    (equal? a b)))

($check-predicate
  ($let* ((a (list 1 5))
          (b (list a 5))
          (c (list b 5))
          (x (list 1 5)))
    (set-car! a c)
    (set-car! x x)
    (equal? a x)))

($check-not-predicate
  ($let* ((a (list 1 5))
          (b (list a 555))
          (c (list b 5))
          (x (list 1 5)))
    (set-car! a c)
    (set-car! x x)
    (equal? a x)))

($check-predicate ($let ((v (vector 1 2 3))) (equal? v v)))
($check-predicate (equal? (vector 1 2 3) (vector 1 2 3)))
($check-not-predicate (equal? (vector 1 2 3) (vector 4 5 6)))
($check-not-predicate (equal? (vector 1 2 3) (vector 1 2)))
($check-not-predicate (equal? (vector 1 2 3) (vector 2 3)))

($check equal?
  ($let ((v (vector 1 2)) (w (vector 1 3)))
    (list (equal? v w) (equal? v w) (equal? v w) (equal? v w)))
  (list #f #f #f #f))

($check-predicate
  ($let* ((a (make-vector 100 1))
          (b (make-vector 100 1))
          (v (make-vector 100 a))
          (w (make-vector 100 b)))
    (equal? v w)))

($check-not-predicate
  ($let* ((a (make-vector 100 1))
          (b (make-vector 100 1))
          (c (make-vector 100 1))
          (v (make-vector 100 a))
          (w (make-vector 100 b)))
    (vector-set! c 50 2)
    (vector-set! v 50 c)
    (equal? v w)))


($check-not-predicate
  ($let ((v (make-vector 100000 #f))
         (w (make-vector 100000 #f)))
      (vector-set! v 50000 #t)
      (equal? v w)))

($check-predicate
  ($let* ((v1 (vector 1))
          (v2 (vector 1 v1))
          (v3 (vector 1 v1 v2)))
    (equal?
      v3
      (vector 1 (vector 1) (vector 1 (vector 1))))))

($check-not-predicate
  ($let* ((v1 (vector 1))
          (v2 (vector 1 v1))
          (v3 (vector 1 v1 v2)))
    (equal?
      v3
      (vector 1 (vector 2) (vector 1 (vector 1))))))

($check-predicate
  ($let* ((a (vector 1 5))
          (b (vector a 5))
          (c (vector b 5))
          (x (vector 1 5)))
    (vector-set! a 0 c)
    (vector-set! x 0 x)
    (equal? a x)))

($check-not-predicate
  ($let* ((a (vector 1 5))
          (b (vector a 555))
          (c (vector b 5))
          (x (vector 1 5)))
    (vector-set! a 0 c)
    (vector-set! x 0 x)
    (equal? a x)))

($check-predicate
  ($let* ((a (list 0 0 0))
          (b (list 0 0 0))
          (c (list 0 0 0))
          (v (vector a b c))
          (w (vector b a c)))
    (set-car! a b)
    (set-car! b c)
    (set-car! c a)
    (equal? v w)))

($check-not-predicate
  ($let* ((a (list 0 0 1))
          (b (list 0 0 2))
          (c (list 0 0 3))
          (v (vector a b c))
          (w (vector b a c)))
    (set-car! a b)
    (set-car! b c)
    (set-car! c a)
    (equal? v w)))

;;
;; two-argument equal? - different argument types
;;

($check-not-predicate (equal? 0 ()))
($check-not-predicate (equal? 0 #f))
($check-not-predicate (equal? (vector) ()))
($check-not-predicate (equal? (vector 1 2 3) (list 1 2 3)))
($check-not-predicate (equal? (vector 1 2 3) (bytevector 1 2 3)))
($check-not-predicate (equal? (string #\a) (list #\a)))

;;
;; 3 or more arguments
;;
($check-predicate (eq? 1 1 1))
($check-not-predicate (eq? #t #t #f))
($check-predicate (eq? #t #t . #0=(#t . #0#)))
($check-not-predicate (eq? #t #t . #0=(#inert . #0#)))

($check-predicate (equal? 1 1 1))
($check-not-predicate (equal? #t #t #f))
($check-predicate (equal? #t #t . #0=(#t . #0#)))
($check-not-predicate (equal? #t #t . #0=(#inert . #0#)))

($let ((p1 (list 1 2 1 2))
       (p2 (list 1 2)))
  (encycle! p1 2 2)
  (encycle! p2 0 2)
  ($check-predicate (equal? p1 p2 . #0=(p2 p1 . #0#))))

;;;
;;; Error Checking and Robustness
;;;

;; boolean?
($check-error (eq? #t . #f))

($check-error (equal? #t . #f))
