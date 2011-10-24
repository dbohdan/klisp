;;
;; (ffi-load-library DLLNAME) ... loads the C library DLLNAME
;; and returns opaque handle.
;;
;; (ffi-load-library) ... returns a handle, which can be used
;; to access the functions linked statically to the interpreter
;;
;; Unloading not supported. ffi-load-library is actually
;; a wrapper around dlopen()
;;
($define! libc (ffi-load-library "libc.so.6"))
($define! self (ffi-load-library))

;; (ffi-make-call-interface ABI RETURN-TYPE ARGUMENT-TYPES) returns
;; libffi call interface object. It is actually a wrapper around
;; ffi_prep_cif().
;;
;; The parameter ABI determines the C call convention. Only
;; "FFI_DEFAULT_ABI" is supported.
;;
;; RETURN-TYPE determines the return type and ARGUMENT-TYPES
;; is a list which determines the arguments. The types
;; are specified as strings:
;;
;;    type      C type       klisp type           note
;;   ----------------------------------------------------
;;    "void"     void         inert              (only return)
;;    "sint"     signed int   fixint
;;   "string"    (char *)     string
;;   "pointer"   (void *)     blob, string, nil  (only arguments)
;;
;; Other data types not supported yet. Varargs function
;; not supported by libffi.
;;

($define! abi "FFI_DEFAULT_ABI")
($define! cif1 (ffi-make-call-interface abi "sint" ()))
($define! cif2 (ffi-make-call-interface abi "sint" (list "string")))
($define! cif3 (ffi-make-call-interface abi "string" (list "string")))

;; (ffi-make-applicative LIB-HANDLE FUNCTION-NAME CALL-INTERFACE)
;;
;; Looks up the function FUNCTION-NAME in the library referenced
;; by LIB-HANDLE. Creates an applicative which calls the function
;; using the interface CALL-INTERFACE. Conversion from/to klisp
;; types is handled automatically.
;;
;; It is a wrapper around dlsym(). The types should match the
;; actual C function prototype, the interpreter might crash
;; otherwise.
;;

($define! getpid (ffi-make-applicative self "getpid" cif1))
($define! getppid (ffi-make-applicative self "getppid" cif1))
($define! system (ffi-make-applicative self "system" cif2))
($define! getenv (ffi-make-applicative self "getenv" cif3))

($define! horner
  ($lambda (polynomial x acc)
    ($if (null? polynomial)
      acc
      (horner (cdr polynomial) x (+ (car polynomial) (* x acc))))))

($define! u32-of-u8-list
  ($lambda (list)
    (horner list 256 0)))

;; warning: 32-bit little endian only
($define! gettimeofday
  ($let
    ( (unix-gettimeofday (ffi-make-applicative libc "gettimeofday"
       (ffi-make-call-interface abi
         "sint" (list "pointer" "pointer")))))
    ($lambda ()
      ($let* ( (buffer (make-blob 8))
               (b ($lambda (i) (blob-u8-ref buffer i))))
        (unix-gettimeofday buffer ())
        (list
          (u32-of-u8-list (map b (list 3 2 1 0)))
          (/ (u32-of-u8-list (map b (list 7 6 5 4))) 1000000))))))

($define! unix-write-string
  ($let*
    ( (unix-write (ffi-make-applicative libc "write"
        (ffi-make-call-interface abi
          "sint" (list "sint" "pointer" "sint")))))
    ($lambda (s) (unix-write 0 s (string-length s)))))

(display "Testing unix write()...")
(unix-write-string "ABCDEFGH")
(newline)

(display "Testing getpid()...")
(write (getpid))
(newline)

(display "Testing getppid()...")
(write (getppid))
(newline)

(display "Testing getenv(\"HOME\")...")
(write (getenv "HOME"))
(newline)

(display "Testing gettimeofday(), assuming 32-bit intel arch...")
(write (gettimeofday))
(newline)

(display "Testing system(\"ls /\")...")
(newline)
(write (system "ls /"))
(newline)


