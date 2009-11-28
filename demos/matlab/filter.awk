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



# Example of how Matlab can be used as a slave in Simdist by the
# help of a wrapper script.
#
# Here, Matlab estimates one-max fitness, by means of a series of
# filters.  The program expects the output to come from a master
# running Lisp, so the input from the master will be wrapped in
# parentheses.  We thus want to transform a genome '(1 0 1)' to
# the matlab command 'sum([1 0 1])', and then filter the output
# from Matlab such that only the response is printed from this
# script.  Finally, make sure that all filter programs run
# unbuffered, a.k.a. interactively.

# Replace normal parentheses with brackets using sed:
sed -u -e's/(/([/g; s/)/])/g' | 
# Create the Matlab command using awk:
awk -W interactive '
{ 
  print "sum"$0
}
# Send the data to matlab
' | matlab -nojvm -nosplash | 
# Finally, print only every fifth line coming from Matlab,
# starting with the 14th.  This skips the welcome message from
# Matlab (may have to be adjusted to fit your particular Matlab
# version).
sed -u -n -e'14~5p'

