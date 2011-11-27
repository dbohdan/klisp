;; check.k & test-helpers.k should be loaded
;;
;; Tests of string features.
;;

;; XXX immutability of string constants
;; this works because this file is loaded and the strings
;; are immutable, but just reading the file wouldn't make them
;; immutable
($check-predicate (immutable-string? ""))
($check-predicate (immutable-string? "abcd"))

;; 13.?.? string?

($check-predicate (string?))
($check-predicate (string? "" "abcdef"))

($check-not-predicate (string? #\a))
($check-not-predicate (string? 0))
($check-not-predicate (string? #f))
($check-not-predicate (string? ()))
($check-not-predicate (string? #inert))

;; XXX string=? string<? string<=? string>? string>=?
;; XXX string-ci=? string-ci<? string-ci<=? string-ci>? string-ci>=?

($check-predicate (string=? "" ""))
($check-predicate (string=? "abcd" "abcd"))
($check-not-predicate (string=? "abcd" ""))
($check-not-predicate (string=? "abcd" "ABCD"))
($check-not-predicate (string=? "aa" "aaa"))

($check-predicate (string<? "" "a"))
($check-predicate (string<? "a" "b"))
($check-predicate (string<? "a" "ab"))
($check-predicate (string<? "A" "a"))
($check-not-predicate (string<? "a" ""))
($check-not-predicate (string<? "aaa" "a"))
($check-not-predicate (string<? "b" "a"))

($check-predicate ($true-for-all-combinations? string<=?
  ("" "A") ("a" "A" "ab")))

($check-predicate ($true-for-all-combinations? string>?
  ("b" "c") ("" "a")))

($check-predicate ($true-for-all-combinations? string>=?
  ("b" "c") ("" "a" "b")))

($check-predicate (string-ci=? "" ""))
($check-predicate (string-ci=? "abcd" "AbCd"))
($check-not-predicate (string-ci=? "abcd" ""))
($check-not-predicate (string=? "aa" "AAA"))

($check-predicate ($true-for-all-combinations? string-ci<?
  ("" "a" "A") ("ab" "AB" "b" "B")))
($check-predicate ($false-for-all-combinations? string-ci<?
  ("b" "B") ("" "a" "A" "aa" "b" "B" "ab" "aB" "Ab" "AB")))

($check-predicate ($true-for-all-combinations? string-ci<=?
  ("" "A" "a") ("a" "A" "ab")))

($check-predicate ($true-for-all-combinations? string-ci>?
  ("b" "B" "c" "C") ("" "a" "A")))

($check-predicate ($true-for-all-combinations? string-ci>=?
  ("b" "B" "c" "C") ("" "a" "A" "b" "B")))

;; XXX make-string

($check-predicate (string? (make-string 0)))
($check-predicate (string? (make-string 1)))
($check equal? (make-string 0) "")
($check equal? (make-string 0 #\a) "")
($check equal? (make-string 3 #\a) "aaa")
($check equal? (string-length (make-string 1000)) 1000)
($check equal? (string-length (make-string 1000 #\a)) 1000)
($check-error (make-string -1))
($check-error (make-string -1 #\a))

;; XXX string

($check-predicate (string? (string)))
($check-predicate (string? (string #\a #\b #\c)))
($check equal? (string) "")
($check equal? (string #\a #\b #\c) "abc")
($check-not-predicate ($let ((x (string #\a)) (y (string #\a))) (eq? x y)))

;; XXX string-upcase string-downcase string-titlecase string-foldcase
($check equal? (string-upcase "ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890abcdefghijklmnopqrstuvwxyz")
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ")
($check equal? (string-titlecase "this is a regular sentence. this 1 2!")
	 "This Is A Regular Sentence. This 1 2!")
($check equal? (string-downcase "ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890abcdefghijklmnopqrstuvwxyz")
	"abcdefghijklmnopqrstuvwxyz01234567890abcdefghijklmnopqrstuvwxyz")
($check equal? (string-foldcase "ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890abcdefghijklmnopqrstuvwxyz")
	"abcdefghijklmnopqrstuvwxyz01234567890abcdefghijklmnopqrstuvwxyz")
($check-predicate (mutable-string? (string-upcase (string-copy "A0a"))))
($check-predicate (mutable-string? (string-upcase "A0a")))
($check-predicate (mutable-string? (string-downcase (string-copy "A0a"))))
($check-predicate (mutable-string? (string-downcase "A0a")))
($check-predicate (mutable-string? (string-titlecase (string-copy "A0a"))))
($check-predicate (mutable-string? (string-titlecase "A0a")))
($check-predicate (mutable-string? (string-foldcase (string-copy "A0a"))))
($check-predicate (mutable-string? (string-foldcase "A0a")))

;; XXX string-length

($check equal? (string-length "") 0)
($check equal? (string-length "0123456789") 10)

;; XXX string-ref

($check equal? (string-ref "0123456789" 0) #\0)
($check equal? (string-ref "0123456789" 9) #\9)
($check-error (string-ref "0123456789" 10))
($check-error (string-ref "0123456789" -1))
($check-error (string-ref "" 0))

;; XXX string-set!

($check equal? ($let ((s (make-string 2 #\a))) (string-set! s 0 #\b) s) "ba")
($check equal? ($let ((s (make-string 2 #\a))) (string-set! s 1 #\b) s) "ab")
($check-error (string-set! (make-string 2) -1 #\a))
($check-error (string-set! (make-string 2) 3 #\a))
($check-error (string-set! "const" 3 #\a))

;; XXX string-fill!

($check equal? ($let ((s (make-string 3 #\a))) (string-fill! s #\b) s) "bbb")
($check-error (string-fill! "const" #\x))

;; Note: Empty string is always immutable. Therefore,
;; it is an error to call string-fill! on empty string.

($check-error (string-fill! (make-string 0) #\b))

;; XXX substring

($check equal? (substring "" 0 0) "")
($check equal? (substring "abcdef" 0 0) "")
($check equal? (substring "abcdef" 3 3) "")
($check equal? (substring "abcdef" 5 5) "")
($check equal? (substring "abcdef" 6 6) "")
($check equal? (substring "abcdef" 2 5) "cde")
($check equal? (substring "abcdef" 0 6) "abcdef")
($check-error (substring "abcdef" -1 0))
($check-error (substring "abcdef" 10 11))
($check-error (substring "abcdef" 3 10))
($check-error (substring "abcdef" 4 2))


;; immutable strings are eq? iff string=?
;; substring generates mutable strings
;; Andres Navarro
($check-predicate
  ($let* ((p "abc") (q (string->immutable-string (substring p 0 3))))
    (eq? p q)))

;; string-copy always generate mutable strings
;; Andres Navarro
($check-not-predicate
 ($let* ((p (string-copy "abc")) (q (substring p 0 3)))
   (eq? p q)))

;; substring always generate mutable strings
($check-predicate (immutable-string? (substring "abc" 0 0)))
($check-not-predicate (immutable-string? (substring "abc" 0 1)))

;; XXX string-append

($check equal? (string-append) "")
($check equal? (string-append "") "")
($check equal? (string-append "a") "a")
($check equal? (string-append "a" "b") "ab")
($check equal? (string-append "a" "b" "c") "abc")

($check-not-predicate
  ($let* ((p "abc") (q (string-append p)))
    (eq? p q)))

($check-predicate (nonempty-mutable-string? (string-append "a" "b")))

;; XXX string-copy

($check equal? (string-copy "") "")
($check equal? (string-copy "abcd") "abcd")

($check-not-predicate
  ($let* ((p "abc") (q (string-copy p)))
    (eq? p q)))

($check-predicate (nonempty-mutable-string? (string-copy "abc")))

;; XXX string->immutable-string

($check equal? (string->immutable-string "") "")
($check equal? (string->immutable-string "abcd") "abcd")

($check-not-predicate
  ($let* ((p "abc") (q (string-copy p)))
    (eq? p q)))

($check-predicate (immutable-string? (string->immutable-string "")))
($check-predicate (immutable-string? (string->immutable-string "abc")))
($check-predicate (immutable-string? (string->immutable-string (make-string 10))))

;; XXX string->list

($check equal? (string->list "") ())
($check equal? (string->list "abc") (list #\a #\b #\c))

;; XXX list->string

($check equal? (list->string ()) "")
($check equal? (list->string (list #\a #\b #\c)) "abc")

($check-not-predicate
  ($let*
    ( (cs (list #\a #\b #\c))
      (x (list->string cs))
      (y (list->string cs)))
  (eq? x y)))

($check-predicate (nonempty-mutable-string? (list->string (list #\a #\b))))

($check-error (list->string ($quote (#\a #0=(#\a . #0#)))))

;; XXX string->vector

($check equal? (string->vector "") (vector))
($check equal? (string->vector "abc") (vector #\a #\B #\c))

($check-not-predicate
  ($let*
    ( (str "abc")
      (x (string->vector str))
      (y (string->vector str)))
  (eq? x y)))

($check-predicate (mutable-vector? (string->vector "abc")))

;; XXX vector->string

($check equal? (vector->string (vector)) "")
($check equal? (vector->string (vector #\a #\b #\c)) "abc")

($check-not-predicate
  ($let*
    ( (cs (vector #\a #\b #\c))
      (x (vector->string cs))
      (y (vector->string cs)))
  (eq? x y)))

($check-predicate (mutable-string? (vector->string (vector #\a #\b))))

;; XXX string->bytevector

($check equal? (string->bytevector "") (bytevector))
($check equal? (string->bytevector "aBc") 
	(bytevector (char->integer #\a)
		    (char->integer #\B) 
		    (char->integer #\c)))

($check-not-predicate
  ($let*
    ( (str "abc")
      (x (string->bytevector str))
      (y (string->bytevector str)))
  (eq? x y)))

($check-predicate (mutable-bytevector? (string->bytevector "abc")))

;; XXX bytevector->string

($check equal? (bytevector->string (bytevector)) "")
($check equal? (bytevector->string (bytevector (char->integer #\a) 
					       (char->integer #\b) 
					       (char->integer #\c))) 
	"abc")

($check-not-predicate
  ($let*
      ((cs (bytevector (char->integer #\a) 
		       (char->integer #\b)
		       (char->integer #\c)))
      (x (bytevector->string cs))
      (y (bytevector->string cs)))
  (eq? x y)))

($check-predicate (mutable-string? 
		   (bytevector->string (bytevector (char->integer #\a)
						   (char->integer #\b)))))

;; 13.1.1 string->symbol
;; XXX symbol->string
;;

($check-predicate (symbol? (string->symbol "abcd")))
($check-predicate (symbol? (string->symbol "")))
($check-predicate (symbol? (string->symbol "0")))
($check-predicate (symbol? (string->symbol "#inert")))

($check equal? (string->symbol "abcd") ($quote abcd))
($check equal? (symbol->string ($quote abcd)) "abcd")

($check equal?
  ($quote sym)
  (string->symbol (symbol->string ($quote sym))))
