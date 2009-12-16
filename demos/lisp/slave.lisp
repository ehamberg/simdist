;; Copyright 2009 Boye A. Hoeverstad
;;
;; This file is part of Simdist.
;;
;; Simdist is free software: you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.
;;
;; Simdist is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with Simdist.  If not, see <http://www.gnu.org/licenses/>.
;;

;; Minimal lisp slave for demoing Simdist. See create-config.sh
;; for usage.  Short hints:
;;
;; lisp -quiet -load stdio_slave.lisp
;;     ---- or ----
;; sbcl --noinform --noprint --load slave.lisp
;;


;; Different lisps have different quit functions.
(defun do-quit ()
  (quit))

(defun reporter ()
  (let ((c 0)
	(freq 100))
    (lambda ()
      (cond ((eq c freq) 
	     (format *error-output* "Slave eval'ed ~a genomes.~%" freq)
	     (setf c 0))
	    (T 
	     (setf c (+ c 1)))))))

;; Scheme-ish version (CL should use labels)
;; (defun count-ones (l)
;;   (defun count-ones-iter (n l)
;;     (cond ((null l) n)
;; 	  ((= 1 (car l))
;; 	   (count-ones-iter (+ n 1) (cdr l)))
;; 	  (T (count-ones-iter n (cdr l)))))
;;   (count-ones-iter 0 l))

; Short version
(defun count-ones (n l)
  (if (null l) n
    (count-ones (+ n (car l)) (cdr l))))

(defun eval-loop (&optional rep)
  (let ((line (read *standard-input* 
                    nil nil nil)))
    (when line
      (format t "~a~%" (count-ones 0 line))
      (when rep (funcall rep))
      (eval-loop rep))))

;; Old version, Scheme-ish
;; (defun eval-loop ()
;;   (let ((rep (reporter)))
;;     (defun read-loop (file)
;;       (let ((line (read file nil 'eof)))
;; 	(cond ((eq 'eof line)
;; 	       (do-quit))
;; 	      (T (format t "~a~%" (count-ones line))
;; 		 (funcall rep)
;; 		 (read-loop file)))))
;;     (with-open-file (input "/dev/stdin" :direction :input)
;; 		    (read-loop input))))

;; With slave reporting
;; (eval-loop (reporter))
;; Without slave reporting
(eval-loop) 
