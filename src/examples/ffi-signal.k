;;
;; Dynamic FFI example.
;; Signal handling and interpreter extension implemented in C.
;;
;; usage:
;;    .../src$ make posix USE_LIBFFI=1
;;           $ cd examples
;;           $ make -f ffi-signal.makefile
;;           $ ../klisp ffi-signal.k
;;
;; files:
;;  ffi-signal.so ......... interpreter extension compiled to a DLL
;;  ffi-signal.k .......... example of client code
;;  ffi-signal.c  ......... C source of the extension
;;  ffi-signal.makefile ... build script
;;

;; (ffi-klisp-state) returns a value which encodes pointer
;; to the interpreter global state (klisp_State *).
;;
;; The following code loads the dynamic library ffi-signal.so
;; and passes the klisp_State pointer to the initialization
;; function kinit_signal_example(). Having access to the
;; internal interpreter structures, the initialization function
;; adds new bindings to the current dynamic environment.
;;
((ffi-make-applicative
  (ffi-load-library "./ffi-signal.so")
    "kinit_signal_example"
    (ffi-make-call-interface
      "FFI_DEFAULT_ABI" "void" (list "pointer")))
 (ffi-klisp-state))

;; The dynamic environment now contains two new bindings:
;;
;;   (install-signal-handler SIGNAME) installs handler for
;;   the signal named SIGNAME (e.g. "SIGINT"). Whenever
;;   a signal arrives, the handler writes a byte into
;;   an internal pipe.
;;
;;   (open-signal-port) opens the read-end of the internal pipe
;;   as a port.
;;
;; The following code demonstrates the signal handling (it is not
;; possible to install arbitrary klisp procedure as a signal handler,
;; because the interpreter is not reentrant).
;;
(install-signal-handler "SIGINT")
($define! signal-port (open-signal-port))
(display "Installed signal handler for SIGINT. Press Ctrl-C to continue...")
(read-char signal-port)
(newline)
(display "Signal detected. Press Ctrl-C again...")
(read-char signal-port)
(newline)
(display "Done.")
(newline)

