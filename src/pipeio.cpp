/********************************************************************
 *   		pipeio.cpp
 *   Created on Wed Jun 21 2006 by Boye A. Hoeverstad.
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
 *   Execute two programs and pipe them together.  First arg is taken
 *   to be the first program, second arg is taken to be the second
 *   (including arguments).
 *******************************************************************/

#include <simdist/pipeio_common.h>

#include <iostream>
#include <vector>
#include <sstream>
#include <errno.h>

#include <unistd.h>
#include <simdist/misc_utils.h>
#include <signal.h>
#include <cstring>
#include <cstdlib>

using namespace std;

size_t nBufSize = 1024 * 1024;
pid_t pid1 = 0, pid2 = 0;


/********************************************************************
 *   Do not pass keyboard-generated signals on to the child
 *   processes, they apparently get them directly from the shell.
 *******************************************************************/
void
signalhandler(int nSig)
{
//   cerr << "Signal in pipeio.\n";
//   exit(0);
//   if (pid1 > 0)
//     kill(pid1, nSig);
//   if (pid2 > 0)
//     kill(pid2, nSig);
}


typedef struct TThreadDataVar
{
  int nPipeRead;
  int nPipeWrite;
  string sDesc;
  TThreadDataVar(int nR, int nW, string sD)
      : nPipeRead(nR), nPipeWrite(nW), sDesc(sD)
  {
  }
} TThreadData;


void* threadBufferPipe(void *pArg)
{
      // Have all signals delivered to the main thread
  sigset_t sigSet;
  sigfillset(&sigSet);
  pthread_sigmask(SIG_BLOCK, &sigSet, 0);

  TThreadData &td = *reinterpret_cast<TThreadData*>(pArg);
  int *nRet = new int;
  *nRet = BufferPipe(nBufSize, td.nPipeRead, td.nPipeWrite);
  if (*nRet)
    cerr << "Error reported by " << td.sDesc << " pipe.\n";
  return nRet;
}


int
main(int argc, char *argv[])
{
  if (!CheckArgs(argc, argv))
    return 1;

  fdistream fReadOut1, fReadOut2;
  fdostream fWriteIn1, fWriteIn2;
  pid1 = Connect(1, argv, &fReadOut1, &fWriteIn1);
  pid2 = Connect(2, argv, &fReadOut2, &fWriteIn2);
  if (pid1 == -1 || pid2 == -1)
    return 1;

  TThreadData td[2] = { TThreadData(fReadOut1.fd(), fWriteIn2.fd(), "process 1 output"), 
                        TThreadData(fReadOut2.fd(), fWriteIn1.fd(), "process 1 input") };
  pthread_t nThreadIds[2];
  if (pthread_create(&nThreadIds[0], 0, threadBufferPipe, &td[0])
      || pthread_create(&nThreadIds[1], 0, threadBufferPipe, &td[1]))
  {
    cerr << "Failed to create one or both data shuffle threads.\n";
    return 1;
  }

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signalhandler;
  sigaction(SIGINT, &sa, 0);

  for (int nT = 0; nT < 2; nT++)
  {
    void *pStat = 0;
    pthread_join(nThreadIds[nT], &pStat);
    int *nStat = static_cast<int*>(pStat);
    if (*nStat)
    {
      cerr << "Error reported by shuffle thread " << nT << ".\n";
      return 1;
    }
    delete nStat;
  }

  return 0;
}
