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

;; Minimal lisp master for demoing Simdist. See create-config.sh
;; for usage.  Short hints:
;;
;; lisp -quiet -load stdio_master.lisp
;;     ---- or ----
;; sbcl --noinform --noprint --load master.lisp
;;


;; Different lisps have different quit functions.
(defun do-quit ()
  (quit))

(defvar pop-size 300)

(defvar genome-length 100)

(defvar generations 100)

(defvar mutation-rate 10)

(defvar crossover-rate 75)

;; min-gene-val is inclusive, max-gene-val is not.
(defvar min-gene-val 0)
(defvar max-gene-val 2)

(defun crossover (rate genome1 genome2)
;;  (format t "crossing ~a with ~a~%" genome1 genome2)
  (if (< (random 100) rate)
      (let* ((pos1 (random (length genome1)))
	     (pos2 (- (length genome1) pos1)))
	(append (butlast genome1 pos1) (nthcdr pos2 genome2)))
	(if (= (random 2) 0)
	    genome1 genome2)))

(defun random-gene()
  (+ min-gene-val (random (- max-gene-val min-gene-val))))

(defun mutate (rate genome)
;;  (format t "mutating ~a~%" genome)
  (if (< (random 100) rate)
      (let* ((len (length genome))
	     (pos (random len))
	     (new (list (random-gene))))
	(append (butlast genome (- len pos)) new (nthcdr (+ pos 1) genome)))
    genome))

(defun create-genome (len)
  (labels ((c-iter (g)
                   (if (= (length g) len)
                       g
                     (c-iter (cons (random-gene) g)))))
          (c-iter '())))

(defun create-population (genome-len size)
  (labels ((p-iter (p)
                   (if (= (length p) size)
                       p
                     (p-iter (cons (create-genome genome-len) p)))))
          (p-iter '())))
  

;; (defun evaluate-population (pop)
;;   (labels ((print-gene (stream g)
;;                        (unless (null g)
;;                          (format stream "~a " (car g))
;;                          (print-gene stream (cdr g))))
;;            (print-pop (p)
;;                       (unless (null p)
;;                         (format t "~a~%" (car p))
;;                         ;;      (format t #'print-gene (car p))
;;                         (print-pop (cdr p))))
;;            (read-fitnesses (f)
;;                            (if (= (length f) (length pop))
;;                                (reverse f)
;;                              (let ((l (read)))
;;                                (read-fitnesses (cons l f))))))

;;           (print-pop pop)
;;           (read-fitnesses '())))

(defun evaluate-population (p)
  (loop for i in p do (format t "~a~%" i))
  (loop repeat (length p) 
    collect (read)))


(defun select-parent (pop fitn)
  (let* ((len (length pop))
	 (idx1 (random len))
	 (idx2 (random len))
	 (op (if (< (random 100) 75) #'> #'<)))
    
;;    (format t "Fighting idx ~a vs ~a, op is ~a~%" idx1 idx2 op)
    (if (funcall op (nth idx1 fitn) (nth idx2 fitn))
	(nth idx1 pop)
      (nth idx2 pop))))

(defun next-generation (pop fitn)
  (labels ((next-gen-iter (new-gen)
                          (if (= (length pop) (length new-gen))
                              new-gen
                            (next-gen-iter (cons (mutate mutation-rate
                                                         (crossover crossover-rate
                                                                    (select-parent pop fitn)
                                                                    (select-parent pop fitn)))
                                                 new-gen)))))
  (next-gen-iter '())))

(defun avg (l)
  (coerce (/ (apply #'+ l) (length l)) 'float))

(defun best-indiv (pop fitn)
  (labels ((return-first-match (pop fitn max-fitn)
                               (if (= (car fitn) max-fitn)
                                   (car pop)
                                 (return-first-match (cdr pop) (cdr fitn) max-fitn))))
          (return-first-match pop fitn (apply #'max fitn))))

(defun ga ()
  (labels ((ga-loop (gen pop status)
                    (let ((fitn (evaluate-population pop)))
      ;; Print status
                      (format status "Generation ~a. Min: ~a  Avg: ~a Max: ~a~%"
                              gen
                              (apply #'min fitn)
                              (avg fitn)
                              (apply #'max fitn))
                      ;; Finish or run next generation
                      (cond ((= gen generations)
                             (format status "Evolution finished. Best solution: ~%~%~a~%~%Bye bye.~%"
                                     (best-indiv pop fitn))
                             (do-quit))
                            (T (ga-loop (+ gen 1) (next-generation pop fitn) status))))))
           ;;   (let ((status (open "output.txt" 
           ;; 		      :direction :output 
           ;; 		      :if-exists :overwrite 
           ;; 		      :if-does-not-exist :create)))
          (ga-loop 1 (create-population genome-length pop-size) *error-output*)))
          
;;(setq *print-right-margin* (+ 10 (* 2 genome-length)))
(setf *print-length* nil)
(ga)
(do-quit)
