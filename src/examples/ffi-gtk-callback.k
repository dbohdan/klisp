;;
;; Dynamic FFI example.
;;
;; Inspired by example gtk.lua from Lua Alien FFI library
;; (the original is only 11 lines long, though...)
;;

($define! gtk (ffi-load-library "libgtk-x11-2.0.so.0"))

;; constants for c types
($define! i "sint")
($define! p "pointer")
($define! s "string")
($define! v "void")
($define! u32 "uint32")

;; helpers for applicative and callback creation
($define! make
  ($lambda (rtype name . atypes)
    (ffi-make-applicative gtk name
      (ffi-make-call-interface "FFI_DEFAULT_ABI" rtype atypes))))

($define! make-callback
  ($lambda (rtype app . atypes)
    (ffi-make-callback 
     app (ffi-make-call-interface "FFI_DEFAULT_ABI" rtype atypes))))


;; gtk bindings
($define! gtk-init (make v "gtk_init" p p))
($define! gtk-main (make v "gtk_main"))
($define! gtk-main-quit (make v "gtk_main_quit"))

($define! GTK_WINDOW_TOPLEVEL 0) ;; these two are for the window type enum
($define! GTK_WINDOW_POPUP 1)
($define! gtk-window-new (make p "gtk_window_new" i))
($define! gtk-window-set-title (make v "gtk_window_set_title" p s))
($define! gtk-widget-show (make v "gtk_widget_show" p))

($define! g-signal-connect-object (make u32 "g_signal_connect_object" p s p p i))
($define! G_CONNECT_AFTER 1) ;; these two are for the connect flag type enum
($define! G_CONNECT_SWAPPED 2)
($define! g-connect ($lambda (obj sig cb data) 
                      (g-signal-connect-object obj sig cb data G_CONNECT_AFTER)))  

;; as per GTK the callback returns void and takes two pointer params: 
;; the object and a data pointer (both are ignored in this case)
($define! my-destroy-handler 
  (make-callback v 
                 ($lambda (obj data)
                   (display "destroying...")
                   (gtk-main-quit))
                 p p))

(gtk-init () ())
($define! window (gtk-window-new GTK_WINDOW_TOPLEVEL))
(gtk-window-set-title window "Callback Test")
;; This will make the "x" in the window show a "destroying..." msg
;; and exit the gtk loop
(g-connect window "destroy" my-destroy-handler ())
(gtk-widget-show window)
(display "Running main...")
(gtk-main)
;; if we reached here, the callback was called and the main
;; gtk loop ended, so we are done!
(display "Done\n")

