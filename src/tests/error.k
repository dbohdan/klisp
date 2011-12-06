;; check.k & test-helpers.k should be loaded
;;
;; Tests of error handling applicatives.
;;

;; XXX error
;;
($check-error (error "test"))

;; XXX error-object? error-object-message error-object-irritants
;;
($let*
  ( (capture-error-object
      ($lambda (proc)
        (guard-dynamic-extent
        ()
        proc
        (list (list error-continuation
          ($lambda (obj divert)
          (apply divert obj)))))))
    (e1 (capture-error-object ($lambda () (error "a"))))
    (e2 (capture-error-object ($lambda () (error "b" 1 2 3))))
    (e3 (capture-error-object ($lambda () (error))))
    (e4 (capture-error-object ($lambda () (error 1)))))

  ($check-predicate (error-object? e1 e2 e3))
  ($check-not-predicate (error-object? ""))
  ($check-not-predicate (error-object? #f))
  ($check-not-predicate (error-object? ()))
  ($check-not-predicate (error-object? 0))

  ($check equal? (error-object-message e1) "a")
  ($check equal? (error-object-message e2) "b")

  ($check-error (error-object-message))
  ($check-error (error-object-message e1 e2))
  ($check-error (error-object-message "not an error object"))

  ($check equal? (error-object-irritants e1) ())
  ($check equal? (error-object-irritants e2) (list 1 2 3))
  ($check equal? (error-object-irritants e3) ())
;; error now uses the standard binding constructs from kghelper
;; for now they don't encapsulate any data in the error, but
;; they will in the future
;;  ($check equal? (error-object-irritants e4) (list 1))

  ($check-error (error-object-irritants))
  ($check-error (error-object-irritants e1 e2))
  ($check-error (error-object-irritants "not an error object")))

;; XXX system-error-continuation

($check-predicate (continuation? system-error-continuation))

($let*
  ( (catch-system-error
      ($lambda (proc)
        (guard-dynamic-extent
        ()
        proc
        (list (list system-error-continuation
          ($lambda (obj divert)
            ($let
              ( ( ((service code message errno) . tail)
                  (error-object-irritants obj)))
              (apply divert (list* service code tail))))))))))

    ($check equal?
      (catch-system-error
        ($lambda ()
          (rename-file "nonexistent-file-name" "other-file-name")))
        (list "rename" "ENOENT" "nonexistent-file-name" "other-file-name")))
