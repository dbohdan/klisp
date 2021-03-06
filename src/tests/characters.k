;; check.k & test-helpers.k should be loaded
;;
;; Tests of character features.
;;

;; 14.?.? char?

($check-predicate (char?))
($check-predicate (char? #\newline #\space #\0 #\A #\a #\~))

($check-not-predicate (char? ""))
($check-not-predicate (char? "a"))
($check-not-predicate (char? 0))
($check-not-predicate (char? #f))
($check-not-predicate (char? ()))
($check-not-predicate (char? #inert))

;; XXX char=? char<? char<=? char>? char>=?

($check-predicate (char=? #\A #\A))
($check-not-predicate (char=? #\A #\B))
($check-not-predicate (char=? #\a #\A))

($check-predicate (char<? #\A #\B))
($check-not-predicate (char<? #\A #\A))
($check-not-predicate (char<? #\B #\A))

($check-predicate (char<=? #\A #\A))
($check-predicate (char<=? #\A #\B))
($check-not-predicate (char<=? #\B #\A))

($check-predicate (char>? #\B #\A))
($check-not-predicate (char>? #\A #\A))
($check-not-predicate (char>? #\A #\B))

($check-predicate (char>=? #\A #\A))
($check-predicate (char>=? #\B #\A))
($check-not-predicate (char>=? #\A #\B))

;; XXX char-ci=? char-ci<? char-ci<=? char-ci>? char-ci>=?

($check-predicate ($true-for-all-combinations? char-ci=? (#\A #\a) (#\A #\a)))
($check-predicate ($false-for-all-combinations? char-ci=? (#\A #\a) (#\B #\b)))

($check-predicate ($true-for-all-combinations? char-ci<? (#\A #\a) (#\B #\b)))
($check-predicate ($false-for-all-combinations? char-ci<? (#\A #\a #\B #\b) (#\A #\a)))

($check-predicate ($true-for-all-combinations? char-ci<=? (#\A #\a) (#\A #\a #\B #\b)))
($check-predicate ($false-for-all-combinations? char-ci<=? (#\B #\b) (#\A #\a)))

($check-predicate ($true-for-all-combinations? char-ci>? (#\B #\b) (#\A #\a)))
($check-predicate ($false-for-all-combinations? char-ci>? (#\A #\a #\B #\b) (#\B #\b)))

($check-predicate ($true-for-all-combinations? char-ci>=? (#\A #\a #\B #\b) (#\A #\a)))
($check-predicate ($false-for-all-combinations? char-ci>=? (#\A #\a) (#\B #\b)))

;; XXX char-alphabetic? char-numeric? char-whitespace?

($check-predicate (char-alphabetic? #\a #\A #\b #\B #\k #\T #\y #\Y #\Z #\z))
($check-predicate ($false-for-all? char-alphabetic? #\newline #\0 #\` #\@ #\{ #\[ #\~))

($check-predicate (char-numeric? #\0 #\1 #\5 #\8 #\9))
($check-predicate ($false-for-all? char-numeric? #\space #\/ #\: #\A))

($check-predicate (char-whitespace? #\space #\newline))
($check-predicate ($false-for-all? char-whitespace? #\0 #\a #\A #\:))
                                        ; TODO ($check-predicate (char-whitespace? #\tab #\return ....))

;; XXX char-upper-case? char-lower-case?

($check-predicate (char-upper-case? #\A #\B #\R #\Y #\Z))
($check-predicate ($false-for-all? char-upper-case? #\0 #\a #\z #\' #\@ #\{ #\[ #\~))
($check-predicate (char-lower-case? #\a #\b #\j #\y #\z))
($check-predicate ($false-for-all? char-lower-case? #\0 #\A #\Z #\' #\@ #\{ #\[ #\~))

;; XXX char-upcase char-downcase char-titlecase char-foldcase
($check equal? (char-upcase #\a) #\A)
($check equal? (char-upcase #\z) #\Z)
($check equal? (char-upcase #\R) #\R)
($check equal? (char-upcase #\2) #\2)

($check equal? (char-downcase #\A) #\a)
($check equal? (char-downcase #\Z) #\z)
($check equal? (char-downcase #\r) #\r)
($check equal? (char-downcase #\9) #\9)

($check equal? (char-titlecase #\a) #\A)
($check equal? (char-titlecase #\z) #\Z)
($check equal? (char-titlecase #\R) #\R)
($check equal? (char-titlecase #\2) #\2)

($check equal? (char-foldcase #\A) #\a)
($check equal? (char-foldcase #\Z) #\z)
($check equal? (char-foldcase #\r) #\r)
($check equal? (char-foldcase #\9) #\9)

;; XXX char->integer integer->char

($check equal? (char->integer #\space) #x20)
($check equal? (char->integer #\0) #x30)
($check equal? (char->integer #\A) #x41)
($check equal? (char->integer #\a) #x61)

($check equal? (integer->char #x20) #\space)
($check equal? (integer->char #x30) #\0)
($check equal? (integer->char #x41) #\A)
($check equal? (integer->char #x61) #\a)

;; XXX char-digit?
($check-predicate (char-digit? #\0))
($check-predicate (char-digit? #\9))
($check-not-predicate (char-digit? #\a))
($check-not-predicate (char-digit? #\2 2))
($check-predicate (char-digit? #\f 16))
($check-predicate (char-digit? #\F 16))
($check-not-predicate (char-digit? #\!))

;; errors

($check-error (char-digit?))
($check-error (char-digit? 12))
($check-error (char-digit? #\9 10 #\a))
($check-error (char-digit? #\9 10 10))
($check-error (char-digit? #\0 1))
($check-error (char-digit? #\0 0))
($check-error (char-digit? #\0 -1))
($check-error (char-digit? #\0 37))

;; XXX char->digit
($check =? (char->digit #\0) 0)
($check =? (char->digit #\9) 9)
($check =? (char->digit #\f 16) 15)
($check =? (char->digit #\F 16) 15)
($check =? (char->digit #\z 36) 35)
($check =? (char->digit #\Z 36) 35)

;; errors
($check-error (char->digit))
($check-error (char->digit 0))
($check-error (char->digit #\0 10 10))
($check-error (char->digit #\0 1))
($check-error (char->digit #\0 37))
($check-error (char->digit #\0 0))
($check-error (char->digit #\0 -1))
($check-error (char->digit #\a 10))
($check-error (char->digit #\2 2))

;; XXX digit->char
($check char=? (digit->char 0) #\0)
($check char=? (digit->char 9) #\9)
($check char=? (char-downcase (digit->char 15 16)) #\f)
($check char=? (char-downcase (digit->char 35 36)) #\z)

;; errors
($check-error (digit->char))
($check-error (digit->char #\0))
($check-error (digit->char 0 10 10))
($check-error (digit->char 0 1))
($check-error (digit->char 0 37))
($check-error (digit->char 0 0))
($check-error (digit->char 0 -1))
($check-error (digit->char 10 10))
($check-error (digit->char 2 2))
