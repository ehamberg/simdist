/********************************************************************
 *   		pvm_slave_tester.cpp
 *   Created on Mon Mar 20 2006 by Boye A. Hoeverstad.
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
 *   Testing the pvm slave, by having this program read input and
 *   provide an answer on output after a pause of random length, in
 *   order to test the timing/abort features.
 *
 *   The maximum wait time before answering can be passed as a command
 *   line argument.
 *******************************************************************/

#include <iostream>
#include <cstdlib>
#include <sys/types.h>
#include <time.h>

using namespace std;

int main(int argc, char *argv[])
{
  const int line_len=2046, max_wait_default = 5;

  int max_wait = (argc > 1) ? atoi(argv[1]) : max_wait_default;
  
  srand(time(0));

  char szLine[2046+1];
  while (cin.getline(szLine, line_len))
  {
    int nWait = my_rand(max_wait);
    sleep(nWait);
    cout << "This is an answer to " << szLine << " after " << nWait << "seconds.\n";
  }
  return 0;
}
