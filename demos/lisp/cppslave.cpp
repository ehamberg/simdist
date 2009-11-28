/********************************************************************
 *   		cppslave.cpp
 *   Created on Fri Apr 20 2007 by Boye A. Hoeverstad.
 *   Copyright 2009 Boye A. Hoeverstad
 *
 *   This file is part of Simdist.
 *
 *   Simdist is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Simdist is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Simdist.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   ***************************************************************
 *   
 *   Reads a lisp "binary" list, and counts the number of 1s.
 *******************************************************************/

#include <iostream>
#include <algorithm>
#include <functional>

using namespace std;

int
main(int argc, char *argv[])
{
  string s;
  while (getline(cin, s))
    cout << count_if(s.begin(), s.end(), bind2nd(equal_to<char>(), '1')) 
         << endl << flush;
  
  return 0;
}
