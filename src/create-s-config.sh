#!/bin/bash

CONFFILE=config.txt
rm -f $CONFFILE

PROG=$HOME/devel/prog
DIST=$PROG/dist/trunk/src

# Test programs arguments

cat <<SHEOF | sed -e'/^#/ d' >$CONFFILE 
verbosity:	        0
slave-verbosity:	0
# verbosity-showonly:     DistributorLauncher, Master evaluation loop, MessagePasser, MessageRouter, MessageStreamer
# verbosity-showonly:     Master evaluation loop
# verbosity-showonly:     slave 1 main
verbosity-showonly:     SlaveClient, Master, JobQueue

batch-mode:             EOF
# batch-size:		$BATCH_SIZE

master-output-mode:      EOF
master-input-mode:      SIMPLE

master-program:		$DIST/s-test-master.sh
# master-arguments:	$DIST/bjornmm_test_out.txt
master-arguments:	$DIST/short.txt

slave-program:		$DIST/s-test-slave.sh

slave-wait-factor: 	10

jobs-per-send:    0

SHEOF

# chmod 400 $CONFFILE
