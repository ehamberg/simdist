/********************************************************************
 *   		jobqueue.h
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
 *   Contains a queue meant to store jobs pending evaluation.  The
 *   queue has functions to provide exclusive access (through a
 *   mutex), and waiting on full/empty/any signal (semaphores).
 *******************************************************************/

#if !defined(__JOBQUEUE_H__)
#define __JOBQUEUE_H__

#include "syncutils.h"
#include "feedback.h"

#include <pthread.h>
#include <stdexcept>
#include <string>
#include <set>
#include <list>

// extern FeedbackError E_JOBQUEUE_CLOSE;
DECLARE_FEEDBACK_ERROR(E_JOBQUEUE_CLOSE)
// extern FeedbackError E_JOBQUEUE_SIGNAL;
DECLARE_FEEDBACK_ERROR(E_JOBQUEUE_SIGNAL)


/********************************************************************
 *   Elements in the queue.  An XML string keeping the job
 *   specification, an ID given by the master, and a set keeping
 *   pointers to all slaves working on the job.  Job IDs should be a
 *   unique identifier for the job, as this is used for operator<.
 *******************************************************************/
class JobQueueElement
{
public:
  std::string sJobID;
  std::string sJobData;
  std::string sResults;
  time_t timeLastStart;
  std::set<class SlaveClient*> workers;

  bool operator<(const JobQueueElement &rhs) const;
  std::string WorkersToString() const;
};


typedef std::set<JobQueueElement> TResultSet;

/********************************************************************
 *   Queue of jobs pending evaluation.  The plan was to inherit from
 *   std::queue, but this class lacks the necessary erase
 *   capabilities.  So it is a list, but treat it as a queue.
 *******************************************************************/
class JobQueue : public std::list<JobQueueElement>, public LockableObject
{
protected:
  pthread_cond_t *m_pSemaphore;
  bool m_bClosed;
  Feedback m_fb;
  TResultSet m_resultSet;
  size_t m_nNumJobsPerSend;
  bool m_bAutoNumJobsPerSend;

  JobQueue(const JobQueue &q);
public:
  JobQueue();
  ~JobQueue();
  
  bool Closed() const;
  int Close();

  int Wait(double dTimeoutSecs = 0);
  int Signal();

  const TResultSet& ResultSet() const;
  TResultSet& ResultSet();
  size_t NumJobsPerSend() const;
};
  

#endif
