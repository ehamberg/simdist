#!/bin/bash

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

GENERATIONS=10
POPSIZE=3

for ((g=0; g<$GENERATIONS; g++)); do
    cat $1
    echo "Master wrote generation $g, ready to read input..." >&2
    for ((p=0; p<$POPSIZE; p++)); do
	read input
	echo "Returned $p from slave: $input" >&2
    done
done


