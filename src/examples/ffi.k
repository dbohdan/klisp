;;
;; Basic FFI examples.
;;
;; usage:
;;    .../src$ make posix USE_LIBFFI=1
;;    .../src$ ./klisp examples/ffi-sdl.k
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
;;   "uint8"    uint8_t       fixint
;;   "uint16"   uint16_t      fixint
;;   "uint32"   uint32_t      fixint, bigint
;;   "uint64"   uint64_t      fixint, bigint
;;   "float"    float         double
;;   "double"   double        double
;;   "pointer"   (void *)     bytevector         (only for arguments)
;;                            string             (only for arguments)
;;                            nil
;;                            pointer (TAG_USER)
;;
;; Other data types not supported yet. Varargs function
;; not supported by libffi.
;;

($define! abi "FFI_DEFAULT_ABI")
($define! cif-int-void (ffi-make-call-interface abi "sint" ()))
($define! cif-string-string (ffi-make-call-interface abi "string" (list "string")))
($define! cif-int-string (ffi-make-call-interface abi "sint" (list "string")))
($define! cif-double-double (ffi-make-call-interface abi "double" (list "double")))

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

($define! getpid (ffi-make-applicative self "getpid" cif-int-void))
($define! getppid (ffi-make-applicative self "getppid" cif-int-void))
($define! system (ffi-make-applicative self "system" cif-int-string))
($define! getenv (ffi-make-applicative self "getenv" cif-string-string))
($define! lgamma (ffi-make-applicative self "lgamma" cif-double-double))

(display "Testing getpid(), getppid() ...")
(write (list (getpid) (getppid)))
(newline)

(display "Testing getenv(\"HOME\")...")
(write (getenv "HOME"))
(newline)

(display "Testing system(\"ls /\")...")
(newline)
(write (system "ls /"))
(newline)

(display "Testing lgamma(9.87)...")
(write (lgamma 9.87))
(newline)

($define! unix-write-string
  ($let*
    ( (unix-write (ffi-make-applicative libc "write"
        (ffi-make-call-interface abi
          "sint" (list "sint" "pointer" "sint")))))
    ($lambda (s) (unix-write 0 s (string-length s)))))

(display "Testing unix write()...")
(unix-write-string "ABCDEFGH")
(newline)

($define! strtoull
  (ffi-make-applicative libc "strtoull"
    (ffi-make-call-interface abi
          "uint64" (list "string" "pointer" "sint"))))

(display "Testing strtoull(\"0x123456789ABCDEF\", NULL, 0)...")
(write (strtoull "0x123456789ABCDEF" () 0))
(display "...")
(write #x123456789ABCDEF)
(display "= #x123456789ABCDEF")
(newline)

;; (ffi-type-suite TYPE) returns a four-element list
;; (SIZE ALIGNMENT REF SET!). SIZE is the size of
;; the data type in bytes. ALIGNMENT is preferred
;; alignment. REF and SET! are applicatives.
;;
;; (REF MEMORY-LOCATION)
;; (SET! MEMORY-LOCATION VALUE)
;;
;; MEMORY-LOCATION is either bytevector, string, pointer,
;; or a two-element list (MEMORY-LOCATION OFFSET).
;; The offset specification can not be nested, i.e.
;; ((blob 1) 2) is not valid memory location.
;;
($define!
  (sint-size sint-alignment sint-ref sint-set!)
  (ffi-type-suite "sint"))

(display "\"sint\" data type size and alignment: ")
(write (list sint-size sint-alignment))
(newline)

;; Using ffi-type-suite, one can define means to convert
;; C structs (stored in bytevectors or arbitrary memory locations)
;; to lists.
;;
($define! align
  ($lambda (offset alignment)
    (+ offset (mod (- alignment offset) alignment))))

($define! regularize-location
  ($lambda (location)
    ($if (pair? location)
      location
      (list location 0))))

($define! decode-struct
  ($lambda type-strings
    ($letrec*
      ( (suites (map ffi-type-suite type-strings))
        (decode ($lambda (base offset tail)
          ($if (null? tail)
            ()
            ($let (((size alignment ref set!) (car tail)))
              (cons
                (ref (list base (align offset alignment)))
                (decode
                  base
                  (+ size (align offset alignment))
                  (cdr tail))))))))
      ($lambda (location)
        ($let (((base offset) (regularize-location location)))
          (decode base offset suites))))))

;; For example,
;;
;;  struct timeval {
;;    time_t      tv_sec;     /* seconds */
;;    suseconds_t tv_usec;    /* microseconds */
;;  };
;;
($define! struct-timeval-ref
  (decode-struct "sint" "sint"))

($define! gettimeofday
  ($let
    ( (unix-gettimeofday
        (ffi-make-applicative libc "gettimeofday"
          (ffi-make-call-interface abi
             "sint" (list "pointer" "pointer")))))
    ($lambda ()
      ($let* ((buffer (make-bytevector (* 2 sint-size))))
        (unix-gettimeofday buffer ())
        ($let (((tv_sec tv_usec) (struct-timeval-ref buffer)))
          (list tv_sec (/ tv_usec 1000000)))))))

(display "Testing gettimeofday(), assuming 32-bit arch...")
(write (gettimeofday))
(newline)

($define! localtime
  ($let
    ( (localtime-r
        (ffi-make-applicative libc "localtime_r"
          (ffi-make-call-interface abi
            "pointer" (list "pointer" "pointer"))))
      (decoder
        (decode-struct "sint" "sint" "sint" "sint" "sint" "sint" "sint" "sint")))
    ($lambda (t)
      ($let*
        ( (t-buf (make-bytevector sint-size))
          (tm-buf (make-bytevector 128)) )
        (sint-set! t-buf t)
        (localtime-r t-buf tm-buf)
        ($let
          (((tm_sec tm_min tm_hour tm_mday tm_mon tm_year . rest) 
           (decoder tm-buf)))
          (list
            (list (+ 1900 tm_year) (+ 1 tm_mon) tm_mday)
            (list tm_hour tm_min tm_sec)))))))

(display "Testing localtime()...")
(write
  ($let (((tm_sec tm_usec) (gettimeofday)))
    (localtime tm_sec)))
(newline)

;; Some C structs are more complex:
;;
;;  struct hostent {
;;      char  *h_name;            /* official name of host */
;;      char **h_aliases;         /* alias list */
;;      int    h_addrtype;        /* host address type */
;;      int    h_length;          /* length of address */
;;      char **h_addr_list;       /* list of addresses */
;;  }
;;
;; Network address is just byte array. IPv4 address
;; contains 4 bytes, IPv6 address contains 16 bytes.
;;
;; (ffi-memmove DESTINATION SOURCE SIZE) copies
;; SIZE bytes from SOURCE to DESTINATION. Both SOURCE
;; and DESTINATION are memory locations as described above.
;; ffi-memmove can copy data between bytevectors and arbitrary
;; memory locations.
;;
($define! copy-location
  ($lambda (location size)
    ($let ((blob (make-bytevector size)))
      (ffi-memmove blob location size)
      blob)))

($define! bytevector->list
  ($letrec
    ((aux ($lambda (blob index)
      ($if (<? index (bytevector-length blob))
        (cons
          (bytevector-u8-ref blob index)
          (aux blob (+ 1 index)))
        ()))))
    ($lambda (blob)
      (aux blob 0))))

($define! parse-address
  ($lambda (location size)
    (bytevector->list (copy-location location size))))

($define!
  (voidptr-size voidptr-alignment voidptr-ref voidptr-set!)
  (ffi-type-suite "pointer"))

($define! null-terminated-array->list
  ($letrec
    ( (aux
        ($lambda (base offset)
          ($if (null? (voidptr-ref (list base offset)))
            ()
            (cons
              (list base offset)
              (aux base (+ offset voidptr-size)))))))
    ($lambda (location)
      (apply aux (regularize-location location)))))

($define!
  (charptr-size charptr-alignment charptr-ref charptr-set!)
  (ffi-type-suite "string"))

($define! parse-hostent
  ($letrec*
    ( (decode-1
        (decode-struct "string" "pointer" "sint" "sint" "pointer")))
    ($lambda (pointer)
      ($let
        ( ((h_name h_aliases h_addrtype h_length h_addr_list) (decode-1 pointer)))
        (list
          h_name
          (map
            charptr-ref
            (null-terminated-array->list h_aliases))
          (map
            ($lambda (a) (parse-address (voidptr-ref a) h_length))
            (null-terminated-array->list h_addr_list)))))))

($define! gethostbyname
  ($let
    ((unix-gethostbyname
       (ffi-make-applicative libc "gethostbyname"
         (ffi-make-call-interface abi "pointer" (list "string")))))
    ($lambda (hostname)
      (parse-hostent (unix-gethostbyname hostname)))))

(display "Testing gehostbyname(\"localhost\")...")
(write (gethostbyname "localhost"))
(newline)

(display "Testing gehostbyname(\"www.google.com\")...")
(write (gethostbyname "www.google.com"))
(newline)

;;
;; Detecting machine endianess.
;;

($define!
  (uint32-size uint32-alignment uint32-ref uint32-set!)
  (ffi-type-suite "uint32"))

($define! endianess
  ($let
    ((buffer (make-bytevector 4)))
    (uint32-set! buffer #x01020304)
    ($let
      ((bytes (bytevector->list buffer)))
      ($cond
        ((equal? bytes (list 1 2 3 4)) "big-endian")
        ((equal? bytes (list 4 3 2 1)) "little-endian")
        (#t                            "unknown")))))

(display "Guessing endianess...")
(write endianess)
(newline)
