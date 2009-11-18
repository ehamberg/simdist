/********************************************************************
 *   		slave.h
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
 *   Slaves: Slave clients, slave servers, and slave client
 *   factory.  The clients reside on the master node, the slave
 *   servers reside on the host/slave nodes (optional), and connect to
 *   the software doing the actual evaluation.
 *******************************************************************/

#if !defined(__SLAVE_H__)
#define __SLAVE_H__

#include "jobqueue.h"
#include "messages.h"

#include "syncutils.h"
#include "feedback.h"
#include "io_utils.h"

#include <pthread.h>

#include <string>
#include <vector>
#include <ios>

// extern FeedbackError E_SLAVE_INITTWICE;
DECLARE_FEEDBACK_ERROR(E_SLAVE_INITTWICE)
// extern FeedbackError E_SLAVECLIENT_START;
DECLARE_FEEDBACK_ERROR(E_SLAVECLIENT_START)
// extern FeedbackError E_SLAVECLIENT_CONNECT;
DECLARE_FEEDBACK_ERROR(E_SLAVECLIENT_CONNECT)
// extern FeedbackError E_SLAVECLIENT_RECEIVE;
DECLARE_FEEDBACK_ERROR(E_SLAVECLIENT_RECEIVE)
// extern FeedbackError E_SLAVECLIENT_RUN;
DECLARE_FEEDBACK_ERROR(E_SLAVECLIENT_RUN)
// extern FeedbackError E_SLAVECLIENT_THREAD;
DECLARE_FEEDBACK_ERROR(E_SLAVECLIENT_THREAD)
// extern FeedbackError E_SLAVECLIENT_WAITQUEUE;
DECLARE_FEEDBACK_ERROR(E_SLAVECLIENT_WAITQUEUE)
// extern FeedbackError E_SLAVECLIENT_ABORT;
DECLARE_FEEDBACK_ERROR(E_SLAVECLIENT_ABORT)
// extern FeedbackError E_SLAVECLIENT_JOBTAKE;
DECLARE_FEEDBACK_ERROR(E_SLAVECLIENT_JOBTAKE)
// extern FeedbackError E_SLAVECLIENT_JOBSEND;
DECLARE_FEEDBACK_ERROR(E_SLAVECLIENT_JOBSEND)
// extern FeedbackError E_SLAVECLIENT_JOBRECEIVE;
DECLARE_FEEDBACK_ERROR(E_SLAVECLIENT_JOBRECEIVE)
// extern FeedbackError E_SLAVECLIENT_PROCESSRESULTS;
DECLARE_FEEDBACK_ERROR(E_SLAVECLIENT_PROCESSRESULTS)
// extern FeedbackError E_SLAVECLIENT_TERMINATE;
DECLARE_FEEDBACK_ERROR(E_SLAVECLIENT_TERMINATE)


class SlaveClient;

class SlaveClientFactory
{
  SlaveClientFactory();
  Feedback m_fb;
  size_t m_nNumSlaves;
public:
  static SlaveClientFactory& Instance();
      // Create slaves with potentially different programs or startup arguments.
  int CreateSlaves(const std::vector<std::string> &servers, 
                   const std::vector<std::string> &slavePrograms,
                   const std::vector<std::string> &slavesArgs,
                   std::vector<SlaveClient*> &slaveClients,
                   JobQueue *pJobQueue, ThreadBarrier *pBarrier);
      // Create "identical" slaves.
  int CreateSlaves(const std::vector<std::string> &servers, 
                   const std::string &sSlaveProgram,
                   const std::string &sSlaveArgs,
                   std::vector<SlaveClient*> &slaveClients,
                   JobQueue *pJobQueue, ThreadBarrier *pBarrier);
  size_t GetNumSlaves() const;
};



class SlaveClient
{
protected:
  std::string m_sServer;
  pthread_t m_threadId;
  ThreadBarrier *m_pBarrier;

  JobQueue *m_pJobQueue;
  typedef std::map<std::string, JobQueueElement> TJobMap;
  TJobMap m_currentJobs; // Keyed on job ID.

  MessagePasser m_mp;
  Feedback m_fb;
  JobReaderWriter m_rw;

  float m_fWaitFactor;
  int m_nNumJobsCompleted;
  double m_dTotalWorkTime;
  time_t m_fTimeJobsTaken;
  
  int ConnectServer(const std::string &sServer, const std::string &sSlaveProgram, std::string sSlaveArgs);
  int Run(JobQueue *pJobQueue);

  int TakeJobs(bool &bJobsTaken);
  int SendJobs();
  int ReceiveJobs(bool &bShutdown);
  int ProcessResults(const std::string &sJobID, const std::string &sResults, float fTime);
  
  int AbortSlaves(const std::set<SlaveClient*> &workers, const std::string &sJobID);

  friend void *slaveclient_thread_func(void *ptd);

public:
  SlaveClient();
  ~SlaveClient();

      // Start creates a thread which in turn connects to server and
      // calls "Run".  Start returns immediately after creating the
      // thread.
  int Start(const std::string &sServer, const std::string &sSlaveProgram, 
            const std::string &sSlaveArgs, JobQueue *pJobQueue, ThreadBarrier *pBarrier); 

  int Terminate();

  std::string Server() const;
};




// class SlaveServerReceiver : public util::Reporting
// {
//   SlaveServerReceiver();
//   ~SlaveServerReceiver();
//   static SlaveServerReceiver m_instance;

//   int m_argc;
//   char **m_argv;

//   pthread_t m_serverThread;
//       //!!-TODO This test is not satisfactory, because it assumes that
//       //!!- the server thread will never go down unexpectedly.  This
//       //!!- variable should be moved to the server class and guarded
//       //!!- by a mutex.  See also comment in source at ProcessMessage().
//   bool m_bServerThreadRunning;

//   int AbortServer(pthread_mutex_t *pLockedMutex);
//   int CreateServer(pthread_mutex_t *pLockedMutex);
//   int KillServer();

//   int ProcessMessage(int nMsgTag, bool &bTerminate);
//   int SendReadyMessage() const;

// public:
//   static SlaveServerReceiver& Instance();
//   int Run(int argc, char *argv[]);
// };



// class SlaveServer : public LockableObject
// {
//   SlaveServer();
//   ~SlaveServer();
//   static SlaveServer m_instance;

//   bool m_bReady, m_bAbort, m_bNewData;
//   int m_nJobID;
//   std::string m_sJobData;

//   pthread_cond_t *m_pSemaphore;
//   int WakeUp();
// public:
//   static SlaveServer &Instance();
//   int Run(int argc, char *argv[]);
//   int Wait(pthread_mutex_t *pLockedMutex, int nTimeoutSecs = 0);

//   bool Ready() const;
//   void SetAbortFlag();
//   int SetJobData(int nJobID, const std::string &sJobData);
// };
  
#endif
