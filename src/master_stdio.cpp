/********************************************************************
 *   		master_stdio.cpp
 *   Created on Fri Mar 17 2006 by Boye A. Hoeverstad.
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
 *   Stub to run master node using standard input/output.
 *
 *   This piece should be rewritten to process input and output
 *   separately.  In the case where the master process is slow in
 *   producing output, the poll may time out even though we have
 *   not reached the end of the batch.  This will lead to
 *   suboptimal processing of the jobs in the batch, since a)
 *   jobs can't be grouped optimally, and b) in the worst case,
 *   some slaves may be running idle.
 *******************************************************************/


// slave_mpi.h must be included before stdio. See comment in slave_mpi.h
#include <simdist/slave_mpi.h>

#include <simdist/master_stdio.h>

#include <simdist/timer.h>
#include <simdist/master.h>

#include <simdist/io_utils.h>
#include <simdist/misc_utils.h>
#include <simdist/options.h>

#include <iostream>
#include <fstream>
#include <cassert>

#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <setjmp.h>
#include <poll.h>

// FeedbackError E_MASTERMAIN_LOOPSETUP("Evaluation loop setup failed");
DEFINE_FEEDBACK_ERROR(E_MASTERMAIN_LOOPSETUP, "Evaluation loop setup failed")
// FeedbackError E_MASTERMAIN_LOOP("Evaluation loop failed");
DEFINE_FEEDBACK_ERROR(E_MASTERMAIN_LOOP, "Evaluation loop failed")
// FeedbackError E_MASTERMAIN_SETUP("The master failed during system setup");
DEFINE_FEEDBACK_ERROR(E_MASTERMAIN_SETUP, "The master failed during system setup")


using namespace std;

std::string sMaster;
pid_t master_pid = 0;
jmp_buf sigPipeEnv;



/********************************************************************
 *   Kill the master process if an interrupt is received.
 *******************************************************************/
// static void signal_handler(int nSignal)
// {
//   Feedback fb("master killer");
//   if (master_pid != 0)
//     KillProcess(master_pid, sMaster, fb);
//   exit(nSignal);
// }


/********************************************************************
 *   Pass on selected signals to the master process.
 *******************************************************************/
static void
SignalPassOn(int nSignal)
{
  Feedback fb("Master signal handler");
  if (master_pid != 0)
  {
    fb.Info(1) << "Received signal " << nSignal
               << " (" << SignalToString(nSignal) << ").  Passing it on to master with pid " << master_pid << "."; 
    kill(master_pid, nSignal);
  }
  else
    fb.Info(1) << "Received signal " << nSignal
               << " (" << SignalToString(nSignal) << "), but master pid is 0, so it cannot be passed on.";
}



/********************************************************************
 *   Thread to simply pass on signals to master process, if this
 *   one is available.
 *******************************************************************/
void*
SignalPassThread(void *)
{
  std::list<int> sigsPassed;
  sigsPassed.push_back(SIGHUP);
  sigsPassed.push_back(SIGINT);
  sigsPassed.push_back(SIGQUIT);
// MPICH uses SIGUSR1 and SIGUSR2 internally.
//   sigsPassed.push_back(SIGUSR1);
//   sigsPassed.push_back(SIGUSR2);
  sigsPassed.push_back(SIGALRM);
  sigsPassed.push_back(SIGTERM);
  sigsPassed.push_back(SIGTSTP);
  sigsPassed.push_back(SIGCONT);
  sigsPassed.push_back(SIGCHLD);

  for (;;)
  {
    sigset_t sigSet;
    sigemptyset(&sigSet);
    for (std::list<int>::const_iterator itsig = sigsPassed.begin(); 
         itsig != sigsPassed.end(); itsig++)
      sigaddset(&sigSet, *itsig);

    int nSignal;
    sigwait(&sigSet, &nSignal);
    SignalPassOn(nSignal);
  }
  return 0;
}
  

/********************************************************************
 *   Signal handling function for main thread, responding only to
 *   SIGPIPE, and taking down distro system if this happens.
 *******************************************************************/
void
SignalPipe(int)
{
  siglongjmp(sigPipeEnv, 1);
}


int RunEvalLoop(int nNumSlaves, fdostream &masterWriteStdin, fdistream &masterReadStdout)
{
  
//  extern bool bMaster;
//  bMaster = true;
  
  Feedback fb("Master evaluation loop");

  Master *pMaster;
  if (DistributorLauncher::Instance().CreateDistributor(&pMaster, nNumSlaves))
    return fb.Error(E_MASTERMAIN_LOOPSETUP) << "The \"CreateDistributor\" call failed.\n";

      // Master created, unblock SIGPIPE to allow for destroying
      // all slaves if the master goes down.
      // Block all signals in all threads
  if (sigsetjmp(sigPipeEnv, 1))
  {
    fb.Warning("SIGPIPE received on master side, assuming master process has died.  "
               "Shutting down distribution system.");
    master_pid = 0;
    DistributorLauncher::Instance().DestroyDistributor(pMaster);
    return fb.Error(E_MASTERMAIN_LOOP);
  }

  sigset_t sigSet;
  sigemptyset(&sigSet);
  sigaddset(&sigSet, SIGPIPE);
  pthread_sigmask(SIG_UNBLOCK, &sigSet, 0);
  Signal(SIGPIPE, SignalPipe);
  
  std::string sMasterInputMode, sMasterOutputMode;
  int nMasterOutputLines, nMasterInputLines;
  if (Options::Instance().Option("master-input-mode", sMasterInputMode)
      || Options::Instance().Option("master-output-mode", sMasterOutputMode))
    return fb.Error(E_MASTERMAIN_LOOPSETUP) << ": Failed to get the necessary options.";

  JobReaderWriter rwWriter(sMasterInputMode), rwReader(sMasterOutputMode);

//   fb.Warning("DEBUG: Main taking a break...");
//   sleep(10);
//   fb.Warning("DEBUG: Break over!");
  size_t nJob = 0, nBatch = 0, nTotal = 0;
  pollfd pfd;
  pfd.fd = masterReadStdout.fd();
  pfd.events = POLLIN;

  typedef std::vector<std::string> TDataVec;
  TDataVec data, results;
  while (masterReadStdout.good())
  {
        // Read jobs as long as there is pending data coming from
        // the master.  Wait indefinitely for the first job, then
        // wait for 10 milliseconds before assuming there will be
        // no more data coming in this batch.  Data is not only
        // buffered in the pipe, it may also be buffered in the
        // fdstream::streambuf.  This means that data may be
        // available even though poll does not indicate anything
        // more to read from the stream.
    int nAvail = masterReadStdout.rdbuf()->in_avail(), nPollRet = -1;
    assert(nAvail >= 0);
    if (nAvail == 0)
    {
          // If only data is empty, we may have processed the
          // first portion of a batch.  Check for more input, but
          // don't wait indefinitely.
      int nTimeout = (data.empty() && results.empty()) ? -1 : 10;
      fb.Info(3, "Polling for data, timeout is ") << nTimeout << "...";
      pfd.revents = 0;
      nPollRet = poll(&pfd, 1, nTimeout);
      fb.Info(3, "Poll returned ") << nPollRet << " (timeout " << nTimeout << ").";
          // nPollRet > 0 means returned events, i.e. data
          // available.
    }
    if (nAvail > 0 || nPollRet > 0)
    {
      fb.Info(3, "Reading job data from master...");
      string sJob;
      JobReaderWriter::int_type nRet = rwReader.Read(masterReadStdout, sJob);
      if (nRet == JobReaderWriter::traits_type::eof())
      {
        fb.Info(1) << ": End-of-file when reading job " << nJob << " in batch " << nBatch 
                   << " (i.e. job " << nTotal << " in total) from master."
                   << "  Assuming the simulation is complete.";
        break;
      }
      else if (nRet)
        return fb.Error(E_MASTERMAIN_LOOP) << "Error while reading job " << nJob << " in batch " << nBatch 
                                           << " (i.e. job " << nTotal << " in total) from master.";

      data.push_back(sJob);
      nJob++;
      nTotal++;
    }
        // nPollRet == 0 means timeout, i.e. no more data coming.
        // Send for eval.  However, do not print out the results
        // immediately.  It is possible that the current job set
        // is only part of a batch (in the process of being)
        // written from the master.  In this case, the remaining
        // data were probably written while evaluation took
        // place.  Trying to write results may cause both pipes
        // to fill up, which results in a deadlock.  Instead,
        // keep evaluating and stack up the results until no more
        // data has appeared during the last evaluation step.
        // This is not 100% safe, but it is still rather likely
        // to work.  The 100% alternative is to revert to batch
        // options: simple/lines, EOF or poll.
    else if (nPollRet == 0)
    {
      if (!data.empty())
      {
        fb.Info(2) << "Read " << data.size() << " jobs from master via stdin, now sending data for evaluation...\n";
        TDataVec newResults(data.size());
        if (pMaster->Evaluate(data, newResults))
          return fb.Error(E_MASTERMAIN_LOOP) << "Distributed evaluation failed.";
        copy(newResults.begin(), newResults.end(), back_inserter(results));
        fb.Info(2, "Results received!");
      }
      else
      {
        fb.Info(2) << "Writing " << results.size() << " results to master..";

        for (size_t nJ = 0; nJ < results.size(); nJ++)
        {
          if (rwWriter.Write(masterWriteStdin, results[nJ]))
            return fb.Error(E_MASTERMAIN_LOOP) << ": Failed to write results to master.";
        }

        fb.Info(2) << "Results written to master.\n";
        results.clear();
      }
      data.clear();    
      nBatch++;
      nJob = 0;
    } 
        // Finally, nPollRet < 0 means error.
    else
      return fb.Error(E_MASTERMAIN_LOOP) << "Unexpected error in poll (value returned is "
                                         << nPollRet << ").";
  }
  
  fb.Info(1, "Simulation complete. Shutting down slaves...");
  DistributorLauncher::Instance().DestroyDistributor(pMaster);
  fb.Info(1, "Evaluation Finished.");
  return 0;
}


int 
master_main(int argc, char *argv[])
{
//   std::cerr << "WARNING: Main taking a break at the very beginning. Pid is " << getpid() << "\n";
//   sleep(10);
//   std::cerr << "Break over!\n";

  Timer timer(false);

      // Block all signals in all threads
  sigset_t sigSet;
  sigfillset(&sigSet);
  pthread_sigmask(SIG_BLOCK, &sigSet, 0);

  Feedback fb("Master main");
  int nInfoLevel;
  std::string sInfoShow, sInfoHide;
  bool bReportTime;
  if (Options::Instance().Option("verbosity", nInfoLevel)
      || Options::Instance().Option("verbosity-showonly", sInfoShow)
      || Options::Instance().Option("verbosity-dontshow", sInfoHide)
      || Options::Instance().Option("report-total-simulation-time", bReportTime))
    return fb.Error(E_MASTERMAIN_SETUP) << ": Unable to get system verbosity options.";
  fb.SetInfoLevel(nInfoLevel);
  fb.SetShowHide(sInfoShow, sInfoHide);
  timer.ReportOnDestroy(bReportTime);

      // Create signal forwarding thread
  pthread_t sigThread;
  if (pthread_create(&sigThread, 0, SignalPassThread, 0))
    return fb.Error(E_MASTERMAIN_SETUP) << ": Unable to launch signal handling thread.";

  std::string sMasterArgs;
  if (int nRet = GetChild(fb, "master", sMaster, sMasterArgs))
    return nRet;

  fb.Info(1) << "Distributor master node loaded with pid " << getpid() << " and thread id " << pthread_self << ".";

      // Create two pipes, input pipe and output pipe, and connect to
      // streams.  For Pvm, this is necessary since we fork later
      // on. For MPI, we just do it to get blocking on read.
  MemoryPipe pipeInput, pipeOutput;
  mpistream pipeInputRead(&pipeInput), pipeOutputRead(&pipeOutput);
  mpostream pipeInputWrite(&pipeInput), pipeOutputWrite(&pipeOutput);

      // Send write end of output pipe and read end of input pipe to message central
  MessageRouter::Instance().SetIOChannels(&pipeInputRead, &pipeOutputWrite);
  if (MessageRouter::Instance().StartReceiver())
    return 1;
    
  fb.Info(3, "Message router loaded.");

//   fb.Warning("Main taking a break...");
//   sleep(5);
//   fb.Warning("Break over!");

      // Send read end of output pipe to MPISender.
  MPISender sender(&argc, &argv, &pipeOutputRead);
  int nNumSlaves = sender.GetNumSlaves();
  if (sender.Start())
    return fb.Error(E_MASTERMAIN_SETUP) << ": Failed to launch MPI sender thread.\n";

//   fb.Info(1, "MPI sender loaded.");
//   fb.Warning("DEBUG: Main taking a break...");
//   sleep(5);
//   fb.Warning(1, "Break over!");

      // Send write end of input pipe to MPIReceiver.
  MPIReceiver receiver(&pipeInputWrite, sender);
  if (receiver.Start())
    return fb.Error(E_MASTERMAIN_SETUP) << ": Failed to launch MPI receiver thread.\n";

  fb.Info(2, "MPI connection channels loaded.  Starting evaluation loop...");

//   fb.Warning("DEBUG: Main taking a break...");
//   sleep(5);
//   fb.Warning("DEBUG: Break over!");

      // Connect to master as the last thing to do before evaluating,
      // to avoid killing it if any of the above processing fails.
  fb.Info(2) << "Connecting to master " << sMaster << "...";

//   signal(SIGINT, signal_handler);
//   signal(SIGTERM, signal_handler);

//   fb.Warning("DEBUG: Main taking a break...");
//   sleep(10);
//   fb.Warning("DEBUG: Break over!");

  fdostream masterWriteStdin;
  fdistream masterReadStdout;
  fb.Info(2) << "Trying to launch master program as '" << sMaster << " " << sMasterArgs << "'...";
  if (ConnectProcess(sMaster, sMasterArgs, &masterWriteStdin, &masterReadStdout, 0, &master_pid, 0))
    return fb.Error(E_MASTERMAIN_SETUP) << ": Failed to launch master! Master program was \"" << sMaster << "\".";

  fb.Info(2) << "Successfully connected to master.";

      // Only kill the master process if something goes wrong,
      // otherwise let it die a natural death.
  if (int nRet = RunEvalLoop(nNumSlaves, masterWriteStdin, masterReadStdout))
  {
    KillProcess(master_pid, sMaster, fb);
    return nRet;
  }

//       // Sleep to see if the slaves will go down because we leave the mpi subsystem time enough to pass on TERMINATE messages...
//   fb.Warning("DEBUG: Main taking a break...");
//   sleep(10);
//   fb.Warning("DEBUG: Break over!");

  fb.Info(2, "Evaluation loop complete.  Waiting for message passing subsystem to shut down..");
  pthread_join(receiver.GetThreadId(), 0);
  fb.Info(2, "MPI message receiver stopped and joined.");
  pthread_join(sender.GetThreadId(), 0);
  fb.Info(2, "MPI message sender stopped and joined.");
  pthread_join(MessageRouter::Instance().GetReceiverThreadId(), 0);
  fb.Info(2, "Message router stopped and joined.  Waiting for master program to finish.");
  pid_t waitRet = wait(0);
  if (waitRet != master_pid)
  {
    int nSleep = 30;
    fb.Warning() << "Failed to wait for master ('wait' system call returned " 
                 << waitRet << " instead of " << master_pid << " as expected)."
                 << " Will pause for " << nSleep << " seconds before going down...";
    sleep(nSleep);
  }
  else
    fb.Info(1, "Distributed evaluation complete.");

  return 0;
}
