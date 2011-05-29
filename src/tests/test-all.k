(load "tests/check.k")
(load "tests/test-helpers.k")

;(check-set-mode! check-mode-report)

;; TODO add applicative?/operative? check in pairs and lists and pair-mutation

(load "tests/booleans.k")
(load "tests/eq-equal.k")
(load "tests/symbols.k")
(load "tests/control.k")
(load "tests/pairs-and-lists.k")
(load "tests/pair-mutation.k")
(load "tests/environments.k")
(load "tests/environment-mutation.k")
(load "tests/combiners.k")

(check-report)