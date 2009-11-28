#!/usr/bin/python -u
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


"""Minimal Python slave for OneMax estimation with Simdist.""" 

import sys
line = sys.stdin.readline()    # Read genes from standard input
while line != "":              
    genes = [int(i) for i in line.split()]
    fitness = sum(genes)
    print fitness               # Print fitness after evaluation
    line = sys.stdin.readline()
