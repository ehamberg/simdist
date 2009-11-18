#!/bin/bash

CONFFILE=options_dist.txt
rm -f $CONFFILE

DIST=$HOME/devel/prog/dist/trunk/src

# Test programs arguments
BATCH_SIZE=10
FACTOR=12
ITERATIONS=2
SIG1=10
SIG2=12
MASTER_OUTPUT_MODE=SIMPLE
MASTER_INPUT_MODE=SIMPLE

cat <<SHEOF | sed -e'/^#/ d' >$CONFFILE 
verbosity:	        3
slave-verbosity:	3
# verbosity-showonly:     Master evaluation loop, Master main, Slave 1 main, Slave 2 main

master-output-mode:      $MASTER_OUTPUT_MODE
master-input-mode:      $MASTER_INPUT_MODE

# test-master takes the following arguments: numlines factor [iterations]
# master-program:	       $HOME/local/bin/logio
# master-arguments:      -v -i $DIST/log_i.txt -o $DIST/log_o.txt -e $DIST/log_e.txt $DIST/test-master 500 12 100
master-program:		$DIST/test-master
master-arguments:	--lines $BATCH_SIZE --factor $FACTOR --iterations $ITERATIONS --signal $SIG1 --signal $SIG2 --job-input-mode $MASTER_INPUT_MODE --job-output-mode $MASTER_OUTPUT_MODE

# test-slave takes the following arguments: factor [-v]
# The factor must be the same as the one given to test-master
slave-program:		logio
slave-arguments:	-i slave_SLAVEID_input.txt -o slave_SLAVEID_output.txt $DIST/test-slave --factor $FACTOR --signal $SIG1 --signal $SIG2 --job-input-mode $MASTER_OUTPUT_MODE --job-output-mode $MASTER_INPUT_MODE 

slave-wait-factor: 	0.1

jobs-per-send:    0

SHEOF

# chmod 400 $CONFFILE
