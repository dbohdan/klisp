;;
;; Windows API FFI example
;; See ffi.k for general info.
;;
;; usage:
;;  1) compile and install libffi 3.0.10
;;  2) set PATH correctly so libffi-5.dll can be found
;;  3) compile klisp: make mingw USE_LIBFFI=1
;;  4) run test script: ....\src> klisp examples/ffi-win32.k
;;

($define! kernel32 (ffi-load-library "kernel32"))
($define! abi "FFI_STDCALL")

($define! DWORD "uint32")
($define! BOOL "sint")

($define! dword-void (ffi-make-call-interface abi DWORD ()))
($define! u64-void (ffi-make-call-interface abi "uint64" ()))

($define! GetLastError (ffi-make-applicative kernel32 "GetLastError" dword-void))
($define! GetTickCount (ffi-make-applicative kernel32 "GetTickCount" dword-void))
($define! GetTickCount64 (ffi-make-applicative kernel32 "GetTickCount64" u64-void))

(display "Testing GetTickCount(), GetTickCount64() ... ")
(write (list (GetTickCount) (GetTickCount64)))
(newline)

($define! (DWORD-size DWORD-alignment DWORD-ref DWORD-set!)
  (ffi-type-suite DWORD))

($define! (charptr-size charptr-alignment charptr-ref charptr-set!)
  (ffi-type-suite "string"))

($define! blob->list
  ($lambda (blob index len)
    ($if (>? len 0)
      (cons
        (blob-u8-ref blob index)
        (blob->list blob (+ 1 index) (- len 1)))
      ())))

($define! blob->string
  ($lambda (blob offset len)
    (list->string (map integer->char (blob->list blob offset len)))))

($define! advapi32 (ffi-load-library "advapi32"))

($define! GetUserName
  ($let
    ( (win32-GetUserName (ffi-make-applicative advapi32 "GetUserNameA"
         (ffi-make-call-interface abi BOOL (list "pointer" "pointer"))))
      ((DWORD-size DWORD-alignment DWORD-ref DWORD-set!)
        (ffi-type-suite DWORD)))
    ($lambda ()
      ($let ((buffer (make-blob 256)) (lenbuf (make-blob 4)))
        (DWORD-set! lenbuf (blob-length buffer))
        (win32-GetUserName buffer lenbuf)
	(blob->string buffer 0 (- (DWORD-ref lenbuf) 1))))))

(display "Testing GetUserName()...")
(write (GetUserName))
(newline)

($define! user32 (ffi-load-library "user32"))

($define! HWND "pointer")
($define! UINT "uint32")
($define! MB_OK 0)

($define! MessageBox
  ($let
    ( (win32-MessageBox (ffi-make-applicative user32 "MessageBoxA"
        (ffi-make-call-interface abi "sint" (list HWND "string" "string" UINT)))))
    ($lambda (text caption)	
      (win32-MessageBox () text caption MB_OK))))

(display "Testing MessageBox()...")
(MessageBox "FFI test" "klisp")
(display "done.")
(newline)
