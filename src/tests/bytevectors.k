;; check.k & test-helpers.k should be loaded
;;
;; Tests of bytevector features.
;;

;; helper functions
;;
;; (list->bytevector INTEGERS) converts list of integers to bytevector
;; The elements of INTEGERS must be in the range 0...255.
;;
;; (u8 X_0 X_1 ... X_{N-1}) returns a bytevector B of length N,
;;   such that B[k] = X_k
;;
;; (u16 X_0 X_1 ... X_{N-1}) returns a bytevector B of length 2N,
;;   such that the bytes B[2k], B[2k+1], combined into 16-bit
;;   unsigned integer, represent the number X_k
;;
;; (u32 X_0 X_1 ... X_{N-1}) returns a bytevector of length 4N
;;   such that the bytes B[4k] ... B[4k+3], combined into 32-bit
;;   unsigned integer, represent the number X_k
;;
($define! list->bytevector
  ($lambda (bytes)
    ($let*
      ( (n (length bytes))
        (v (make-bytevector n)) )
      ($letrec
        ((loop ($lambda (i xs)
          ($if (<? i n)
            ($sequence
              (bytevector-u8-set! v i (car xs))
              (loop (+ i 1) (cdr xs)))
            #inert))))
        (loop 0 bytes)
        v))))

($define! u8
  ($lambda bytes (list->bytevector bytes)))

;; TODO: endianess
($define! u16
  ($let
    ((decompose ($lambda (w) (list (mod w 256) (div w 256)))))
    ($lambda words
      (list->bytevector (apply append (map decompose words))))))

;; TODO: endianess
($define! u32
  ($let
    ((decompose
      ($lambda (w)
        (list (mod w 256) (mod (div w 256) 256)
              (mod (div w 65536) 256) (div w 16777216)))))
    ($lambda words
      (list->bytevector (apply append (map decompose words))))))


;; XXX bytevector?

($check-predicate (bytevector?))
($check-predicate (bytevector? (make-bytevector 0)))
($check-predicate (bytevector? (make-bytevector 1)))

($check-not-predicate (bytevector? 0))
($check-not-predicate (bytevector? ""))
($check-not-predicate (bytevector? ()))

;; XXX immutable-bytevector? mutable-bytevector?

($check-predicate (immutable-bytevector?))
($check-predicate (immutable-bytevector? (make-bytevector 0)))

($check-predicate (mutable-bytevector?))
($check-predicate (mutable-bytevector? (make-bytevector 1)))

;; (R7RS 3rd draft, section 6.3.7) make-bytevector bytevector-length

($check equal? (bytevector-length (make-bytevector 0)) 0)
($check equal? (bytevector-length (make-bytevector 0 0)) 0)
($check equal? (bytevector-length (make-bytevector 1)) 1)
($check equal? (bytevector-length (make-bytevector 1 2)) 1)
($check equal? (bytevector-length (make-bytevector 8192)) 8192)

;; (R7RS 3rd draft, section 6.3.7) bytevector-u8-ref

($check equal? (bytevector-u8-ref (make-bytevector 10 0) 1) 0)
($check equal? (bytevector-u8-ref (make-bytevector 10 123) 5) 123)
($check equal? (bytevector-u8-ref (make-bytevector 10 255) 9) 255)

;; (R7RS 3rd draft, section 6.3.7) bytevector-u8-set!
;; additional property: returns #inert
;; additional property: destination must be mutable
;;
($let*
  ((v (make-bytevector 10))
   (w (bytevector->immutable-bytevector v)))
  ($check equal? (bytevector-u8-set! v 0 1) #inert)
  ($check equal? (bytevector-u8-ref v 0) 1)
  ($check equal? (bytevector-u8-set! v 0 32) #inert)
  ($check equal? (bytevector-u8-ref v 0) 32)
  ($check equal? (bytevector-u8-set! v 6 42) #inert)
  ($check equal? (bytevector-u8-ref v 0) 32)
  ($check equal? (bytevector-u8-ref v 6) 42)
  ($check-error (bytevector-u8-ref v -1))
  ($check-error (bytevector-u8-ref v 10))
  ($check-error (bytevector-u8-ref v 12345))
  ($check-error (bytevector-u8-set! v -1 0))
  ($check-error (bytevector-u8-set! v 10 255))
  ($check-error (bytevector-u8-set! v 5 -1))
  ($check-error (bytevector-u8-set! v 9 256))
  ($check-error (bytevector-u8-set! v 9 #\x))
  ($check-error (bytevector-u8-set! v 9 #f))
  ($check-error (bytevector-u8-set! w 0 0)))

;; (R7RS 3rd draft, section 6.3.7) bytevector-copy
;;
($check equal? (bytevector-copy (u8 1 2 3)) (u8 1 2 3))
($check-predicate (mutable-bytevector? (bytevector-copy (u8 1 2 3))))

($check-predicate
  (mutable-bytevector?
    (bytevector-copy (bytevector->immutable-bytevector (u8 1 2 3)))))

;; XXX bytevector-copy!
;; additional property: returns #inert
;; additional property: destination must be mutable
;;
($let ((v (make-bytevector 5 0)))
  ($check equal? (bytevector-copy! (u8 1 2 3 4 5) v) #inert)
  ($check equal? v (u8 1 2 3 4 5))
  ($check-no-error (bytevector-copy! (bytevector->immutable-bytevector (u8 9 9)) v))
  ($check equal? v (u8 9 9 3 4 5))
  ($check-error (bytevector-copy! (u8 1 2 3 4 5 6) v))
  ($check-error
    (bytevector-copy!
      (u8 1)
      (bytevector->immutable-bytevector (u8 1)))))

;; (R7RS 3rd draft, section 6.3.7) bytevector-copy-partial

($check equal? (bytevector-copy-partial (u8 1 2 3) 0 0) (u8))
($check equal? (bytevector-copy-partial (u8 1 2 3) 0 2) (u8 1 2))
($check equal? (bytevector-copy-partial (u8 1 2 3) 2 3) (u8 3))
($check equal? (bytevector-copy-partial (u8 1 2 3) 3 3) (u8))
($check-error (bytevector-copy-partial (u8 1 2 3) 2 4))
($check-error (bytevector-copy-partial (u8 1 2 3) -1 0))

;; R7RS 3rd draft, section 6.3.7) bytevector-copy-partial!
;; additional property: returns #inert
;; additional property: destination must be mutable
;;
($let*
  ((v (make-bytevector 5 9))
   (w (bytevector->immutable-bytevector v)))
  ($check equal? (bytevector-copy-partial! (u8 1 2) 0 2 v 0) #inert)
  ($check equal? v (u8 1 2 9 9 9))
  ($check equal? (bytevector-copy-partial! (u8 5 6) 1 2 v 4) #inert)
  ($check equal? v (u8 1 2 9 9 6))
  ($check-error (bytevector-copy-partial! (u8 1 2) 0 2 v -1))
  ($check-error (bytevector-copy-partial! (u8 1 2) 0 2 v 4))
  ($check-error (bytevector-copy-partial! (u8 1 2) 2 3 v 0))
  ($check-error (bytevector-copy-partial! (u8 1 2) -1 0 v 0))
  ($check-error (bytevector-copy-partial! (u8 1 2) 0 2 w 0)))

;; XXX bytevector-u8-fill!
($check-predicate (inert? (bytevector-u8-fill! (bytevector 1 2) 0)))
($check equal? ($let ((b (bytevector 1 2 3)))
		 (bytevector-u8-fill! b 0)
		 b)
	(bytevector 0 0 0))

;; XXX bytevector->immutable-bytevector

($check-predicate
  (immutable-bytevector? (bytevector->immutable-bytevector (u8 1 2))))
($check-not-predicate
  (mutable-bytevector? (bytevector->immutable-bytevector (u8 1 2))))
