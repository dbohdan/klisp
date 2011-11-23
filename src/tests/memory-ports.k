;; check.k & test-helpers.k should be loaded
;;
;; Tests of string and bytevector port features.
;;

;; (R7RS 3rd draft, section 6.7.1) open-input-string
;; TODO: char-ready?
;; TODO: unicode input
;; TODO: closing
;;
($let ((p (open-input-string "")))
  ($check-predicate (port? p))
  ($check-predicate (input-port? p))
  ($check-not-predicate (output-port? p))
  ($check-predicate (textual-port? p))
  ($check-not-predicate (binary-port? p))
  ($check-predicate (port-open? p))
  ($check-predicate (eof-object? (peek-char p)))
  ($check-predicate (eof-object? (read-char p))))

($let ((p (open-input-string "abc")))
  ($check equal? (read-char p) #\a)
  ($check equal? (peek-char p) #\b)
  ($check equal? (read-char p) #\b)
  ($check equal? (read-char p) #\c)
  ($check-predicate (eof-object? (read-char p))))

($let ((p (open-input-string "(1 2 #ignore) \"x\"")))
  ($check equal? (read p) (list 1 2 #ignore))
  ($check equal? (read p) "x")
  ($check-predicate (eof-object? (read p))))

;; (R7RS 3rd draft, section 6.7.1) open-output-string get-output-string
;; TODO: newline
;;
($let ((p (open-output-string)))
  ($check-predicate (port? p))
  ($check-predicate (output-port? p))
  ($check-not-predicate (input-port? p))
  ($check-predicate (textual-port? p))
  ($check-not-predicate (binary-port? p))
  ($check-predicate (port-open? p))
  ($check equal? (get-output-string p) "")
  ($check-no-error (write-char #\a p))
  ($check equal? (get-output-string p) "a")
  ($check-no-error (display "bc" p))
  ($check equal? (get-output-string p) "abc")
  ($check-no-error (write (list 1 "2" 3) p))
  ($check equal? (get-output-string p) "abc(1 \"2\" 3)"))

($check-error (get-output-string (get-current-input-port)))
($check-error (get-output-string (get-current-output-port)))

($let ((p (open-output-string)))
  ($check-no-error (display (make-string 100 #\a) p))
  ($check-no-error (display (make-string 1000 #\b) p))
  ($check-no-error (display (make-string 10000 #\c) p))
  ($check equal? (string-length (get-output-string p)) 11100)
  ($check equal? (string-ref (get-output-string p) 11001) #\c))

;; (R7RS 3rd draft, section 6.7.1) open-input-bytevector
;; TODO: u8-ready?
;; TODO: closing
;;
($let ((p (open-input-bytevector (make-bytevector 0))))
  ($check-predicate (port? p))
  ($check-predicate (input-port? p))
  ($check-not-predicate (output-port? p))
  ($check-predicate (binary-port? p))
  ($check-not-predicate (textual-port? p))
  ($check-predicate (eof-object? (peek-u8 p)))
  ($check-predicate (eof-object? (read-u8 p))))

($let*
  ((v (make-bytevector 3 0))
   (p ($sequence
        (bytevector-u8-set! v 0 2)
        (bytevector-u8-set! v 1 129)
        (open-input-bytevector v))))
  ($check equal? (read-u8 p) 2)
  ($check equal? (peek-u8 p) 129)
  ($check equal? (read-u8 p) 129)
  ($check equal? (read-u8 p) 0)
  ($check-predicate (eof-object? (read-u8 p))))

;; (R7RS 3rd draft, section 6.7.1) open-output-bytevector get-output-bytevector

($let ((p (open-output-bytevector)))
  ($check equal? (bytevector-length (get-output-bytevector p)) 0)
  ($check-no-error (write-u8 1 p))
  ($check equal? (bytevector-length (get-output-bytevector p)) 1)
  ($check-no-error (write-u8 10 p))
  ($check-no-error (write-u8 129 p))
  ($let ((v (get-output-bytevector p)))
    ($check equal? (bytevector-length v) 3)
    ($check equal? (bytevector-u8-ref v 0) 1)
    ($check equal? (bytevector-u8-ref v 1) 10)
    ($check equal? (bytevector-u8-ref v 2) 129)))
