/********************************************************************
  *   		jobqueue.cpp
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

#include "../config.h"

#include <simdist/jobqueue.h>
#include <simdist/slave.h>

#include <simdist/options.h>

#include <cstring>
#include <cassert>
#include <cmath>
#include <sys/errno.h>
#include <sys/time.h>

// FeedbackError E_JOBQUEUE_CLOSE("Failed to close the queue");
DEFINE_FEEDBACK_ERROR(E_JOBQUEUE_CLOSE, "Failed to close the queue");
// FeedbackError E_JOBQUEUE_SIGNAL("Failed to signal the queue");
DEFINE_FEEDBACK_ERROR(E_JOBQUEUE_SIGNAL, "Failed to signal the queue");


bool
JobQueueElement::operator<(const JobQueueElement &rhs) const
{ 
  return sJobID < rhs.sJobID; 
}


std::string
JobQueueElement::WorkersToString() const
{
  std::string s;
  std::set<class SlaveClient*>::const_iterator wit = workers.begin();
  if (wit != workers.end())
    s = (*wit)->Server();
  while (++wit != workers.end())
    s += ", " + (*wit)->Server();
  return s;
}


JobQueue::JobQueue()
  : LockableObject("Jobqueue-mutex")
  , m_pSemaphore(new pthread_cond_t)
  , m_bClosed(false)
  , m_fb("JobQueue")
  , m_nNumJobsPerSend(1)
  , m_bAutoNumJobsPerSend(false)
{
      //!!- No error handling.  Problems will arise if
      //initialization fails.  Consider moving to separate class
      //with automatic creation/destructio and initialization.
  int nInit = pthread_cond_init(m_pSemaphore, 0);
  assert(nInit == 0 || !m_fb.Error(E_COND_INIT));
  int nNumPerSend;
  if (Options::Instance().Option("jobs-per-send", nNumPerSend))
    m_fb.Warning("Failed to get option jobs-per-send, will default to ") 
      << m_nNumJobsPerSend;
  else
  {
    if (nNumPerSend <= 0)
      m_bAutoNumJobsPerSend = true;
    else
      m_nNumJobsPerSend = static_cast<size_t>(nNumPerSend);
  }
  
}


JobQueue::~JobQueue()
{
      //!!- The mutex should be unlocked now.
  int nDest = pthread_cond_destroy(m_pSemaphore);
  assert(nDest == 0 || !m_fb.Error(E_COND_DESTROY));
  delete m_pSemaphore;
}


/********************************************************************
 *   The result set is part of the jobqueue because 
 *
 *    a) they are always used together, and 
 *   
 *    b) access to the result set must be synchronized.  Since the
 *       result set is normally accessed at a time when the job queue
 *       is already locked, it is natural to use the queue locking
 *       when accessing the result set as well.
 *******************************************************************/
TResultSet&
JobQueue::ResultSet()
{
  return m_resultSet;
}

const TResultSet& 
JobQueue::ResultSet() const
{
  return m_resultSet;
}


/********************************************************************
 *   Returns a bool indicating whether the queue has been closed or
 *   not.  A queue is initially open, and may be closed by a call to
 *   Close().
 *
 *   Call from within mutex lock.
 *******************************************************************/
bool 
JobQueue::Closed() const
{
  return m_bClosed;
}


/********************************************************************
 *   Sets the queue state to "closed", indicating that the queue is no
 *   longer in use (i.e. the evolutionary search has completed).
 *******************************************************************/
int
JobQueue::Close()
{
  AutoMutex mtx;
  if (AcquireMutex(mtx))
    return m_fb.Error(E_JOBQUEUE_CLOSE);
  m_bClosed = true;
  return 0;
}


/********************************************************************
 *   Wait on a queue for a signal, or for the specified number of
 *   seconds to elapse (optional).  Note that spurious wakeups may
 *   occur.  The function returns 0 on success (success includes
 *   spurious wake-up, signaled wake-up and timeout), otherwise an
 *   error code.
 *
 *   The queue should be locked upon entry.
 *
 *   !!- TODO: Generalize this and put in syncutils.
 *******************************************************************/
int
JobQueue::Wait(double dTimeoutSecs /*=0*/)
{
  assert(m_pSemaphore && m_pMutex);

  if (dTimeoutSecs > 0)
  {
    timespec timeout;
    int (*pWaitFunc)(pthread_cond_t *, pthread_mutex_t *, const struct timespec *) = pthread_cond_timedwait;

#if HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE_NP
    pWaitFunc = pthread_cond_timedwait_relative_np;
    double dInt, dFrac = modf(dTimeoutSecs, &dInt);
    timeout.tv_sec = static_cast<time_t>(dInt);
    timeout.tv_nsec = static_cast<long int>(dFrac * 1e9);
#elif HAVE_CLOCK_GETTIME
    if (clock_gettime(CLOCK_REALTIME, &timeout))
    {
      m_fb.Warning(std::string("Failed to get high-resolution time when waiting for queue to empty.  "
                               "Waiting at least one second. System error message: ")
                   + strerror(errno));
      time_t cur = time(0);
      timeout.tv_sec = cur + static_cast<time_t>(ceil(dTimeoutSecs));
      timeout.tv_nsec = 0;
    }
    else
    {
      double dInt, dFrac = modf(dTimeoutSecs, &dInt);
      timeout.tv_sec += static_cast<time_t>(dInt);
      timeout.tv_nsec += static_cast<long int>(dFrac * 1e9);
    }

        // No more than 1e9 - 1 nanoseconds!
    while (timeout.tv_nsec >= 1e9)
    {
      timeout.tv_sec++;
      timeout.tv_nsec -= static_cast<long int>(1e9);
    }
#else
  ERROR: Unable to calculate waiting time correctly!!
#endif
    
    int nRet = pWaitFunc(m_pSemaphore, m_pMutex, &timeout);

    if (nRet && nRet != ETIMEDOUT)
    {
      if (nRet == EINTR)
        return m_fb.Error(E_COND_TIMEDWAIT) << ", the wait was interrupted by a signal.";
      return m_fb.Error(E_COND_TIMEDWAIT) << ", unrecognized return code: " << nRet;
    }
  }
  else
    if(pthread_cond_wait(m_pSemaphore, m_pMutex))
      return m_fb.Error(E_COND_WAIT);

  return 0;
}



/********************************************************************
 *   Signal all threads waiting on the queue.  This may include both
 *   slave clients and the master.
 *******************************************************************/
int
JobQueue::Signal()
{

      // First update num jobs per send, if applicable, as a
      // signal indicates that something important has happened
      // to the queue (i.e. either it's empty, or it has been
      // refilled).
  if (m_bAutoNumJobsPerSend)
  {
    size_t nNumSlaves = SlaveClientFactory::Instance().GetNumSlaves();
    if (nNumSlaves > 0)
      m_nNumJobsPerSend = std::max(static_cast<size_t>(1), static_cast<size_t>(size() / nNumSlaves));
  }

  int nRet = pthread_cond_broadcast(m_pSemaphore);
  return m_fb.ErrorIfNonzero(nRet, E_JOBQUEUE_SIGNAL); // This only happens if the semaphore is not initialized correctly
}


/********************************************************************
 *   Return the number of jobs to be sent to a slave in each
 *   batch.  Unprotected, should preferably be called while the
 *   queue is locked.
 *******************************************************************/
size_t 
JobQueue::NumJobsPerSend() const
{
  return m_nNumJobsPerSend;
}

