;; check.k & test-helpers.k should be loaded

;;;
;;; Basic Functionality
;;;

;; symbol?
($check-predicate (applicative? symbol?))

($let (($qs ($vau (s) #ignore s)))
  ($let ((s1 ($qs s1))
	 (s2 ($qs s2))
	 (s3 ($qs s3)))
    ($check-predicate (symbol?))
    ($check-predicate (symbol? s1))
    ($check-predicate (symbol? s1 s2 s3))
    ($check-predicate (symbol? s1 . #0=(s2 . #0#)))))

($check-not-predicate (symbol? ()))
($check-not-predicate (symbol? (cons () ())))
($check-not-predicate (symbol? #ignore))
($check-not-predicate (symbol? (make-environment)))
($check-not-predicate (symbol? #inert))
($check-not-predicate (symbol? wrap))
($check-not-predicate (symbol? (call/cc ($lambda (c) c))))
($check-not-predicate (symbol? ($let (((enc . #ignore) 
					(make-encapsulation-type)))
				  (enc #inert))))
($check-not-predicate (symbol? (memoize #inert)))
($check-not-predicate (symbol? 1))
;($check-not-predicate (symbol? 1.0))
($check-not-predicate (symbol? #e+infinity))
;($check-not-predicate (symbol? #i+infinity))
;($check-not-predicate (symbol? #undefined))
;($check-not-predicate (symbol? #real-with-no-primary-value))
($check-not-predicate (symbol? "string"))
($check-not-predicate (symbol? #\a))
($check-not-predicate (symbol? (get-current-input-port))) 
