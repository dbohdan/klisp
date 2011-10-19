;; check.k & test-helpers.k should be loaded
;;
;; Please look for the keyword FAIL in the source code.
;; The marked lines include
;;  - failing tests
;;  - tests corresponding to incorrect or unclear specification
;;
;; Other bugs:
;;
;;  - evaluating
;;      ($check equal? (round 1.1) 1)
;;      ($check equal? (round -1.1) -1)
;;    freezes the interpreter
;;

;; 12.4 External representation of numbers.
;;
;; check various formats against plain decimals
;;

($check equal? #d000 0)
($check equal? #d099 99)
($check equal? #d-099 -99)
($check equal? #d35/67 35/67)

($check equal? #x00 0)
($check equal? #x0FF 255)
($check equal? #x-0FF -255)
($check equal? #x-AB/CD -171/205)

($check equal? #b00000 0)
($check equal? #b01111 15)
($check equal? #b-01111 -15)
($check equal? #b#e101 5)

($check equal? #o0000 0)
($check equal? #o0777 511)
($check equal? #o-0777 -511)
($check equal? #e#o-16 -14)

($check equal? #e-infinity #e-infinity)
($check equal? #e+infinity #e+infinity)
($check equal? #i-infinity #i-infinity)
($check equal? #i+infinity #i+infinity)

;; 12.5.1 number? finite? integer?

($check-predicate (number? 0 1 3/5 -3.14e0 #real))
($check-not-predicate (number? 5 "6" 7))

($check-predicate (finite? 0 1/3 -99999999))
($check-not-predicate (finite? #e+infinity))
($check-not-predicate (finite? #e-infinity))

($check-error (finite? #real))
($check-error (finite? #undefined))

($check-predicate (integer? 0 8/2 -12/6 1.0 -1.25e2))
($check-not-predicate (integer? #e+infinity))
($check-not-predicate (integer? #e-infinity))
($check-not-predicate (integer? #real))
($check-not-predicate (integer? "0"))

;; 12.5.2 =?

($check-predicate (=?))
($check-predicate (=? -1))
($check-predicate (=? 0 0.0 0/1 -0.0 -0/1))
($check-predicate (=? #e+infinity #e+infinity))
($check-predicate (=? #e-infinity #e-infinity))
($check-predicate (=? . #0=(1 . #0#)))
($check-not-predicate (=? 0 1))
($check-not-predicate (=? 1 #e-infinity))
($check-not-predicate (=? #e+infinity #e-infinity))
($check-not-predicate (=? 2 5/2))
($check-not-predicate (=? . #0=(1 2 . #0#)))
($check-error (=? 0 #f))
($check-error (=? 1 #t))
($check-error (=? #real #real))
($check-error (=? 1 #real))
($check-error (=? #real -1/2))
($check-error (=? #real #e+infinity))

;; 12.5.3 <? <=? >=? >?

($check-predicate (<?))
($check-predicate (<? 1))
($check-predicate (<? 1 3 7 15))
($check-not-predicate (<? 1 7 3 7 15))
($check-predicate (<? #e-infinity -1 0 1 #e+infinity))

;; 12.5.4 +

($check equal? (+ 1 1) 2)
($check equal? (+) 0)
($check equal? (+ . #0=(0 . #0#)) 0)
($check equal? (+ . #0=(1 . #0#)) #e+infinity)
($check equal? (+ . #0=(-1 . #0#)) #e-infinity)
;---- ($check equal? (+ . #0=(1 -1 . #0#)) #real)       ; FAIL

;; 12.5.5 *

($check equal? (* 2 3) 6)
($check equal? (*) 1)
;---- ($check equal? (* 0 #e+infinity) #real)           ; FAIL
;---- ($check equal? (* 0 #e-infinity) #real)           ; FAIL
($check equal? (* . #0=(1 . #0#)) 1)
($check equal? (* . #0=(2 . #0#)) #e+infinity)
($check equal? (* . #0=(1/2 . #0#)) 0)
;---- ($check equal? (* . #0=(1/2 2 . #0#)) #real)      ; FAIL
;---- ($check equal? (* . #0=(-1 . #0#)) #real)         ; FAIL

;; 12.5.5 -

($check equal? (- 5 3) 2)
($check-error (-))
($check-error (- 0))

;; 12.5.7 zero?

($check-predicate (zero? 0 0/1 -0 -0/1 0.0 -0.0 #i0))
($check-not-predicate (zero? 1))
($check-not-predicate (zero? -0.0001))
($check-not-predicate (zero? #e+infinity))
($check-not-predicate (zero? #e-infinity))
($check-error (zero? #real))
($check-error (zero? #undefined))

;; 12.5.8 div, mod, div-and-mod

($check equal? (div 10 2) 5)
($check equal? (div -10 2) -5)
($check equal? (div 10 -2) -5)
($check equal? (div -10 -2) 5)

($check equal? (div 10 7) 1)
($check equal? (div -10 7) -2)

;; (div real1 real2) ... Let n be the greatest integer such that
;; real2 * n <= real1. Applicative div returns n.
;;
;; If real2 is negative, then such integer n does not exist.
;; interpretation : result shall be #undefined
;;
;--- ($check equal? (div 10 -7) #undefined)           ; FAIL
;--- ($check equal? (div -10 -7) #undefined)          ; FAIL

($check equal? (mod 10 7) 3)
($check equal? (div-and-mod 10 7) (list 1 3))

;; 12.5.9 div0, mod0, div-and-mod0
;; Test cases from R6RS. The commented test cases
;; contradict the KernelReport.

($check equal? (div-and-mod 123 10) (list 12 3))
;----- ($check equal? (div-and-mod 123 -10) (list -12 3))
($check equal? (div-and-mod -123 10) (list -13 7))
;----- ($check equal? (div-and-mod -123 -10) (list 13 7))
($check equal? (div0-and-mod0 123 10) (list 12 3))
;----- ($check equal? (div0-and-mod0 123 -10) (list -12 3))
($check equal? (div0-and-mod0 -123 10) (list -12 -3))
;----- ($check equal? (div0-and-mod0 -123 -10) (list 12 -3))

;; 12.5.10 positive? negative?

($check-predicate (positive? 1 1.0 1/1 999999999999 #e+infinity))
($check-not-predicate (positive? 0))                          ; FAIL
($check-not-predicate (positive? #e-infinity))                ; FAIL
($check-error (positive? #real))
($check-error (positive? #undefined))

($check-predicate (negative? -1 -1.0 -1/1 -999999999999 #e-infinity)) ; FAIL
($check-not-predicate (negative? 0))
($check-not-predicate (negative? #e+infinity))
($check-error (negative? #real))
($check-error (negative? #undefined))

;; 12.5.11 even? odd?

($check-predicate (even? 0 2 -2 4/2 9999999999998))
($check-error (even? #e+infinity))
($check-error (even? #e-infinity))

($check-predicate (odd? 1 -1 6/2 9999999999999))
($check-error (odd? #e+infinity))
($check-error (odd? #e-infinity))

;; 12.5.12 abs

($check equal? (abs 0) 0)
($check equal? (abs 1) 1)
($check equal? (abs -1) 1)
($check equal? (abs #e+infinity) #e+infinity)
($check equal? (abs #e-infinity) #e+infinity)

;; 12.5.12 max min

($check equal? (max) #e-infinity)
($check equal? (max 1 2 3 4) 4)
($check equal? (max #e-infinity #e+infinity) #e+infinity)

($check equal? (min) #e+infinity)
($check equal? (min 1 2 3 4) 1)
($check equal? (min #e-infinity #e+infinity) #e-infinity)

;; 12.5.12 lcm gcd
;; TODO

;; 12.6.1 exact? inexact? robust? undefined?

($check-predicate (exact? 0 1 -1 1/2 999999999999 #e-infinity))
($check-not-predicate (exact? 3.14))
($check-not-predicate (exact? #i-infinity))
($check-not-predicate (exact? #real))
($check-not-predicate (exact? #undefined))

($check-predicate (inexact? #real 3.14 #undefined #i+infinity))
($check-not-predicate (inexact? 0))
($check-not-predicate (inexact? #e+infinity))

($check-predicate (robust? 0 1 -1 1/3 999999999999 #e-infinity #e+infinity))
($check-predicate (robust? 3.14))           ; FAIL
($check-not-predicate (robust? #real))
($check-not-predicate (robust? #undefined))

($check-predicate (undefined? #undefined))
($check-not-predicate (undefined? 0))

;; 12.6.2 get-real-internal-bounds get-real-exact-bounds
;; TODO: How to test it?
($check equal? (get-real-internal-bounds 0) (list 0 0))
($check equal? (get-real-exact-bounds 0) (list 0 0))

;; 12.6.3 get-real-internal-primary get-real-exact-primary
;; TODO: How to test it?

;; 12.6.4 make-inexact
;; TODO

;; 12.6.5 real->inexact real->exact
;; TODO

;; 12.6.6 with-strict-arithmetic get-strict-arithmetic?
;; TODO

;; 12.7.1 with-narrow-arithmetic get-narrow-arithmetic?
;; TODO

;; 12.8.1 rational?

($check-predicate (rational? 0 1 1/2))
($check-not-predicate (rational? (sqrt 2)))    ; FAIL
($check-not-predicate (rational? #e+infinity))

;; 12.8.2 /

($check equal? (/ 2 3) 2/3)
($check equal? (/ 1 2 3) 1/6)
($check-error (/ 1 0))
($check-error (/ #e+infinity #e+infinity))

;; 12.8.3 numerator denominator

($check equal? (numerator 3/4) 3)
($check equal? (numerator -3/4) -3)
($check equal? (denominator 3/4) 4)
($check equal? (denominator -3/4) 4)

;; 12.8.4 floor ceiling truncate bound

($check equal? (floor 0) 0)
($check equal? (floor 1.23) 1)        ; FAIL
($check equal? (floor -1.23) -2)      ; FAIL

($check equal? (ceiling 0) 0)
($check equal? (ceiling 1.23) 2)      ; FAIL
($check equal? (ceiling -1.23) -1)    ; FAIL

($check equal? (truncate 0) 0)
($check equal? (truncate 1.99) 1)     ; FAIL
($check equal? (truncate -1.99) -1)   ; FAIL

($check equal? (round 0) 0)
($check equal? (round 1/2) 0)
;-- ($check equal? (round 1.1) 1)     ; FREEZES INTERPRETER 
($check equal? (round 3/2) 2)
;--($check equal? (round 1.9) 2)
($check equal? (round -1/2) 0)
;-- ($check equal? (round -1.1) -1)   ; FREEZES INTERPRETER
($check equal? (round -3/2) -2)
;--($check equal? (round -1.9) -2)

;; 12.8.5 rationalize simplest-rational

($check equal? (rationalize 0 1) 0)
($check equal? (rationalize 0.1 0.05) 1/6)       ; FAIL

($check equal? (simplest-rational 2/7 3/5) 1/2)
($check equal? (simplest-rational 0.1 0.3) 1/4)  ; FAIL

;; 12.9.1 real?

($check-predicate (real? 0 1 -1 1/2 999999999999 #e-infinity))
($check-not-predicate (real? #undefined))

;; 12.9.2 exp log
;; These functions are not described in the Report, but let us try...

($check equal? (exp 0.0) 1.0)
($check equal? (log 1.0) 0.0)

;; 12.9.2 sin cos tan
($check equal? (sin 0.0) 0.0)
($check equal? (cos 0.0) 1.0)
($check equal? (tan 0.0) 0.0)

;; 12.9.2 asin acos atan
($check equal? (asin 0.0) 0.0)
($check equal? (acos 1.0) 0.0)
($check equal? (atan 0.0) 0.0)

;; 12.9.5 sqrt
($check equal? (sqrt 0.0) 0.0)
($check equal? (sqrt 1.0) 1.0)
($check equal? (sqrt 4.0) 2.0)

;; 12.9.6 expt
($check equal? (expt 2.0 4.0) 16.0)

;; 12.10 Complex features
;; not implemented
