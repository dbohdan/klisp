;; check.k & test-helpers.k should be loaded

;;;
;;; Basic Functionality
;;;

;; keyword?
($check-predicate (applicative? keyword?))

($check-predicate (keyword?))
($check-predicate (keyword? #:s1))
($check-predicate (keyword? #:s1 #:s2 #:s3))
($check-predicate (keyword? #:s1 . #0=(#:s2 . #0#)))

($check-not-predicate (keyword? ()))
($check-not-predicate (keyword? (cons () ())))
($check-not-predicate (keyword? #ignore))
($check-not-predicate (keyword? (make-environment)))
($check-not-predicate (keyword? #inert))
($check-not-predicate (keyword? wrap))
($check-not-predicate (keyword? (call/cc ($lambda (c) c))))
($check-not-predicate (keyword? ($let (((enc . #ignore) 
                                        (make-encapsulation-type)))
                                  (enc #inert))))
($check-not-predicate (keyword? (memoize #inert)))
($check-not-predicate (keyword? 1))
($check-not-predicate (keyword? 1.0))
($check-not-predicate (keyword? #e+infinity))
($check-not-predicate (keyword? #i+infinity))
($check-not-predicate (keyword? #undefined))
($check-not-predicate (keyword? #real))
($check-not-predicate (keyword? "string"))
($check-not-predicate (keyword? #\a))
($check-not-predicate (keyword? (get-current-input-port))) 
($check-not-predicate (keyword? ((unwrap list) . symbol)))

;; keyword->string
($check-predicate (applicative? keyword->string))
($check equal? (keyword->string #:keyword) "keyword")

;; string->keyword
($check-predicate (applicative? string->keyword))
($check equal? (string->keyword "keyword") #:keyword)
($check equal? (keyword->string (string->keyword "keyword")) "keyword")

;; keyword->symbol
($check-predicate (applicative? keyword->symbol))
($check equal? (keyword->symbol #:keyword) ((unwrap list) . keyword))

;; symbol->keyword
($check-predicate (applicative? symbol->keyword))
($check equal? (symbol->keyword ((unwrap list) . keyword)) #:keyword)
($check equal? (keyword->symbol (symbol->keyword ((unwrap list) . keyword))) 
        ((unwrap list) . keyword))

;;;
;;; Eq?-ness & Equal?-ness
;;;
($check-predicate (eq? #:keyword #:keyword))
($check-predicate (equal? #:keyword #:keyword))