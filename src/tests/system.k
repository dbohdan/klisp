;; check.k & test-helpers.k should be loaded
;;
;; Tests of system features.
;;

;; (R7RS 3rd draft, section 6.7.4) get-current-second

($check-predicate (applicative? get-current-second))
($check-predicate (number? (get-current-second)))

;; TODO: Update before the year 2031....

($let ((T-2011-01-01 1293836400) (T-2031-01-01 1924988400))
  ($check-predicate (<? T-2011-01-01 (get-current-second)))
  ($check-predicate (>? T-2031-01-01 (get-current-second))))

;; (R7RS 3rd draft, section 6.7.4) get-current-jiffy get-jiffies-per-second

($check-predicate (applicative? get-current-jiffy get-jiffies-per-second))
($check-predicate (exact-integer? (get-current-jiffy) (get-jiffies-per-second)))
($check-predicate (positive? (get-current-jiffy) (get-jiffies-per-second)))

($let* ((jiffy1 (get-current-jiffy)) (jiffy2 (get-current-jiffy)))
  ($check-predicate (<=? jiffy1 jiffy2)))

($let* ((jps1 (get-jiffies-per-second)) (jps2 (get-jiffies-per-second)))
  ($check-predicate (=? jps1 jps2)))
