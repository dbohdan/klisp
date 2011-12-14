;; check.k & test-helpers.k should be loaded
;;
;; Tests of system features.
;;

;; (R7RS 3rd draft, section 6.7.4) current-second

($check-predicate (applicative? current-second))
($check-predicate (number? (current-second)))

;; TODO: Update before the year 2031....

($let ((T-2011-01-01 1293836400) (T-2031-01-01 1924988400))
  ($check-predicate (<? T-2011-01-01 (current-second)))
  ($check-predicate (>? T-2031-01-01 (current-second))))

;; (R7RS 3rd draft, section 6.7.4) current-jiffy jiffies-per-second

($check-predicate (applicative? current-jiffy jiffies-per-second))
($check-predicate (exact-integer? (current-jiffy) (jiffies-per-second)))
($check-predicate (positive? (current-jiffy) (jiffies-per-second)))

($let* ((jiffy1 (current-jiffy)) (jiffy2 (current-jiffy)))
  ($check-predicate (<=? jiffy1 jiffy2)))

($let* ((jps1 (jiffies-per-second)) (jps2 (jiffies-per-second)))
  ($check-predicate (=? jps1 jps2)))
