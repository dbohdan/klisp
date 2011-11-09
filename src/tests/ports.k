;; check.k & test-helpers.k should be loaded
;;
;; Tests of i/o features.
;;
;; Utilities for testing input and output features:
;;
;; temp-file .......... temporary file for input and output
;; temp-file-2 ........ second temporary file for input and output
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
;; ($output-test PROGRAM) ... evaluates PROGRAM with current
;; output port initialized for writing to a temporary file.
;; Returns the contents of the temporary file as string.
;;

;; Hope that the file names will work under both Unix and Windows.
($define! temp-file "klisp-ports-test-1.txt")
($define! temp-file-2 "klisp-ports-test-2.txt")
($define! test-input-file "tests/ports.k")
($define! nonexistent-file "nonexistent-file.txt")
($define! invalid-file "!@#$%^&*/invalid/file/name.txt")

($define! prepare-input
  ($lambda (text)
    (with-output-to-file temp-file
			 ($lambda () ($if (string? text) 
					  (display text) 
					  #inert)))))

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

($define! call-with-closed-input-port
  ($lambda (program)
    ($let ((port (open-input-file test-input-file)))
      (close-input-file port)
      (program port))))

($define! call-with-closed-output-port
  ($lambda (program)
    ($let ((port (open-output-file temp-file)))
      (close-output-file port)
      (program port))))

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

($check-no-error
 ($let ((p (open-input-file test-input-file)))
   ($check-predicate (port? p))
   ($check-predicate (input-port? p))
   ($check-not-predicate (equal? p (get-current-input-port)))
   ($check-not-predicate (equal? p (get-current-output-port)))
   (close-input-file p)
   (close-input-file p)))

($check-error (call-with-closed-output-port close-input-file))

($check-no-error  
 ($let ((p (open-output-file temp-file)))
   ($check-predicate (port? p))
   ($check-predicate (output-port? p))
   ($check-not-predicate (equal? p (get-current-input-port)))
   ($check-not-predicate (equal? p (get-current-output-port)))
   (close-output-file p)
   (close-output-file p)))

($check-error (call-with-closed-input-port close-output-file))

;; 15.1.7 read

($check-predicate (eof-object? ($input-test #inert (read))))
($check-predicate (eof-object? ($input-test "" (read))))

($check equal? ($input-test "#inert" (read)) #inert)
($check equal? ($input-test "(0 1 -1 #t #f #inert)" (read)) (list 0 1 -1 #t #f #inert))
($check equal? ($input-test "(1 2 (3 4 5) (6 . 7))" (read)) (list 1 2 (list 3 4 5) (list* 6 7)))

($check equal? ($input-test "1 2" (read)) 1)
($check equal? ($input-test "1 2" ($sequence (read) (read))) 2)
($check-predicate (eof-object? ($input-test "1 2" ($sequence (read) (read) (read)))))

($check-error ((read (get-current-output-port))))
($check-error (call-with-closed-input-port read))

;; 15.1.8 write

($check equal? ($output-test #inert) "")

($check equal?
  ($output-test (write (list 123 12345678901234567890 1/2 -3.14)))
  "(123 12345678901234567890 1/2 -3.14)")
($check equal?
  ($output-test (write (list #e+infinity #e-infinity #i+infinity #i-infinity #real #undefined)))
  "(#e+infinity #e-infinity #i+infinity #i-infinity #real #undefined)")
($check equal?
  ($output-test (write (list #\x #\newline #\space)))
  "(#\\x #\\newline #\\space)")
($check equal?
  ($output-test (write (list #t #f)))
  "(#t #f)")
($check equal?
  ($output-test (write (list #inert #ignore)))
  "(#inert #ignore)")

($check equal? ($output-test (write "")) "\"\"")
($check equal? ($output-test (write "a\\b\"")) "\"a\\\\b\\\"\"")

($check equal?
  ($output-test (write (list 1 2 (list 3 4 5) () (list* 6 7))))
  "(1 2 (3 4 5) () (6 . 7))")
($check equal?
  ($output-test (write ($quote #0=(1 2 #1=(3 4 . #0#) #2="abc" #3=(5 6 #1# #2# #3# . #0#)))))
  "#0=(1 2 #1=(3 4 . #0#) #2=\"abc\" #3=(5 6 #1# #2# #3# . #0#))")

($check-error (write 0 (get-current-input-port)))
($check-error (call-with-closed-output-port ($lambda (p) (write 0 p))))

;; 15.2.1 call-with-input-file call-with-output-file
;; 15.2.2 load
;; 15.2.3 get-module
;; TODO

;; Additional input functions: eof-object? read-char peek-char

($check-predicate ($false-for-all? eof-object?
  0 -1 #t #f () "" (get-current-input-port)))

($check-predicate (eof-object? ($input-test "" (read-char))))
($check-predicate (eof-object? ($input-test "" (peek-char))))

($check equal? ($input-test "a" (read-char)) #\a)
($check-predicate (eof-object? ($input-test "a" ($sequence (read-char) (read-char)))))
($check equal? ($input-test "a" (peek-char)) #\a)
($check equal? ($input-test "a" ($sequence (peek-char) (peek-char))) #\a)
($check equal? ($input-test "a" ($sequence (peek-char) (peek-char) (peek-char))) #\a)
($check equal? ($input-test "ab" ($sequence (read-char) (read-char))) #\b)
($check equal? ($input-test "ab" ($sequence (peek-char) (read-char))) #\a)

($check equal? ($input-test "a" (read-char (get-current-input-port))) #\a)
($check-error ((read-char (get-current-output-port))))
($check-error (call-with-closed-input-port read-char))

($check equal? ($input-test "a" (peek-char (get-current-input-port))) #\a)
($check-error ((peek-char (get-current-output-port))))
($check-error (call-with-closed-input-port peek-char))

;; Additional input functions: char-ready?
;; TODO

;; Additional output functions: write-char newline display flush-ouput-port

($check equal? ($output-test (write-char #\a)) "a")
($check equal? ($output-test (write-char #\a (get-current-output-port))) "a")
($check-error (write-char #\a (get-current-input-port)))
($check-error (call-with-closed-output-port ($lambda (p) (write-char #\a p))))

($check equal? ($output-test (newline)) (string #\newline))
($check equal? ($output-test (newline (get-current-output-port))) (string #\newline))
($check-error (newline (get-current-input-port)))
($check-error (call-with-closed-output-port newline))

($check equal? ($output-test (display "")) "")
($check equal? ($output-test (display "abc")) "abc")
($check equal? ($output-test (display "a\\b\"")) "a\\b\"")
($check equal? ($output-test (display #\x)) "x")
($check equal? ($output-test (display "abc" (get-current-output-port))) "abc")
($check-error ($output-test (display "abc" (get-current-input-port))))
($check-error (call-with-closed-output-port ($lambda (p) (display "abc" p))))

($check equal? ($output-test (flush-output-port)) "")
($check equal? ($output-test (flush-output-port (get-current-output-port))) "")
($check-error (flush-output-port (get-current-input-port)))
($check-error (call-with-closed-output-port flush-output-port))

;; Currently, write and write-char flush automatically
;; and flush-output-port causes no effect.
;;
;; ($define! colliding-output-test
;;   ($lambda (combiner)
;;     (call-with-output-file temp-file
;;       ($lambda (p1)
;;         (call-with-output-file temp-file
;;           ($lambda (p2)
;;             (combiner p1 p2)))))
;;     (with-input-from-file temp-file read-string-until-eof)))
;; 
;; ($check equal?
;;   (colliding-output-test ($lambda (p1 p2)
;;     (display "1" p1)
;;     (display "2" p2)
;;     (flush-output-port p1)
;;     (flush-output-port p2)))
;;   "12")
;; 
;; ($check equal?
;;   (colliding-output-test ($lambda (p1 p2)
;;     (display "1" p1)
;;     (display "2" p2)
;;     (flush-output-port p2)
;;     (flush-output-port p1)))
;;   "21")


;; File manipulation functions: file-exists? delete-file rename-file

($check-predicate (file-exists? test-input-file))
($check-not-predicate (file-exists? nonexistent-file))
($check-not-predicate (file-exists? invalid-file))

($check-no-error (prepare-input "test"))
($check-predicate (file-exists? temp-file))
($check-no-error (delete-file temp-file))
($check-not-predicate (file-exists? temp-file))
($check-error (delete-file nonexistent-file))
($check-error (delete-file invalid-file))

($check-no-error (prepare-input "test"))
($check-predicate (file-exists? temp-file))
($check-not-predicate (file-exists? temp-file-2))
($check-no-error (rename-file temp-file temp-file-2))
($check-predicate (file-exists? temp-file-2))
($check-not-predicate (file-exists? temp-file))
($check-no-error (delete-file temp-file-2))

($check-error (rename-file nonexistent-file temp-file))
($check-error (rename-file invalid-file temp-file))

;; Cleanup.
;; Check that temporary files were deleted.

($check-not-predicate (file-exists? temp-file))
($check-not-predicate (file-exists? temp-file-2))
