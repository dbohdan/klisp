;; check.k & test-helpers.k should be loaded
;;
;; Tests of i/o features.
;;
;; TODO: Make the test portable.
;; TODO: Delete temporary files.

;; Utilities for testing input and output features.
;;
;; temp-file .......... temporary file for input and output
;; test-input-file .... pre-existing file for input
;; nonexistent-file ... valid file name denoting non-existent file
;; invalid-file ....... invalid file name
;;
;; ($input-test INPUT PROGRAM) ... evaluates PROGRAM with current
;; input port initialized for reading from a temporary file
;; prepared according to INPUT. If INPUT is a string,
;; the contents of the file is the contents of the string.
;; Otherwise, empty file is prepared.
;;

($define! temp-file "/tmp/klisp-ports-test.txt")
($define! test-input-file "tests/ports.k")
($define! nonexistent-file "nonexistent-file.txt")
($define! invalid-file "!@#$%^&*/invalid/file/name.txt")

($define! prepare-input
  ($lambda (text)
    (with-output-to-file temp-file
      ($lambda () ($if (string? text) (display text) #inert)))))

($define! read-string-until-eof
  ($lambda ()
    ($letrec
      ( (loop ($lambda (prefix)
          ($let ((c (read-char)))
            ($if (eof-object? c)
              #inert
              ($sequence
                (set-cdr! prefix (cons c ()))
                (loop (cdr prefix)))))))
        (buf (cons () ())))
      (loop buf)
      (list->string (cdr buf)))))

($define! eval-with-input
  ($lambda (program denv)
    (with-input-from-file temp-file ($lambda () (eval program denv)))))

($define! eval-with-output
  ($lambda (program denv)
    (with-output-to-file temp-file ($lambda () (eval program denv)))))

($define! $input-test
  ($vau (input program) denv
    (prepare-input input)
    (eval-with-input program denv)))

($define! $output-test
  ($vau (program) denv
    (eval-with-output program denv)
    (with-input-from-file temp-file read-string-until-eof)))

;; 15.1.1 port?

($check-predicate (port? (get-current-input-port) (get-current-output-port)))
($check-predicate (port?))
($check-not-predicate (port? 0))
($check-not-predicate (port? #t))
($check-not-predicate (port? ()))
($check-not-predicate (port? #inert))

;; 15.1.2 input-port? output-port?

($check-predicate (input-port? (get-current-input-port)))
($check-predicate (input-port?))
($check-predicate (output-port? (get-current-output-port)))
($check-predicate (output-port?))

;; 15.1.3 with-input-from-file, with-output-to-file
;;
;; klisp documentation:
;;
;;   The result of the applicatives with-input-from-file
;;   and with-output-from-file is inert.
;;
;; R5RS:
;;
;;   With-input-from-file and with-output-to-file
;;   return(s) the value(s) yielded by thunk.
;;

($check equal? (with-input-from-file test-input-file ($lambda () 1)) 1)
($check-error (with-input-from-file nonexistent-file ($lambda () 1)))
($check-error (with-input-from-file invalid-file ($lambda () 1)))

($check equal? (with-output-to-file temp-file ($lambda () 1)) 1)
($check-error (with-output-to-file invalid-file ($lambda () 1)))

($check equal?
  ($let ((orig (get-current-input-port)))
    (with-input-from-file test-input-file
      ($lambda () (equal? orig (get-current-input-port)))))
  #f)

($check equal?
  ($let ((orig (get-current-output-port)))
    (with-output-to-file temp-file
      ($lambda () (equal? orig (get-current-output-port)))))
  #f)

;; 15.1.4 get-current-input-port? get-current-output-port?
;;   Functionality covered by other tests

;; 15.1.5 open-input-file open-output-file
;; 15.1.6 close-input-file close-output-file

($let ((p (open-input-file test-input-file)))
  ($check-predicate (port? p))
  ($check-predicate (input-port? p))
  ($check-not-predicate (equal? p (get-current-input-port)))
  ($check-not-predicate (equal? p (get-current-output-port)))
  (close-input-file p))

($let ((p (open-output-file temp-file)))
  ($check-predicate (port? p))
  ($check-predicate (output-port? p))
  ($check-not-predicate (equal? p (get-current-input-port)))
  ($check-not-predicate (equal? p (get-current-output-port)))
  (close-output-file p))

;; 15.1.7 read

($check-predicate (eof-object? ($input-test #inert (read))))
($check-predicate (eof-object? ($input-test "" (read))))

($check equal? ($input-test "#inert" (read)) #inert)
($check equal? ($input-test "(0 1 -1 #t #f #inert)" (read)) (list 0 1 -1 #t #f #inert))
($check equal? ($input-test "(1 2 (3 4 5) (6 . 7))" (read)) (list 1 2 (list 3 4 5) (list* 6 7)))

($check equal? ($input-test "1 2" (read)) 1)
($check equal? ($input-test "1 2" ($sequence (read) (read))) 2)
($check-predicate (eof-object? ($input-test "1 2" ($sequence (read) (read) (read)))))

;; 15.1.8 write

($check equal? ($output-test #inert) "")
($check equal? ($output-test (write 123)) "123")
($check equal? ($output-test (write (list 1 2 #t #f #inert ()))) "(1 2 #t #f #inert ())")
($check equal? ($output-test (write (list 1 2 (list 3 4 5) (list* 6 7)))) "(1 2 (3 4 5) (6 . 7))")

;; 15.2.1 call-with-input-file call-with-output-file
;; 15.2.2 load
;; 15.2.3 get-module
;; TODO

;; Additional input functions: read-char peek-char

($check-predicate (eof-object? ($input-test "" (read-char))))
($check-predicate (eof-object? ($input-test "" (peek-char))))

($check equal? ($input-test "a" (read-char)) #\a)
($check-predicate (eof-object? ($input-test "a" ($sequence (read-char) (read-char)))))
($check equal? ($input-test "a" (peek-char)) #\a)
($check equal? ($input-test "a" ($sequence (peek-char) (peek-char))) #\a)
($check equal? ($input-test "a" ($sequence (peek-char) (peek-char) (peek-char))) #\a)
($check equal? ($input-test "ab" ($sequence (read-char) (read-char))) #\b)
($check equal? ($input-test "ab" ($sequence (peek-char) (read-char))) #\a)

;; Additional input functions: char-ready?
;; TODO

;; Additional output functions: write-char newline display flush-ouput-port

($check equal? ($output-test (write-char #\a)) "a")
($check equal? ($output-test (newline)) (list->string (list #\newline)))
($check equal? ($output-test (display "abc")) "abc")

;; XXX flush-ouput-port is difficult to test...

;; File manipulation functions: file-exists? delete-file rename-file
;; TODO