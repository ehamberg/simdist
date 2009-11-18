/********************************************************************
 *   		multi_pipeio.cpp
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
 *   Like pipeio, but creates several slaves.  Makes some
 *   simplifying assumptions:
 *
 *   - The programs will work in master-slave mode.  
 *
 *   - All data is 1-liners
 *
 *   - Slaves never print anything at their own will
 *******************************************************************/

#include <simdist/pipeio_common.h>

#include <simdist/misc_utils.h>

#include <iostream>
#include <list>
#include <poll.h>
#include <errno.h>
#include <cstdlib>


using namespace std;

static const int num_slaves = 4;

list<string> questions, answers;

fdistream fReadMaster, fReadSlaves[num_slaves];
fdostream fWriteMaster, fWriteSlaves[num_slaves];

void
PollMaster()
{
  questions.clear();
  int nWait = -1; // Wait forever for the first input
  while (true)
  {
        // Accept EINTR; this will happen if we attach a debugger.
    struct pollfd pollFd = { fReadMaster.fd(), POLLIN, 0 };
    int nPollRet = poll(&pollFd, 1, nWait);
    while (nPollRet == -1 && errno == EINTR)
      nPollRet = poll(&pollFd, 1, nWait);
    if (nPollRet <= 0)
      break;

    string sLine;
    if (!getline(fReadMaster, sLine) || sLine.empty())
      break;
    questions.push_back(sLine);
      
        // Wait at most 10 msec for subsequent input
    nWait = 10;
  }
}


void 
PushToSlaves()
{
  answers.clear();
  for (list<string>::iterator qIt = questions.begin(); qIt != questions.end();)
  {
    int nCurSlave = 0;
    for (; nCurSlave < num_slaves && qIt != questions.end(); nCurSlave++, qIt++)
      fWriteSlaves[nCurSlave] << *qIt << "\n" << flush;
    for (int nAnswer = 0; nAnswer < nCurSlave; nAnswer++)
    {
      string sLine;
      if (!getline(fReadSlaves[nAnswer], sLine) || sLine.empty())
      {
        cerr << "ERROR: Failed to read answer from slave " << nAnswer << ".\n";
        exit(1);
      }
      answers.push_back(sLine);
    }
  }
}

      
void
WriteMaster()
{
  for(list<string>::iterator aIt = answers.begin(); aIt != answers.end(); aIt++)
    fWriteMaster << *aIt << "\n";
  fWriteMaster.flush();
}


int
main(int argc, char *argv[])
{
  if (!CheckArgs(argc, argv))
    return 1;
  
  if (Connect(1, argv, &fReadMaster, &fWriteMaster) == -1)
    return 1;
  for (int nIdx = 0; nIdx < num_slaves; nIdx++)
    if (Connect(2, argv, &fReadSlaves[nIdx], &fWriteSlaves[nIdx]) == -1)
      return 1;

//   cerr << "Reading a char from the first slave...\n";
//   char c;
//   fReadSlaves[0] >> c;
//   cerr << "Done, got '" << c << "'.\n";
//   exit(1);

  while (true)
  {
    PollMaster();
    if (questions.empty())
      return 0;
    PushToSlaves();
    WriteMaster();
  }
}
