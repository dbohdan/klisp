;;
;; Dynamic FFI example.
;; Interfacing Simple DirectMedia Layer Library.
;;
;; Tested with SDL 1.2 on Debian Squeeze, x86.
;; It is quite likely that this program will not work
;; with other versions and other operating systems.
;;
;; Dynamic FFI is inherently unsafe. The user is responsible
;; for specifying correct argument types, data structure
;; layout, alignment, etc. even though it is platform dependent.
;;
;; usage:
;;    .../src$ make posix USE_LIBFFI=1
;;    .../src$ ./klisp examples/ffi-sdl.k
;;
;; The program shall create a window and responding
;; to mouse click.
;;

($define! sdl-import
  ($let
    ( (libsdl (ffi-load-library "libSDL.so"))
      (abi "FFI_DEFAULT_ABI"))
    ($lambda (rtype name . args)
      (ffi-make-applicative libsdl name
        (ffi-make-call-interface abi rtype args)))))

($define! SDL_INIT_TIMER #x00000001)
($define! SDL_INIT_AUDIO #x00000010)
($define! SDL_INIT_VIDEO #x00000020)
($define! SDL_INIT_NOPARACHUTE #x01000000)

($define! sdl-init (sdl-import "sint" "SDL_Init" "uint32"))
($define! sdl-quit (sdl-import "void" "SDL_Quit"))

($define! SDL_SWSURFACE  #x00000000)
($define! SDL_HWSURFACE  #x00000001)

($define! sdl-set-video-mode (sdl-import "pointer" "SDL_SetVideoMode" "sint" "sint" "sint" "uint32"))
($define! sdl-wm-set-caption (sdl-import "void" "SDL_WM_SetCaption" "string" "pointer"))

($define! sdl-wait-event
  ($let
    ((SDL_WaitEvent (sdl-import "sint" "SDL_WaitEvent" "pointer")))
    ($lambda ()
      ($let*
        ( (buffer (make-blob 512))
          (ok (SDL_WaitEvent buffer)))
        ($if (zero? ok)
          (apply-continuation error-continuation "SDL_WaitEvent signalled error")
          buffer)))))

($define! align
  ($lambda (offset alignment)
    (+ offset (mod (- alignment (mod offset alignment)) alignment))))

($define! $quote
  ($vau (x) denv x))

($define! $define-struct-projectors!
  ($letrec*
    ( (aux
        ($lambda (fields offset denv)
          ($if (null? fields)
            ()
            ($let*
              ( (((projector-name type-string) . tail) fields)
                ((size alignment ref set!) (ffi-type-suite type-string))
                (aligned-offset (align offset alignment))
                (projector-function ($lambda (blob) (ref (list blob aligned-offset)))))
              (write (list projector-name size alignment aligned-offset))
              (newline)
              (eval
                (list
                  ($quote $define!)
                  projector-name
                  (list ($quote $quote) projector-function))
                denv)
              (aux tail (+ size aligned-offset) denv))))))
    ($vau fields denv
      (aux fields 0 denv))))

($define! SDL_QUIT 12)
($define! SDL_MOUSEMOTION 4)
($define! SDL_MOUSEBUTTONDOWN 5)
($define! SDL_MOUSEBUTTONUP 6)

($define-struct-projectors!
  (event-type "uint8"))

($define-struct-projectors!
  (MouseMotionEvent.type "uint8")
  (MouseMotionEvent.state "uint8")
  (MouseMotionEvent.which "uint8")
  (MouseMotionEvent.x "uint16")
  (MouseMotionEvent.y "uint16")
  (MouseMotionEvent.xrel "sint16")
  (MouseMotionEvent.yrel "sint16"))

($define-struct-projectors!
  (MouseButtonEvent.type "uint8")
  (MouseButtonEvent.which "uint8")
  (MouseButtonEvent.button "uint8")
  (MouseButtonEvent.state "uint8")
  (MouseButtonEvent.x "uint16")
  (MouseButtonEvent.y "uint16"))

($define! with-sdl
  ($lambda (window-title worker)
    (display "Initializing SDL...")
    ($let ((status (sdl-init SDL_INIT_VIDEO)))
      (write status)
      (newline)
      ($if (<? status 0)
        (apply error-continuation "error initializing SDL")
        ($sequence
          (guard-dynamic-extent
            ()
            ($lambda ()
              ($let* ((screen (sdl-set-video-mode 640 480 32 SDL_HWSURFACE)))
                ($if (null? screen)
                  (apply-continuation error-contination "unable to set video mode")
                  ($sequence
                    (sdl-wm-set-caption window-title ())
                    (worker screen)))))
            (list
              (list
                error-continuation
                ($lambda (v divert)
                  (display "Error. Deinitializing SDL...")
                  (sdl-quit)
                  (display "done.")
                  (newline)
                  v))))
          (display "Finished. Deinitializing SDL...")
          (sdl-quit)
          (display "done.")
          (newline))))))

($define-struct-projectors!
  (SDL_Surface.flags "uint32")
  (SDL_Surface.format "pointer")
  (SDL_Surface.w "sint")
  (SDL_Surface.h "sint")
  (SDL_Surface.pitch "uint16")
  (SDL_Surface.pixels "pointer")
  (SDL_Surface.offset "sint"))

($define! draw-pixel
  ($let
    ( ((pixel-size pixel-alignment pixel-ref pixel-set!) (ffi-type-suite "uint32"))
      (SDL_MapRGB (sdl-import "uint32" "SDL_MapRGB" "pointer" "uint8" "uint8" "uint8"))
      (SDL_LockSurface (sdl-import "sint" "SDL_LockSurface" "pointer"))
      (SDL_UnlockSurface (sdl-import "void" "SDL_UnlockSurface" "pointer"))
      (SDL_Flip (sdl-import "sint" "SDL_Flip" "pointer")))
    ($lambda (screen (x y) (r g b))
      ($if (<? (SDL_LockSurface screen) 0)
        (apply error-continuation "SDL_LockSurface failed")
        ())
      ($let
        ( (pixels (SDL_Surface.pixels screen))
          (pitch (SDL_Surface.pitch screen))
          (color (SDL_MapRGB (SDL_Surface.format screen) r g b)))
        ;(display (list color x y pixel-size pitch (SDL_Surface.flags screen)))
        (pixel-set!
          (list pixels (+ (* x pixel-size) (* y pitch)))
          color))
      (SDL_UnlockSurface screen)
      (SDL_Flip screen))))

($define! event-loop
  ($lambda (screen drawing)
    ($let*
      ( (ev (sdl-wait-event))
        (t (event-type ev)))
      ($cond
        ( (equal? t SDL_QUIT)
          ())
        ( (and? drawing (equal? t SDL_MOUSEMOTION))
          (draw-pixel
            screen
            (list (MouseMotionEvent.x ev) (MouseMotionEvent.y ev))
            (list 0 255 0))
          (event-loop screen #t))
        ( (equal? t SDL_MOUSEBUTTONDOWN)
          (draw-pixel
            screen
            (list (MouseButtonEvent.x ev) (MouseButtonEvent.y ev))
            (list 255 0 0))
          (event-loop screen #t))
        ( (equal? t SDL_MOUSEBUTTONUP)
          (draw-pixel
            screen
            (list (MouseButtonEvent.x ev) (MouseButtonEvent.y ev))
            (list 0 0 255))
          (event-loop screen #f))
        (#t
          (event-loop screen drawing))))))

($define! main
  ($lambda (argv)
    (with-sdl "klisp ffi demo"
      ($lambda (screen) (event-loop screen #f)))))

