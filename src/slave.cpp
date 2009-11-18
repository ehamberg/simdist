/********************************************************************
 *   		slave.cpp
 *   Created on Mon Feb 27 2006 by Boye A. Hoeverstad.
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

#include <simdist/slave.h>

#include <simdist/options.h>
#include <simdist/misc_utils.h>

#include <cassert>
#include <string>
#include <iostream>
#include <sstream>
#include <functional>
#include <algorithm>
#include <memory>
#include <sys/errno.h>

// FeedbackError E_SLAVE_INITTWICE("Tried to connect to more than one server.");
DEFINE_FEEDBACK_ERROR(E_SLAVE_INITTWICE, "Tried to connect to more than one server.")
// FeedbackError E_SLAVECLIENT_START("Failed to start the client");
DEFINE_FEEDBACK_ERROR(E_SLAVECLIENT_START, "Failed to start the client")
// FeedbackError E_SLAVECLIENT_CONNECT("Failed to connect to server");
DEFINE_FEEDBACK_ERROR(E_SLAVECLIENT_CONNECT, "Failed to connect to server")
// FeedbackError E_SLAVECLIENT_RECEIVE("Failed to receive message from server");
DEFINE_FEEDBACK_ERROR(E_SLAVECLIENT_RECEIVE, "Failed to receive message from server")
// FeedbackError E_SLAVECLIENT_RUN("Run loop failed");
DEFINE_FEEDBACK_ERROR(E_SLAVECLIENT_RUN, "Run loop failed")
// FeedbackError E_SLAVECLIENT_THREAD("Failed to create thread");
DEFINE_FEEDBACK_ERROR(E_SLAVECLIENT_THREAD, "Failed to create thread")
// FeedbackError E_SLAVECLIENT_WAITQUEUE("Failed to lock queue in order to wait for work");
DEFINE_FEEDBACK_ERROR(E_SLAVECLIENT_WAITQUEUE, "Failed to lock queue in order to wait for work")
// FeedbackError E_SLAVECLIENT_ABORT("Failed to abort other slaves");
DEFINE_FEEDBACK_ERROR(E_SLAVECLIENT_ABORT, "Failed to abort other slaves")
// FeedbackError E_SLAVECLIENT_JOBTAKE("Failed to pop job from job queue");
DEFINE_FEEDBACK_ERROR(E_SLAVECLIENT_JOBTAKE, "Failed to pop job from job queue")
// FeedbackError E_SLAVECLIENT_JOBSEND("Failed to send job results");
DEFINE_FEEDBACK_ERROR(E_SLAVECLIENT_JOBSEND, "Failed to send job results")
// FeedbackError E_SLAVECLIENT_JOBRECEIVE("Failed to receive job results");
DEFINE_FEEDBACK_ERROR(E_SLAVECLIENT_JOBRECEIVE, "Failed to receive job results")
// FeedbackError E_SLAVECLIENT_PROCESSRESULTS("Failed to process job results");
DEFINE_FEEDBACK_ERROR(E_SLAVECLIENT_PROCESSRESULTS, "Failed to process job results")
// FeedbackError E_SLAVECLIENT_TERMINATE("Failed to terminate slave");
DEFINE_FEEDBACK_ERROR(E_SLAVECLIENT_TERMINATE, "Failed to terminate slave")


static const int message_version = 2;

SlaveClientFactory::SlaveClientFactory()
    : m_fb("SlaveClientFactory")
    , m_nNumSlaves(0)
{

}



/*static*/ SlaveClientFactory&
SlaveClientFactory::Instance()
{
  static SlaveClientFactory instance;
  return instance;
}



/********************************************************************
 *   Create slaves and connect them to a server.  For each server
 *   successfully loaded, push a SlaveClient onto the slaveClients
 *   vector.  The vector may or may not be empty upon entry to this
 *   routine.  The size change will indicate the number of slaves
 *   successfully created.  
 *
 *   !!- sSlaveProgram indicates the evaluation program that the slave
 *   servers should connect to.  This is only relevant for stdio type
 *   slaves.
 *******************************************************************/
int 
SlaveClientFactory::CreateSlaves(const std::vector<std::string> &servers, 
                                 const std::vector<std::string> &slavePrograms, 
                                 const std::vector<std::string> &slavesArgs, 
                                 std::vector<SlaveClient*> &slaveClients,
                                 JobQueue *pJobQueue, ThreadBarrier *pBarrier)
{
  if (servers.size() != slavePrograms.size())
    return m_fb.Error(E_INTERNAL_LOGIC) << "server list and server program list are not the same size.";

      // Spawn slave server tasks. 
  int nFailures = 0, nSuccesses = 0;
  for (std::size_t nSrv = 0; nSrv < servers.size(); nSrv++)
  {
    std::auto_ptr<SlaveClient> slave(new SlaveClient());
    if (slave->Start(servers[nSrv], slavePrograms[nSrv], slavesArgs[nSrv], pJobQueue, pBarrier))
    {
      m_fb.Warning() << "Failure " << ++nFailures << " to start to a slave.  Server: " 
                     << servers[nSrv] << ", program: " << slavePrograms[nSrv] << ".";
      continue;
    }
    slaveClients.push_back(slave.release());
    nSuccesses++;
  }
  m_nNumSlaves += nSuccesses;

  m_fb.Info(1) << nSuccesses << " slaves created and ready (out of " << servers.size() << " requested)";
  return 0;
}


/********************************************************************
 *   Same as the above version, only this one launches a number of
 *   slaves running the same program with the same startup arguments.
 *******************************************************************/
int 
SlaveClientFactory::CreateSlaves(const std::vector<std::string> &servers, 
                                 const std::string &sSlaveProgram,
                                 const std::string &sSlaveArgs,
                                 std::vector<SlaveClient*> &slaveClients,
                                 JobQueue *pJobQueue, ThreadBarrier *pBarrier)
{
  const std::vector<std::string> slavePrograms(servers.size(), sSlaveProgram); 
  const std::vector<std::string> slavesArgs(servers.size(), sSlaveArgs); 
  return CreateSlaves(servers, slavePrograms, slavesArgs, slaveClients, pJobQueue, pBarrier);
}


size_t
SlaveClientFactory::GetNumSlaves() const
{
  return m_nNumSlaves;
}


SlaveClient::SlaveClient()
    : m_pBarrier(0)
    , m_pJobQueue(0)
    , m_fb("SlaveClient")
    , m_rw(JobReaderWriter::bytecount)
    , m_fWaitFactor(100)
    , m_nNumJobsCompleted(0)
    , m_dTotalWorkTime(0)
{
  if (Options::Instance().Option("slave-wait-factor", m_fWaitFactor))
    m_fb.Warning("Failed to get option slave-wait-factor, will default to ") 
      << m_fWaitFactor;
}



SlaveClient::~SlaveClient()
{
}


typedef struct TSlaveClientThreadDataVar
{
  std::string sServer;
  std::string sSlaveProgram;
  std::string sSlaveArgs;
  JobQueue *pJobQueue;
  SlaveClient *pClient;
  ThreadBarrier *pBarrier;
} TSlaveClientThreadData;


void *slaveclient_thread_func(void *pArg)
{
  std::auto_ptr<TSlaveClientThreadData> ptd(static_cast<TSlaveClientThreadData*>(pArg));

  ptd->pBarrier->Add(pthread_self());

  ptd->pClient->m_fb.RegisterThreadDescription("Slaveclient");
  int nRet = ptd->pClient->m_fb.ErrorIfNonzero(ptd->pClient->ConnectServer(ptd->sServer, ptd->sSlaveProgram, ptd->sSlaveArgs), E_SLAVECLIENT_CONNECT);
  if (!nRet)
    nRet = ptd->pClient->m_fb.ErrorIfNonzero(ptd->pClient->Run(ptd->pJobQueue), E_SLAVECLIENT_RUN);

  ptd->pClient->m_fb.Info(2, "Slave client finished processing and returned with code ") << nRet;
  ptd->pBarrier->Erase(pthread_self());

  return reinterpret_cast<void*>(nRet);
}


/********************************************************************
 *   Start the slave: Launch a separate thread which connects to
 *   server and calls Run.
 *******************************************************************/
int
SlaveClient::Start(const std::string &sServer, const std::string &sEvalProgram, 
                   const std::string &sSlaveArgs, JobQueue *pJobQueue, ThreadBarrier *pBarrier)
{
 // Allocate on heap since local variable may go out of scope
 // before thread starts.
  TSlaveClientThreadData td = { sServer, sEvalProgram, sSlaveArgs, pJobQueue, this, pBarrier }, *ptd = new TSlaveClientThreadData;
  (*ptd) = td;

      // Create with small stack to avoid excessive use of memory.
  const int stack_size_bytes = 1024 * 50;
  pthread_attr_t   stackSizeAttribute;
  pthread_attr_init (&stackSizeAttribute);
  if (pthread_attr_setstacksize (&stackSizeAttribute, stack_size_bytes))
      m_fb.Warning("Failed to set stack size attribute on slave thread.");

  return m_fb.ErrorIfNonzero(pthread_create(&m_threadId, &stackSizeAttribute, slaveclient_thread_func, ptd),
                             E_SLAVECLIENT_THREAD);
}


/********************************************************************
 *   Connect to the indicated slave server: Send a connect message and
 *   wait for ready signal.  Called by Start.
 *******************************************************************/
int
SlaveClient::ConnectServer(const std::string &sServer, const std::string &sSlaveProgram, std::string sSlaveArgs)
{
  if (!m_sServer.empty())
    return m_fb.Error(E_SLAVE_INITTWICE);

  m_fb.Info(3) << "Attempting to connect to " << sServer << " and run " << sSlaveProgram << "..."; 
  m_sServer = sServer;

  std::string sMasterOutputMode, sMasterInputMode, sSlaveIdTag;
  int nMasterInputLines;
  if (Options::Instance().Option("master-output-mode", sMasterOutputMode)
      || Options::Instance().Option("master-input-mode", sMasterInputMode)
      || Options::Instance().Option("slave-id-tag", sSlaveIdTag))
    return m_fb.Error(E_SLAVECLIENT_CONNECT);

  FindReplaceAll(sSlaveArgs, sSlaveIdTag, sServer);

  std::stringstream ss;
  ss << "CONNECT" << "\n" 
     << sServer << "\n" 
     << message_version << "\n" 
     << sMasterOutputMode << "\n"
     << sMasterInputMode << "\n"
     << sSlaveProgram << "\n"
     << sSlaveArgs << "\n";

  if (m_mp.Send(sServer, ss.str()))
    return m_fb.Error(E_SLAVECLIENT_CONNECT) 
      << ". Server: " << sServer << ", program: " << sSlaveProgram;

  std::string sMessage, sServerReply, sTag;
  while (sTag != "READY")
  {
    if (m_mp.Receive(sServer, sMessage))
      return m_fb.Error(E_SLAVECLIENT_CONNECT) << ", could not receive connect confirmation.";
    std::stringstream ss(sMessage);
    ss >> sTag >> sServerReply;
    if (sTag == "READY" && sServerReply != sServer)
      return m_fb.Error(E_SLAVECLIENT_CONNECT) << ", received malformed ready message: Server mismatch! Was \""
                                               << sServerReply << "\", should be \"" << sServer << "\".";
  }

  m_fb.SetIdentifier(m_fb.Identify() + "-" + sServer);
  m_fb.Info(2) << "Successfully connected to " << sServer << "!"; 
  return 0;
}



/********************************************************************
 *   This routine starts the slave client loop.  This routine only
 *   returns on error or when the queue has been closed.  The slave
 *   server should already be up and running when this routine is
 *   called.
 *******************************************************************/
int
SlaveClient::Run(JobQueue *pJobQueue)
{
  m_pJobQueue = pJobQueue;

  bool bQueueClosed = false;
  while (!bQueueClosed)
  {
    AutoMutex mtx;
    if (pJobQueue->AcquireMutex(mtx))
      return m_fb.Error(E_SLAVECLIENT_WAITQUEUE);
    while (pJobQueue->empty() && !(bQueueClosed = pJobQueue->Closed()))
      pJobQueue->Wait();

    if (!bQueueClosed)
    {
          // Pop job, add ourselves to set of workers, push job.
      bool bJobsTaken;
      if (TakeJobs(bJobsTaken))
        return m_fb.Error(E_SLAVECLIENT_JOBTAKE);

      if (!bJobsTaken)
        continue;

          // Now we release the mutex and send the jobs to the slave.
      mtx.Unlock();
      if (SendJobs())
        return m_fb.Error(E_SLAVECLIENT_JOBSEND);
          // Receive results or abort message.
      bool bShutdown;
      while (!m_currentJobs.empty())
      {
        if (ReceiveJobs(bShutdown))
          return m_fb.Error(E_SLAVECLIENT_JOBRECEIVE);
        if (bShutdown)
          return 0;
      }
    }
  }
  return 0;
}


/********************************************************************
 *   Take a set of jobs: Pop from the end of the queue, store ID of
 *   worker in set, push to the beginning of the queue, and store a
 *   copy to send to slave server.  This may perhaps be improved in
 *   terms of memory usage by keeping not copies of the jobs, but only
 *   pointers in the queue.  Keep in mind that SendJobs works outside
 *   the guarded section, so anything may happen to the queue once we
 *   leave here.
 *******************************************************************/
int
SlaveClient::TakeJobs(bool &bJobsTaken)
{
  if (m_pJobQueue->empty())
    return m_fb.Error(E_INTERNAL_LOGIC) << "Job queue is empty in TakeJobs routine.";

  JobQueueElement *pJob = &(m_pJobQueue->front());
  if (!pJob->workers.empty())
  {
    double dAvgTime = ((m_nNumJobsCompleted && m_dTotalWorkTime) ? m_dTotalWorkTime / m_nNumJobsCompleted : 1);
    double dWait = dAvgTime * m_fWaitFactor * m_pJobQueue->NumJobsPerSend();
    dWait -= difftime(time(0), pJob->timeLastStart);
    if (dWait > 0)
    {
      bJobsTaken = false;
      m_fb.Info(2) << "Waiting " << dWait << " seconds before double-processing a job. "
                   << "First job in queue is " + pJob->sJobID + ", which is currently being processed by "
                   << pJob->workers.size() << " other slave(s): " + pJob->WorkersToString() + ".";
      m_pJobQueue->Wait(dWait);
      return 0;
    }
    else
      m_fb.Info(2) << "Now double-processing job " + pJob->sJobID 
                   << ", which is currently being processed by "
                   << pJob->workers.size() << " other slave(s): " + pJob->WorkersToString() + ".";
  }
  
  bJobsTaken = true;
  m_fTimeJobsTaken = time(0);

  if (!(m_currentJobs.size() < m_pJobQueue->NumJobsPerSend()))
    return m_fb.Error(E_INTERNAL_LOGIC) << ": Slave tried to take a new job from queue, while still processing one or more old ones (have "
                                        << m_currentJobs.size() << " jobs, should have at most " << m_pJobQueue->NumJobsPerSend() << ").";

  do
  {
    pJob->workers.insert(this);
    pJob->timeLastStart = m_fTimeJobsTaken;
    m_currentJobs[pJob->sJobID] = *pJob;
    m_pJobQueue->push_back(*pJob);
    m_pJobQueue->pop_front();
    pJob = &(m_pJobQueue->front());
  } 
  while (m_currentJobs.size() < m_pJobQueue->NumJobsPerSend()
         && pJob->workers.empty()); // This also takes care of the situation where this slave has taken all jobs in the queue.

  m_fb.Info(3) << "Took " << m_currentJobs.size() << " job(s) from job queue.";
  return 0;
}



/********************************************************************
 *   Send the current set of jobs to the slave server.  Return 0 when
 *   the jobs are successfully sent.
 *******************************************************************/
int 
SlaveClient::SendJobs()
{
  std::stringstream ss;
  ss << "JOB" << "\n" 
     << m_sServer << "\n" 
     << m_currentJobs.size() << "\n";
  for (TJobMap::const_iterator jit = m_currentJobs.begin(); jit != m_currentJobs.end(); jit++)
  {
    ss << jit->first << "\n";
    m_rw.Write(ss, jit->second.sJobData);
  }
  return m_mp.Send(m_sServer, ss.str());
}
 
 

/********************************************************************
 *   Block waiting for results.  Return true both on received results
 *   and server aborted ready.
 *******************************************************************/
int
SlaveClient::ReceiveJobs(bool &bShutdown)
{
  bShutdown = false;
  std::string sMessage;
  if (m_mp.Receive(m_sServer, sMessage))
    return m_fb.Error(E_SLAVECLIENT_RECEIVE);

  std::stringstream ss(sMessage);
  std::string sTag, sServer;
  ss >> sTag >> sServer;
  if (sServer != m_sServer)
    return m_fb.Error(E_SLAVECLIENT_RECEIVE) 
      << ", received message from the wrong server.  Should be: " 
      << m_sServer << ", was: " << sServer << ".";

  if (sTag == "RESULTS")
  {
    int nNumResults;
    ss >> nNumResults;
    for (int nRes = 0; nRes < nNumResults; nRes++)
    {
      std::string sJobID, sResults, sTime, sLine;
      float fTime;
      ss >> sJobID >> fTime;
      std::getline(ss, sLine); // Chomp endline
      if (m_rw.Read(ss, sResults)
          || ProcessResults(sJobID, sResults, fTime))
        return m_fb.Error(E_SLAVECLIENT_RECEIVE); 
    }
        // Update average processing time.  Note that this is
        // different from processing time spent on the server, as
        // recorded in the job messages.
    m_nNumJobsCompleted += nNumResults;
    m_dTotalWorkTime += difftime(time(0), m_fTimeJobsTaken);
    return 0;
  } 
  else if (sTag == "ABORTED_READY")
  {
    std::string sJobID;
    ss >> sJobID;
    m_fb.Info(2) << "Server " << m_sServer 
                 << " was aborted on job " << sJobID 
                 << ". Ready for more work.";
  } 
  else if (sTag == "FAIL")
  {
    int nCode;
    std::string sDesc, sLine;
    ss >> nCode;
    std::getline(ss, sLine);
    int nLineCtr = 0;
    while (std::getline(ss, sLine))
      sDesc += (nLineCtr++ ? "\n" : "") + sLine; // Can't test on sMessage.empty(). See comment in utils/ReadJob.
    m_fb.Warning() << "Server " << m_sServer << " failed with error code "
                   << nCode << " and description: " << sDesc;
  }
  else if (sTag == "SHUTDOWN_MASTER")
    bShutdown = true;
  else 
    m_fb.Warning() << "Received unexpected message: " << sTag;

  return 0;
}



/********************************************************************
 *   Process received results from server: Add results to result set,
 *   and abort all other slaves working on the same job.
 *******************************************************************/
int 
SlaveClient::ProcessResults(const std::string &sJobID, const std::string &sResults, float fTime)
{
  TJobMap::const_iterator jit = m_currentJobs.find(sJobID);
  if (jit == m_currentJobs.end())
  {
    m_fb.Info(1) << "Received results from a job with an ID not in the list of current jobs.";
    return 0;
  }
  m_currentJobs.erase(sJobID);

  AutoMutex mtx;
  if (m_pJobQueue->AcquireMutex(mtx))
    return m_fb.Error(E_SLAVECLIENT_PROCESSRESULTS) << ", couldn't lock job queue.";

      // Find job in queue, since the set of workers may have changed
      // since we took the job.
  for(JobQueue::iterator job_it = m_pJobQueue->begin(); job_it != m_pJobQueue->end(); job_it++)
  {
        // Job found in queue
    if (sJobID == job_it->sJobID)
    {
          // Store results
      job_it->sResults = sResults;

          // Abort all other clients working on the current job
      if (AbortSlaves(job_it->workers, job_it->sJobID))
        return m_fb.Error(E_SLAVECLIENT_PROCESSRESULTS) 
          << ", failed to abort the other slaves on the same job.";

          // Store results
      job_it->workers.clear();
      m_pJobQueue->ResultSet().insert(*job_it);

          // Remove job from job queue
      m_pJobQueue->erase(job_it);
      
          // Wake up the master if necessary
      if (m_pJobQueue->empty())
        m_pJobQueue->Signal();
        
      return 0;
    }
  }
      // We get thus far only if the job doesn't exist in the queue.
      // This could indicate a timing error somewhere, but probably
      // simply means that two servers completed processing the same
      // job.
  m_fb.Info(2) << "Received results for a job not found in job queue. Job ID: " << sJobID;
  return 0;
}


/********************************************************************
 *   Abort all other slaves on the current job.  The caller should
 *   have exclusive Pvm AND job queue access.
 *******************************************************************/
int
SlaveClient::AbortSlaves(const std::set<SlaveClient*> &workers, const std::string &sJobID)
{
  for(std::set<SlaveClient*>::const_iterator slave_it = workers.begin(); slave_it != workers.end(); slave_it++)
  {
    if ((*slave_it) == this)
      continue;

    const std::string sServer = (*slave_it)->m_sServer;
    std::stringstream ss;
    ss << "ABORT" << "\n"
       << sServer << "\n"
       << sJobID;
    if (m_mp.Send(sServer, ss.str()))
      return m_fb.Error(E_SLAVECLIENT_ABORT) << ", couldn't send abort message to server " << sServer << ".";
  }      

  return 0;  
}



/********************************************************************
 *   Attempt to nicely terminate a slave by sending a TERMINATE
 *   message to the server.  Highly optimistic, since we don't
 *   know if the slave thread is running or not.
 *
 *   !!- TODO: Consider adding state variable to the slave thread.
 *******************************************************************/
int
SlaveClient::Terminate()
{
  std::stringstream ss;
  ss << "TERMINATE" << "\n" 
     << m_sServer;

  Feedback fb("SlaveClient::Terminate");
  MessagePasser mp;
  return fb.ErrorIfNonzero(mp.Send(m_sServer, ss.str()), E_SLAVECLIENT_TERMINATE)
    << ". Server: " << m_sServer;
}


std::string
SlaveClient::Server() const
{
  return m_sServer;
}


//===========================================================================




// SlaveServerReceiver::SlaveServerReceiver()
//     : Reporting("SlaveServerReceiver")
//     , m_bServerThreadRunning(false)
// {
// }


// SlaveServerReceiver::~SlaveServerReceiver()
// {
//   if (m_bServerThreadRunning)
//   {
//     SlaveServer &srv = SlaveServer::Instance();
//     AutoMutex mtx;
//     if (srv.AcquireMutex(mtx))
//       Error(E_SERVER_LOCK);
//     if (!srv.Ready() && AbortServer(mtx.GetLockedMutex()))
//       Error(E_SERVER_ABORT);
//     if (KillServer())
//       Error(E_SERVER_KILL);
//   }
// }


// /********************************************************************
//  *   Send a ready message to parent.  Return nonzero on failure. 
//  *
//  *   The mutex should be locked upon entry to guarantee exclusive
//  *   access to pvm send.
//  *******************************************************************/
// int
// SlaveServerReceiver::SendReadyMessage() const
// {
//   pvm_stream s(true);
//   if (!(s << PVM_MESSAGE_VERSION).good())
//     return Error(E_PVM_PACK);
//   if (pvm_send(pvm_parent(), PvmControl::PVM_READY) < 0)
//     return Error(E_PVM_SEND);
//   return 0;
// }

// /********************************************************************
//  *   Process a message with tag nMsgTag.  Set bTerminate to true if
//  *   the message processing loop should terminate.  Return 0 to
//  *   continue the message processing, nonzero to terminate due to a
//  *   nonrecoverable error.  This function is called from the receiver
//  *   thread, and sends pvm messages.  This means that both threads
//  *   will send through pvm, and synchronization will be necessary.
//  *******************************************************************/
// int 
// SlaveServerReceiver::ProcessMessage(int nMsgTag, bool &bTerminate)
// {
//   bTerminate = false;
//   int nMsgVer;
//   pvm_stream s;
//   if (!(s >> nMsgVer).good())
//     return Error(E_PVM_UNPACK);

//   if (nMsgVer != PVM_MESSAGE_VERSION)
//   {
//     std::stringstream ss;
//     ss << "Slave server received data with invalid version.  Slave server Tid: " << pvm_mytid()
//        << ".  Version: " << nMsgVer << " (should be " << PVM_MESSAGE_VERSION << ")";
//     Warning(ss.str());
//     return 0;
//   }

//   AutoMutex mtx;
//   if (SlaveServer::Instance().AcquireMutex(mtx))
//     return Error(E_MUTEX_LOCK);

//   switch(nMsgTag)
//   {
//       case PvmControl::PVM_GETREADY:
//         if (CreateServer(mtx.GetLockedMutex()))
//           return Error(E_SERVER_CREATE);
//         if (SendReadyMessage())
//           return Error(E_PVM_SEND);
//         break;
//       case PvmControl::PVM_ABORT:
//         if (AbortServer(mtx.GetLockedMutex()))
//           return Error(E_SERVER_ABORT);
//         if (SendReadyMessage())
//           return Error(E_PVM_SEND);
//         break;
//       case PvmControl::PVM_JOBDATA:
//         {
//               //!!-TODO Check that server actually exists before
//               //!!- sending job.  See also comment in header at
//               //!!- m_bServerThreadRunning
//           std::string sJobData;
//           int nJobID;
//           pvm_stream s;
//           if (!(s >> nJobID >> sJobData).good())
//             return Error(E_PVM_UNPACK);
//           if (SlaveServer::Instance().SetJobData(nJobID, sJobData))
//             return Error(E_SERVER_SETJOB);
//         }
//         break;
//       case PvmControl::PVM_TERMINATE:
//         bTerminate = true;
//         if (AbortServer(mtx.GetLockedMutex()))
//           return Error(E_SERVER_ABORT);
//         if (KillServer())
//           return Error(E_SERVER_KILL);
//         break;
//       default:
//         {
//           std::stringstream ss;
//           ss << "Received pvm data with unknown message tag: " << nMsgTag;
//           Warning(ss.str());
//         }
//   }
//   return 0;
// }

// SlaveServerReceiver SlaveServerReceiver::m_instance;

// /*static*/ SlaveServerReceiver& 
// SlaveServerReceiver::Instance()
// {
//   return m_instance;
// }



// /********************************************************************
//  *   Start the listening loop.  This will not return until a terminate
//  *   message is sent.
//  *******************************************************************/
// int 
// SlaveServerReceiver::Run(int argc, char *argv[])
// {
//       // Two in one: Load the pvm subsystem and inform the user.
//   std::stringstream ss;
//   ss << "Slave server receiver " << pvm_mytid() << " is up and running";
//   Info(ss.str());

//       // Store argc and argv in order to pass to future SlaveServer
//       // threads.
//   m_argc = argc;
//   m_argv = argv;

//   while (true)
//   {
//     int nBufID = pvm_recv(-1, -1);
//     if (nBufID < 0)
//       return Error(E_PVM_RECV);

//     int nBytes, nMsgTag, nTid;
//     if (pvm_bufinfo(nBufID, &nBytes, &nMsgTag, &nTid) < 0)
//       return Error(E_PVM_BUFINFO);

//     bool bTerminate = false;
//     if (ProcessMessage(nMsgTag, bTerminate))
//       return Error(E_SERVER_RECV);
//     if (bTerminate)
//       return 0;
//   }
// }


// /********************************************************************
//  *   Abort a server thread.  Implementation-wise, this implies first
//  *   setting a terminate flag, then waiting for a while, next
//  *   cancelling the working thread and the restarting a fresh thread.
//  *   The fact that a new server thread is created may include an
//  *   overhead if the server is in fact aborted as part of the
//  *   termination process.  This is however a side effect of the way
//  *   the abort process is implemented, and should therefore not have
//  *   an effect for the caller.  Besides, termination only happens once
//  *   during a run.
//  *
//  *   The mutex should be locked upon entry.
//  *******************************************************************/
// int 
// SlaveServerReceiver::AbortServer(pthread_mutex_t *pLockedMutex)
// {
//   static const int server_wait_abort_sec = 10;

//   if (m_bServerThreadRunning)
//   {
//     SlaveServer &srv = SlaveServer::Instance();
//     if (!srv.Ready())
//     {
//       Info("Aborting server..");
//       srv.SetAbortFlag();
//       srv.Wait(pLockedMutex, server_wait_abort_sec);
//     }
//     if (!srv.Ready())
//     {
//       if (KillServer())
//         return Error(E_SERVER_KILL);
//       if (CreateServer(pLockedMutex))
//         return Error(E_SERVER_CREATE);
//       Info("Server didn't respond to abort.  Killed and recreated"); 
//     } 
//     else
//       Info("Server successfully aborted");
//   }
//   else
//   {
//     Warning("Attempted to abort a non-existing server.  Server will be created");
//     if (CreateServer(pLockedMutex))
//       return Error(E_SERVER_CREATE);
//   }

//   assert(m_bServerThreadRunning && SlaveServer::Instance().Ready());
//   return 0;
// }


// /********************************************************************
//  *   Forcefully kill the server thread.  Whether it is ready or not,
//  *   just take it down.  Mutex should have been acquired long ago.
//  *******************************************************************/
// int 
// SlaveServerReceiver::KillServer()
// {
//   if (!m_bServerThreadRunning)
//   {
//     Warning("Tried to kill a server that wasn't running.");
//     return 0;
//   }

//   if (pthread_cancel(m_serverThread))
//     return Error(E_SERVER_CANCEL);

//   if (pthread_join(m_serverThread, 0))
//     return Error(E_SERVER_JOIN);

//   m_bServerThreadRunning = false;
//   return 0;
// }


// typedef struct TSlaveServerThreadDataVar
// {
//   int argc;
//   char **argv; //!!- TODO: This causes internal compiler error if declared as 'char *argv[]'.  Consider bug report.
// } TSlaveServerThreadData;

// void*
// slave_server_thread_routine(void *thread_data)
// {
//   TSlaveServerThreadData *td = reinterpret_cast<TSlaveServerThreadData*>(thread_data);
//   SlaveServer::Instance().Run(td->argc, td->argv);
//       //!!- m_bServerThreadRunning = false; This obviously doesn't work, but see comments at declaration in header
//   return 0;
// }

// /********************************************************************
//  *   Create a new server thread.  This should not return until the
//  *   server thread is up and running.
//  *******************************************************************/
// int 
// SlaveServerReceiver::CreateServer(pthread_mutex_t *pLockedMutex)
// {
//   if (m_bServerThreadRunning)
//     return Error(E_SERVER_EXISTS);

//   TSlaveServerThreadData td = { m_argc, m_argv };
//   if (pthread_create(&m_serverThread, 0, slave_server_thread_routine, &td))
//     return Error(E_THREAD_CREATE);

//   while (!SlaveServer::Instance().Ready())
//     if (SlaveServer::Instance().Wait(pLockedMutex))
//       return Error(E_COND_WAIT);

//   return 0;
// }


// SlaveServer::SlaveServer()
//     : m_bReady(false), m_bAbort(false), m_bNewData(false)
//     , m_pSemaphore(new pthread_cond_t)
// {
//       //!!- As usual, no error handling if init fails.  But it won't.
//   int nInit = pthread_cond_init(m_pSemaphore, 0);
//   assert(nInit == 0 || !Error(E_COND_INIT));
// }


// SlaveServer::~SlaveServer()
// {
//   int nDest = pthread_cond_destroy(m_pSemaphore);
//   assert(nDest == 0 || !Error(E_COND_DESTROY));
//   delete m_pSemaphore;
// }


// SlaveServer SlaveServer::m_instance;

// /*static*/ SlaveServer& 
// SlaveServer::Instance()
// {
//   return m_instance;
// }


// /********************************************************************
//  *   Server processing loop.  This is the candidate for future
//  *   extensions.  At the moment, it loops waiting for new data.  When
//  *   data arrives, it is printed to stdout, and then a line of input
//  *   is expected, which is sent to the parent as result.
//  *******************************************************************/
// int 
// SlaveServer::Run(int argc, char *argv[])
// {
//   const int max_line_len = 1024;

//   std::stringstream ss;
//   ss << "Slave server " << pvm_mytid() << "received " << argc 
//      << " arguments, and they were as follows: ";
//   for (int arg = 0; arg < argc; arg++)
//     ss << "\t" << argv[arg] << "\n";
//   ss << "Trying to load evaluating app as specified in arguments";
//   Info(ss.str());

//   std::string sEvalProgCmdLine;
//   for (int nArg = 0; nArg < argc; nArg++)
//     sEvalProgCmdLine += argv[nArg] + std::string(" ");

//   FILE *pEvalProg = popen(sEvalProgCmdLine.c_str(), "rw");
//   if (!pEvalProg)
//     return Error(E_EVALPROG_OPEN);

//   m_bReady = true;
//   pthread_cond_signal(m_pSemaphore);

//   AutoMutex mtx;
//   if (AcquireMutex(mtx))
//     return Error(E_MUTEX_LOCK);

//   ss.clear();
//   ss << "Slave server " << pvm_mytid() << " is up and running";
//   Info(ss.str());


//   while (!m_bAbort)
//   {
//     while (!m_bNewData)
//       if (Wait(mtx.GetLockedMutex()))
//         return Error(E_COND_WAIT);
    
//     std::string sJobData = m_sJobData;
//     int nJobID = m_nJobID;
//     m_bNewData = false;
//     m_bReady = false;
//     mtx.Unlock();


//     if (fputs(sJobData.c_str(), pEvalProg) == EOF)
//       return Error(E_EVALPROG_WRITE);

//     std::vector<char> sLine(max_line_len + 1);
//     char *szRes = fgets(&sLine[0], max_line_len, pEvalProg);
//     if (!szRes)
//       return Error(E_EVALPROG_READ);
//     std::string sResults = &sLine[0];

//     if (AcquireMutex(mtx))
//       return Error(E_MUTEX_LOCK);
//     pvm_stream s(true);
//     if (!(s << PVM_MESSAGE_VERSION << nJobID << sResults).good())
//       return Error(E_PVM_PACK);
//     if (pvm_send(pvm_parent(), PvmControl::PVM_RESULTS) < 0)
//       return Error(E_PVM_SEND);
//     m_bReady = true;
//   }
//   return 0;
// }


// /********************************************************************
//  *   Indicates to the server that it should abort.
//  *
//  *   Called from within mutex lock.
//  *******************************************************************/
// void 
// SlaveServer::SetAbortFlag()
// {
//   m_bAbort = true;
// }


// /********************************************************************
//  *   Transfer new job data from the receiver to the actual server.
//  *   This function then signals the SlaveServer, so it wakes up and
//  *   starts processing the job data.
//  *
//  *   Calles from within mutex lock.
//  *******************************************************************/
// int 
// SlaveServer::SetJobData(int nJobID, const std::string &sJobData)
// {
//   if (!m_bReady || m_bNewData)
//     return Error(E_SERVER_NOTREADY);

//   m_nJobID = nJobID;
//   m_sJobData = sJobData;

//   return WakeUp();
// }


// bool 
// SlaveServer::Ready() const
// {
//   return m_bReady;
// }


// int
// SlaveServer::Wait(pthread_mutex_t *pLockedMutex, int nTimeoutSecs /*=0*/)
// {
//       //!!-TODO Refactor this to avoid duplication.
//   assert(m_pSemaphore && pLockedMutex);

//   if (nTimeoutSecs > 0)
//   {
//     time_t cur = time(0);
//     timespec ts = { cur + nTimeoutSecs, 0 };
//     int nRet = pthread_cond_timedwait(m_pSemaphore, pLockedMutex, &ts);
//     if (nRet && nRet != ETIMEDOUT)
//       return Error(E_COND_WAIT);
//   }
//   else
//     if(pthread_cond_wait(m_pSemaphore, pLockedMutex))
//       return Error(E_COND_WAIT);

//   return 0;
// }


// int
// SlaveServer::WakeUp()
// {
//   if (pthread_cond_signal(m_pSemaphore))
//     return Error(E_COND_SIGNAL);
//   return 0;
// }
