/********************************************************************
 *   		pipeio_common.cpp
 *   Created on Tue Nov 04 2008 by Boye A. Hoeverstad.
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
 *   See header file for description.
 *******************************************************************/

#include <simdist/pipeio_common.h>

#include <simdist/misc_utils.h>

#include <iostream>
#include <iterator>

using namespace std;

bool
CheckArgs(int argc, char *argv[])
{
  if (argc != 3)
  {
    cerr << "Usage: " << argv[0] << " process1 process2\n"
         << "process1 and process2 will be executed, and their I/O piped together.  "
         << "If the programs need arguments, run as follows:\n"
         << argv[0] << " 'process1 arg1 arg2' \"process2 arg1 arg2\"\n";
    return false;
  }
  return true;
}


pid_t
Connect(int nArg, char *argv[], 
        fdistream *pfReadOut, fdostream *pfWriteIn)
{
  std::string sCommand = argv[nArg];
  std::vector<std::string> argVec;
  if (CreateArgumentVector(sCommand, argVec))
  {
    cerr << "Failed to parse argument " << nArg << " into process names and arguments.\n";
    return false;
  }

  pid_t nPid;
  fdistream fReadErr;
  bool bConnectErr = false;

  if (ConnectProcess(argVec, pfWriteIn, pfReadOut, (bConnectErr ? &fReadErr : 0), &nPid, 0))
  {
    cerr << "Failed to connect to child process.  Argument vector is:\n\t";
    copy(argVec.begin(), argVec.end(), ostream_iterator<string>(cerr, "\t"));
    cerr << "\n";
    return static_cast<pid_t>(-1);
  }
  return nPid;
}
