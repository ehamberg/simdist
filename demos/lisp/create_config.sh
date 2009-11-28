#!/bin/bash
#
# Copyright 2009 Boye A. Hoeverstad
#
# This file is part of Simdist.
#
# Simdist is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Simdist is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Simdist.  If not, see <http://www.gnu.org/licenses/>.
#

# Demo script that creates a config file which can in turn be
# read by Simdist.  This script is written for use with Steel
# Bank Common Lisp (sbcl).
#
# Use as follows:
# - Modify POPSIZE and GENERATIONS parameters below
# - On the command line, type '. create-config.sh'.  Note the dot
#   in front.  This tells the shell to source (i.e. read and
#   execute) the commands in this file.  This will modify
#   'master.lisp' and create the files 'config.txt' and 'pbs.sh'
# - Run simdist either directly by 'simdist config.txt', or, if
#   you are on a cluster using PBS which happens to have a
#   queue called 'optimist', you may submit the job by typing
#   'qsub pbs.sh'.
#


# Name of output file
CONFFILE=config.txt
rm -f $CONFFILE

# Test programs arguments
POPSIZE=300
GENERATIONS=100
# GENOME_LENGTH=50

MASTER="master.lisp"
SLAVE="slave.lisp"

sed -i -e"
s/(defvar pop-size [0-9]*)/(defvar pop-size $POPSIZE)/;
s/(defvar generations [0-9]*)/(defvar generations $GENERATIONS)/;" $MASTER
# s/(defvar genome-length [0-9]*)/(defvar genome-length $GENOME_LENGTH)/;" $MASTER

cat <<SHEOF | sed -e'/^#/ d' >$CONFFILE 
verbosity:	        0
slave-verbosity:	0

# CMUCL users may use something like
# lisp -quiet -load master.lisp
master-program:		logio
master-arguments:	-e output.txt sbcl --noinform  --no-userinit  --load $MASTER

# slave-program:        ./cppslave
slave-program:          sbcl
slave-arguments:	--noinform  --no-userinit  --load $SLAVE

SHEOF

cat <<SHEOF >pbs.sh
#PBS -q optimist -l nodes=10
cd `pwd`
simdist $CONFFILE
SHEOF
