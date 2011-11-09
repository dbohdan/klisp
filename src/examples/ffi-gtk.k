;;
;; Dynamic FFI example.
;;
;; Inspired by example gtk.lua from Lua Alien FFI library
;; (the original is only 11 lines long, though...)
;;

($define! gtk (ffi-load-library "libgtk-x11-2.0.so.0"))
($define! i "sint")
($define! p "pointer")
($define! make
  ($lambda (rtype name . atypes)
    (ffi-make-applicative gtk name
      (ffi-make-call-interface "FFI_DEFAULT_ABI" rtype atypes))))

($define! gtk-init (make "void" "gtk_init" p p))
($define! gtk-message-dialog-new (make p "gtk_message_dialog_new" p i i i "string"))
($define! gtk-dialog-run (make i "gtk_dialog_run" p))

(gtk-init () ())
(gtk-dialog-run (gtk-message-dialog-new () 0 0 1 "Klisp Rocks!"))
