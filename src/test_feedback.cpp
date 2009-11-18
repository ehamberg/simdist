/********************************************************************
 *   		test_feedback.cpp
 *   Created on Wed Mar 22 2006 by Boye A. Hoeverstad.
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
 *   Testing the feedback library
 *******************************************************************/

#include <iostream>
#include <vector>
#include <pthread.h>
#include <cassert>
#include <cstdlib>
#include <simdist/feedback.h>
#include <simdist/errorcodes_thread.h>

const int default_info_level = 2;

using namespace std;

// FeedbackError E_SOME_ERROR("This is some error");
DEFINE_FEEDBACK_ERROR(E_SOME_ERROR, "This is some error")

int returntest(Feedback &fb)
{
  return fb.Error(E_SOME_ERROR);
}

void runtest(Feedback &f1, Feedback &f2)
{
//   assert(f1.Error(FeedbackError("Bogus error")) << ", that's a strange error" == 134565);
//   assert(f1.Error(E_THREAD_CREATE_EINVAL) == E_THREAD_CREATE_EINVAL);
  assert(f1.ErrorIfNonzero(0, E_THREAD_CREATE) == 0);

  f2.Warning("A warning") << " with the number " << 12 << ".";
  f2.Info(1) << "Information is the best";
  f2.Info(2) << "Information is the best";
  f2.Info(3) << "Information is the best";
  f2.Info(4) << "Information is the best";
  f2.Info(5) << "Information is the best";
}

struct tdvar {
  int nNumReps;
  Feedback *pf;
} td;

void* thread_func(void *)
{
  Feedback f2("This is f2");
  f2.RegisterThreadDescription("test-thread");

  for (int nRep = 0; nRep < td.nNumReps; nRep++)
    runtest(*td.pf, f2);
  return 0;
}

int main(int argc, char *argv[])
{
  
  if (argc != 1 && argc < 3)
  { 
    cerr << "Usage: " << argv[0] << " [num_threads num_reps [info-level] ]\n"
         << "Spawn num_threads threads, each printing a bunch of info "
         << "num_reps times.  Default 1 for both.  "
         << "Optionally specify info level as third argument.  Default is " 
         << default_info_level << ".\n";
    return 1;
  }

  int nNumThreads = argc > 2 ? atoi(argv[1]) : 1;
  int nNumReps    = argc > 2 ? atoi(argv[2]) : 1;
  int nInfoLevel  = argc > 3 ? atoi(argv[3]) : default_info_level;

  Feedback fb("Messages-tester");
  fb.RegisterThreadDescription("main");
  FeedbackCentral::Instance().SetInfoLevel(nInfoLevel);

  fb.Info(0) << "About to create " << nNumThreads << " thread(s), each running " 
       << nNumReps << " repetition(s) of the test.\n\n";

  returntest(fb);

      // Used to be global, but then order of initialization matters, and this may crash with initialization of FeedbackCentral::m_instance.  Caused problems on Mac when this file was linked before feedback.o
  Feedback f1("This is common feedback");
  td.nNumReps = nNumReps;
  td.pf = &f1;

  
      // Create threads and run tests
  vector<pthread_t> vtid(nNumThreads);

  for (int nThr = 0; nThr < nNumThreads; nThr++)
  {
    if (pthread_create(&(vtid[nThr]), 0, thread_func, 0))
    {
      fb.Warning() << "Failed to create thread number " << nThr << "!\n";
      return 1;
    }
    fb.Info(0) << "Created thread number " << nThr << " (id " << vtid[nThr] << ").\n";
  }

  for  (int nThr = 0; nThr < nNumThreads; nThr++)
  {
    if (pthread_join(vtid[nThr], 0))
    {
      fb.Warning() << "Failed to join thread number " << nThr << "!\n";
      return 1;
    }
  }

  return 0;

}
