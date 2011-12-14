;; check.k & test-helpers.k should be loaded
;;
;; Tests of promises - lazy evaluation features.
;;

;; 9.1.1 promise?

($check-predicate (promise?))
($check-predicate (promise? ($lazy 0) (memoize 0)))
($check-not-predicate (promise? 0))
($check-not-predicate (promise? ()))
($check-not-predicate (promise? #inert))

;; 9.1.2 force

($check equal? (force 0) 0)
($check equal? (force (force 1)) 1)
($check equal? (force ($lazy 2)) 2)
($check equal? (force (force ($lazy 3))) 3)
($check equal? (force ($lazy ($lazy 4))) 4)
($check-error (force))
($check-error (force "too" "many"))

;; 9.1.3 $lazy

($check-error ($lazy))
($check-error ($lazy "too" "many"))
($check equal? (force ($lazy (get-current-environment))) 
        (get-current-environment))

;; Test cases from R(-1)RK
($define! lazy-test-1
  ($sequence
    ($provide! (get-count p)
      ($define! count 5)
      ($define! get-count ($lambda () count))
      ($define! p
        ($let ((self (get-current-environment)))
          ($lazy
           ($if (<=? count 0)
                count
                ($sequence
                  ($set! self count (- count 1))
                  (force p)
                  ($set! self count (+ count 2))
                  count))))))
    ($check equal? (get-count) 5)
    ($check equal? (force p) 0)
    ($check equal? (get-count) 10)))

($define! lazy-test-2
  ($let
      ((temp-file "klisp-ports-test.txt"))
    (with-output-to-file temp-file
                         ($lambda ()
                           ($define! p1 ($lazy (display "*")))
                           ($define! p2 ($lazy p1))
                           (force p2)
                           (force p1)))
    ($let
        ((result (with-input-from-file temp-file read)))
      (delete-file temp-file)
      result)))

($check equal? lazy-test-2 ($quote *))

;; The third test constructs infinite lazy list
;; and forces first 100 elements. The Kernel Report
;; version forces 10^10 elements.
;;
;; TODO: Test the original version in separate script
;; as a benchmark.
;;
($define! lazy-test-3
  ($sequence
    ($define! stream-filter
      ($lambda (p? s)
        ($lazy
         ($let ((v (force s)))
           ($if (null? v)
                v
                ($let ((s (stream-filter p? (cdr v))))
                  ($if (p? (car v))
                       (cons (car v) s)
                       s)))))))
    ($define! from
      ($lambda (n)
        ($lazy (cons n (from (+ n 1))))))
    (force
     (stream-filter ($lambda (n) (=? n 100))
                    (from 0)))))

($check equal? (car lazy-test-3) 100)

;; 9.1.4 memoize

($check-error (memoize))
($check-error (memoize "too" "many"))

($check equal? (force (memoize 0)) 0)
($check equal? (force (force (memoize 0))) 0)
($check-predicate (promise? (force (memoize (memoize 0)))))
($check equal? (force (force (memoize (memoize 0)))) 0)
($check-predicate (promise? (force (memoize ($lazy 0)))))
($check equal? (force (force (memoize ($lazy 0)))) 0)
($check equal? (force ($lazy (memoize 0))) 0)
($check equal? (force (force ($lazy (memoize 0)))) 0)

;; 9.1.5? $delay

($check-error (memoize))
($check-error (memoize "too" "many"))

($check equal? (force ($delay 0)) 0)
($check equal? (force (force ($delay 0))) 0)
($check equal? (force ($delay (get-current-environment))) 
        (get-current-environment))
($check-predicate (promise? (force ($delay (memoize 0)))))
($check equal? (force (force ($delay (memoize 0)))) 0)
