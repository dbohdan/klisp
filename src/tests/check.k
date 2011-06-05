;;; Simple test framework based on srfi-78
;;;
;;; See Copyright Notice in klisp.h
;;;
;;; SOURCE NOTE: Based on the reference implementation by Sebastian Egner 
;;;
;;; TEMP: No eager comprehension for now
;;; XXX: modes are encapsulated values instead of symbols, it could also
;;; be done with a $check-set-mode! operative, or with keyword objects
;;; it they were implemented. 
;;;
;;; 

;; TODO refactor out some of the code in $check, $check-error, and the -ec 
;; variants, there is too much duplication and the applicatives are a bit 
;; too long.
($provide! 
 ($check $check-error check-report check-reset! check-set-mode! 
	 check-passed? check-mode-off check-mode-summary 
	 check-mode-report-failed check-mode-report)
 ;; PRIVATE

 ;; STATE

 ;; internal count
 ($define! passed 0)
 ($define! failed 0)
 ($define! first-failed #inert) ;; form: (error? . extra-data)
 ;; no error: (#f exp actual expected)
 ;; error: (#t string exp error)
 ;; failed = 0 => first-failed = #inert

 ;; initial state: report-failed (states are off summary report-failed and 
 ;; report)
 ($define! report-on? #t)    ; #t except in all states except: off
 ($define! report-fail? #t)  ; #t in states: report-failed and report
 ($define! report-pass? #f)  ; #t in state: report

 ;; encapsulation for mode parameter
 ($define! (enc-mode mode? get-mode-params) (make-encapsulation-type))
 ;; /STATE

 ;; little helper for error catching
 ;; This evaluates expression in the dynamic environment
 ;; If no error occurs it returs #t
 ;; If an there is an error, the handler applicative is called
 ;; in the dynamic environment with the object passed to the error 
 ;; continuation as sole argument
 ($define! $without-error?
	   ($vau (exp handler) denv
		 (guard-dynamic-extent
		  ()
		  ($lambda ()
			   (eval exp denv)
			   #t)
		  (list (list error-continuation
			      ($lambda (error-obj divert)
				       (apply (eval handler denv) 
					      (list error-obj) denv)
				       (apply divert #f)))))))
 
 ;; ;; another way to do the same: return a pair of (error? result/error-obj)
 ;; ;; but it is difficult to use because it starts nesting (see $check)
 ;; ($define! $try
 ;;   ($vau (exp) denv
 ;; 	   (guard-dynamic-extent
 ;; 	    ()
 ;; 	    ($lambda ()
 ;; 	      (list #t (eval exp denv))
 ;; 	    (list (list error-continuation
 ;; 			($lambda (error-obj divert)
 ;; 			  (apply divert (list #f error-obj)))))))))

 

 ($define! check-passed!
	   ($let ((env (get-current-environment)))
		 ($lambda ()
			  ($set! env passed (+ passed 1)))))

 ($define! check-failed/expected!
	   ($let ((env (get-current-environment)))
		 ($lambda ls
			  ($if (zero? failed)
			       ($set! env first-failed (cons #f ls))
			       #inert)
			  ($set! env failed (+ failed 1)))))

 ($define! check-failed/error!
	   ($let ((env (get-current-environment)))
		 ($lambda ls
			  ($if (zero? failed)
			       ($set! env first-failed (cons #t ls))
			       #inert)
			  ($set! env failed (+ failed 1)))))
 
 ($define! describe-passed
	   ($lambda (exp actual)
		    (show-exp exp)
		    (show-res actual)
		    (show-passed 1)))
 
 ($define! describe-failed
	   ($lambda (exp actual expected)
		    (show-exp exp)
		    (show-res actual)
		    (show-failed expected)))
 
 ($define! describe-error
	   ($lambda (str exp err-obj)
		    (display str)
		    (show-exp exp)
		    (show-error err-obj)))

 ($define! describe-first-failed
	   ($lambda ()
		    ($if (not? (zero? failed))
			 ($let (((error? . extra-data) first-failed))
			       (apply ($if error?
					   describe-error 
					   describe-failed)
				      extra-data))
			 #inert)))

 ;; show applicatives
 ($define! show-exp
	   ($lambda (exp)
		    (write exp)
		    (display " => ")))

 ($define! show-res
	   ($lambda (res)
		    (write res)))

 ($define! show-passed
	   ($lambda (cases)
		    (display "; *** passed ")
		    ($if (not? (=? cases 1))
			 ($sequence (display "(")
				    (display cases)
				    (display " cases)"))
			 #inert)
		    (display "***")
		    (newline)))

 ($define! show-failed
	   ($lambda (expected)
		    (display "; *** failed ***")
		    (newline)
		    (display " ; expected result: ")
		    (write expected)
		    (newline)))

 ($define! show-error
	   ($lambda (err-obj)
		    (display "; *** error ***")
		    (newline)
		    (display "; error object: ")
		    (write err-obj)
		    (newline)))
 ;; /PRIVATE

 ;; PUBLIC

 ;; general check facility. It always take an equality predicate
 ;; needs to be operative to save the original expression
 ($define! $check 
   ($let ((handler (wrap ($vau (error-obj) denv
			   ($set! denv error-obj error-obj)))))
     ($vau (test? exp expected) denv
       ($cond ((not? report-on?) #inert)
	      ((not? ($without-error? ($define! test? (eval test? denv)) 
				      handler))
	       ($let ((error-ls
		       (list "error evaling test? applicative: " test? 
			     error-obj)))
		 (apply check-failed/error! error-ls)
		 ($if report-fail?
		      (apply describe-error error-ls)
		      #inert)))
	      ((not? ($without-error? ($define! expected (eval expected denv))
				      handler))
	       ($let ((error-ls
		       (list "error evaling expected value: " expected 
			     error-obj)))
		 (apply check-failed/error! error-ls)
		 ($if report-fail?
		      (apply describe-error error-ls)
		      #inert)))
	      ((not? ($without-error? ($define! res (eval exp denv)) handler))
	       ($let ((error-ls
		       (list "error evaling expression: " exp error-obj)))
		 (apply check-failed/error! error-ls)
		 ($if report-fail?
		      (apply describe-error error-ls)
		      #inert)))
	      ((not? ($without-error? ($define! test-result
					(apply test? (list res expected)))
				      handler)) ;; no dyn env here
	       ($let ((error-ls
		       (list "error evaling (test? exp expected): "
			     (list test? exp expected) error-obj)))
		 (apply check-failed/error! error-ls)
		 ($if report-fail?
		      (apply describe-error error-ls)
		      #inert)))
	      (test-result
	       (check-passed!)
	       ($if report-pass? (describe-passed exp res) #inert))
	      (#t ; test-result = #f
	       (check-failed/expected! exp res expected)
	       ($if report-fail? (describe-failed exp res expected) 
		    #inert))))))

;; XXX /work in progress

 ;; helpers
 ($define! $check-ec-helper
   ($vau (test?-exp exp expected-exp escape/c) denv
    ;; TODO, add argument-list for errors
     ($cond ((not? ($without-error? ($define! test? (eval test? denv)) 
				    handler))
	     ($let ((error-ls
		     (list "error evaling test? applicative: " test? 
			   error-obj)))
	       (apply check-failed/error! error-ls)
	       ($if report-fail?
		    (apply describe-error error-ls)
		    #inert)
	       (apply-continuation escape/c #inert)))
	    ((not? ($without-error? ($define! expected (eval expected denv)) 
				    handler))
	     ($let ((error-ls
		     (list "error evaling expected value: " expected 
			   error-obj)))
	       (apply check-failed/error! error-ls)
	       ($if report-fail?
		    (apply describe-error error-ls)
		    #inert)
	       (apply-continuation escape/c #inert)
	       ))
	    ((not? ($without-error? ($define! res (eval exp denv)) handler))
	     ($let ((error-ls
		     (list "error evaling expression: " exp error-obj)))
	       (apply check-failed/error! error-ls)
	       ($if report-fail?
		    (apply describe-error error-ls)
		    #inert)
	       (apply-continuation escape/c #inert)))
	    ((not? ($without-error? ($define! test-result
				      (apply test? (list res expected)))
				    handler)) ;; no dyn env here
	     ($let ((error-ls
		     (list "error evaling (test? exp expected): "
			   (list test? exp expected) error-obj)))
	       (apply check-failed/error! error-ls)
	       ($if report-fail?
		    (apply describe-error error-ls)
		    #inert)
	       (apply-continuation escape/c #inert)))
	    (test-result
	      ; (check-passed!) passed only after all passed
	      ; ($if report-pass? (describe-passed exp res) #inert))
	     #inert
	     (#t ; test-result = #f
	      (check-failed/expected! exp res expected)
	      ($if report-fail? (describe-failed exp res expected) #inert)
	      (apply-continuation escape/c #inert))))))

 ($define! $check-ec
   ($let ((handler (wrap ($vau (error-obj) denv
			   ($set! denv error-obj error-obj)))))
     ($vau (gens test? exp expected . maybe-arg-list) denv
       ;; TODO add check
       ($define! arg-list ($if (null? maybe-arg-list)
			       ()
			       (car maybe-arg-list)))
       ($cond ((not? report-on?) #inert)
	      ((not? ($without-error? ($define! gen (eval (cons $nested-ec 
								gens)
							  denv)) handler))
	       ($let ((error-ls
		       (list "error evaling qualifiers: " gens error-obj)))
		 (apply check-failed/error! error-ls)
		 ($if report-fail?
		      (apply describe-error error-ls)
		      #inert)))
	      (($let/cc escape/c
		 ;; TODO add some security to the continuation
		 ;; (like make it one-shot and/or avoid reentry)
		 (eval (list do-ec (list gen)
			     (list check-ec-helper 
				   test?-exp exp expected-exp 
				   escape/c)))
		 #t)
	       ;; ... TODO passed with n cases
	       (check-passed!)
	       ($if report-pass? (describe-passed exp res) #inert)
	       )
	      (#t ;; TODO didn't pass...
	       #inert
	       )))))

;; XXX /work in progress

 ;; Check that the given expression throws an error
 ;; needs to be operative to save the original expression
 ;; (not in the srfi, probably because of poor specification of error 
 ;; signaling in R5RS
 ;; but very useful for checking proper argument checking)
 ($define! $check-error
	   ($let ((handler (wrap ($vau (error-obj) denv
				       ($set! denv error-obj error-obj)))))
		 ($vau (exp) denv
		       ($cond ((not? report-on?) #inert)
			      (($without-error? ($define! result 
						  (eval exp denv)) handler)
			       ($let ((error-ls
				       (list exp result "<ERROR>")))
				     (apply check-failed/expected! error-ls)
				     ($if report-fail?
					  (apply describe-failed error-ls)
					  #inert)))
			      (#t ;; didn't throw error
			       (check-passed!)
			       ($if report-pass?
				    (describe-passed exp error-obj)
				    #inert))))))

 ($define! check-report
	   ($lambda ()
		    ($if report-on?
			 ($sequence
			  (display "Tests Passed: ")
			  (write passed)
			  (newline)
			  (display "Tests Failed: ")
			  (write failed)
			  (newline)
			  (display "Tests Total: ")
			  (write (+ failed passed))
			  (newline)
			  (describe-first-failed))
			 #inert))) ;; state: off don't show anything


 ;; the modes are an encapsulated object each of 
 ;; '(off summary report-failed report)
 ;; is an ecapsulated list of their effect on state variables 
 ;; (report-on? report-error? report-pass?)
 ($define! check-mode-off (enc-mode (list #f #f #f)))
 ($define! check-mode-summary (enc-mode (list #t #f #f)))
 ($define! check-mode-report-failed (enc-mode (list #t #t #f)))
 ($define! check-mode-report (enc-mode (list #t #t #t)))
 
 ($define! check-set-mode!
	   ($let ((env (get-current-environment)))
		 ($lambda (mode)
			  ($if (mode? mode)
			       ($set! env 
				      (report-on? report-error? report-pass?)
				      (get-mode-params mode))
			       (#t (error "$check-set-mode: invalid mode"))))))
 
 ($define! check-reset!
	   ($let ((env (get-current-environment)))
		 ($lambda ()
			  ($set! env passed 0)
			  ($set! env failed 0)
			  ($set! env first-failed #inert))))

 ($define! check-passed? 
	   ($lambda (expected)
		    (and? (zero? failed)
			  (=? passed expected)))))
;; /PUBLIC


;; I drawed freely from the reference implementation so here is the 
;; copyright notice:

;; 
;; Permission is hereby granted, free of charge, to any person obtaining
;; a copy of this software and associated documentation files (the
;; ``Software''), to deal in the Software without restriction, including
;; without limitation the rights to use, copy, modify, merge, publish,
;; distribute, sublicense, and/or sell copies of the Software, and to
;; permit persons to whom the Software is furnished to do so, subject to
;; the following conditions:
;; 
;; The above copyright notice and this permission notice shall be
;; included in all copies or substantial portions of the Software.
;; 
;; THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND,
;; EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
;; NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
;; LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
;; OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
;; WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
;; 
;; -----------------------------------------------------------------------
;; 
;; Lightweight testing (reference implementation)
;; ==============================================
;;
;; Sebastian.Egner@philips.com

