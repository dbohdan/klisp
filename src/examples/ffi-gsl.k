;;
;; Dynamic FFI callback example.
;; Interfacing GNU Scientific Library.
;;
;; struct gsl_function_struct {
;;    double (* function) (double x, void * params);
;;    void * params;
;; };
;; typedef struct gsl_function_struct gsl_function ;
;;
;; int gsl_deriv_central (const gsl_function * f, double x, double h, double * result, double * abserr)
;;
;; (ffi-make-callback APPLICATIVE CALL-INTERFACE) creates a C callable
;; function with interface CALL-INTERFACE and returns pointer
;; to the entry point. The function will in turn call APPLICATIVE.
;;

($define! gsl (ffi-load-library "libgsl.so"))
($define! abi "FFI_DEFAULT_ABI")

($define! make-gsl-function
  ($let
    ( ((pointer-size alingment ref set!) (ffi-type-suite "pointer"))
      (cif (ffi-make-call-interface abi "double" (list "double" "pointer"))) )
  ($lambda (f)
    ($let
      ( (gslf (make-blob (* 2 pointer-size)) )
        (aux ($lambda (x params) (f x))))
      (set! (list gslf 0) (ffi-make-callback aux cif))
      gslf))))

($define! gsl-deriv-central
  ($let
    ( (gsl_deriv_central (ffi-make-applicative gsl "gsl_deriv_central"
        (ffi-make-call-interface abi "sint" (list "pointer" "double" "double" "pointer" "pointer"))))
      ((double-size alingment ref set!) (ffi-type-suite "double")))
    ($lambda (f)
      ($let
        ((gslf (make-gsl-function f)))
        ($lambda (x h)
          ($let
            ( (result (make-blob double-size))
              (abserr (make-blob double-size)))
           (gsl_deriv_central gslf x h result abserr)
           (list (ref result) (ref abserr))))))))

(display "Testing gsl_deriv_central...")
(newline)

($define! f
  ($lambda (x)
    (display (list "callback called with x = " x))
    (newline)
    (+ (* 2.0 x x) 3.0)))
($define! df (gsl-deriv-central f))

(for-each
  ($lambda (x)
    ($let*
      ( (fx (f x))
        ((dfx abserr) (df x 0.001)))
      (for-each
        display
        (list "x = " x ", f(x) = " fx ", f'(x) = " dfx ", |error| <= " abserr))
      (newline)))
  (list -1.0 0.0 1.0 2.0))
