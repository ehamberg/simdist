/********************************************************************
 *   		syncutils.h
 *   Created on Mon Mar 27 2006 by Boye A. Hoeverstad.
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
 *   Utilities to easy synchronization.
 *******************************************************************/

#if !defined(__SYNCUTILS_H__)
#define __SYNCUTILS_H__

#include "feedback.h"

#include <set>
#include <pthread.h>

#include <semaphore.h>

/********************************************************************
 *   Wrapper for POSIX semaphores.
 *******************************************************************/
class Semaphore
{
  sem_t *m_pSem;
  Semaphore();
  Semaphore(Semaphore&);
public:
  explicit Semaphore(int nValue, bool bShared = false);
  virtual ~Semaphore();
  virtual int Post();
  virtual int Wait();
      // The first overload sets nVal and returns the value returned
      // from sem_getvalue.  This is 0 unless an error has occurred.
      // The man pages unclear here, but it seems that nVal is always
      // 0 or larger, irrespective of the number of threads/processes
      // waiting for the semaphore.
  virtual int Value(int &nVal);
      // The second overload returns the value set by get_value,
      // assuming the call succeeded.
  virtual int Value();
};


// /********************************************************************
//  *   A binary semaphore can't be constructed on the basis of a normal
//  *   semaphore (at least not without including additional locking, but
//  *   probably not even then), since a post must atomically check and
//  *   modify the semaphore value.
//  *
//  *   !!- TODO: Very weak error checking in wait and post functions;
//  *   switch to AutoMutex and actually return on error.
//  *******************************************************************/
// 
// I think this class works, but it needs a bit more testing before
// being put to use.  One feature is not supported at all, and that is
// the order in which threads are woken up, when more than one thread
// is waiting on the semaphore.  Since pthread_cond_signal does not
// specify which of the waiting threads is woken up, I believe this
// can only be achieved by a broadcast on the condition, coupled with
// separate flags for each waiting thread.  This solution is of course
// very inefficient.
//
// Consider using a common base class for both semaphore types.
//
// class BinarySemaphore // : public BasicSemaphore
// {
//   pthread_cond_t *m_pCond;
//   pthread_mutex_t *m_pMtx;
//   int m_nValue;
//   bool m_bUnlocking; // Avoid spurious wake-ups
//   BinarySemaphore();
//   BinarySemaphore(BinarySemaphore&);
// public:
//   explicit BinarySemaphore(int nValue);
//   virtual ~BinarySemaphore();
//   virtual int Post();
//   virtual int Wait();
//   virtual int Value(int &nVal);
//   virtual int Value();
// };



/********************************************************************
 *   Wrapper for pthread conditions.  Takes a predicate operation as
 *   input, so this class can deal with checking the predicate in case
 *   of spurious wake-ups.  
 *
 *   The original version used a pointer to function + pointer to void
 *   in its constructor, and only mutex as argument to Wait function.
 *   This setup uses two templated Wait overloads, which can take any
 *   kind of parameter.  
 *
 *   This setup enables the use of both traditional function pointers
 *   and functors.  It also increases flexibility, as the any
 *   predicate can be used in a Wait.  On the other hand, this may not
 *   be desirable, as it increases the chances of introducing errors.
 * 
 *   Further, the current syntax clutters the code surrounding the
 *   Wait, but at the same time it is easy to see which predicate is
 *   used.  Finally, there is no need for a friend function.
 *
 *   The optimal solution would be a mix of the two, but then
 *   Condition would have to be a template, and this makes it very
 *   hard to define Condition<some type> members in a class,
 *   especially since they would have to refer to incomplete types if
 *   one wants to use member functions from the same class.
 *
 *******************************************************************/
class Condition
{
  pthread_cond_t *m_pCond;
public:
  Condition()
      : m_pCond(new pthread_cond_t)
  {
    pthread_cond_init(m_pCond, 0);
  }

  ~Condition()
  {
    pthread_cond_destroy(m_pCond);
    delete m_pCond;
  }

  int Signal()
  {
    return pthread_cond_signal(m_pCond);
  }

  int Broadcast()
  {
    return pthread_cond_broadcast(m_pCond);
  }

  template<class Op, class OpArg>
  int Wait(pthread_mutex_t *pMtx, Op op, OpArg opArg)
  {
    while (!op(opArg))
    {
      if (int nRet = pthread_cond_wait(m_pCond, pMtx))
        return nRet;
    }
    return 0;
  }

  template<class Op>
  int Wait(pthread_mutex_t *pMtx, Op op)
  {
    while (!op())
    {
      if (int nRet = pthread_cond_wait(m_pCond, pMtx))
        return nRet;
    }
    return 0;
  }
};


/********************************************************************
 *   Class to automatically unlock a mutex in destructor.  The mutex sent
 *   in should already be locked.
 *******************************************************************/
class AutoMutex
{
  AutoMutex(AutoMutex &); // Not implemented: No copy semantics.
  Feedback m_fb;
  pthread_mutex_t *m_pLockedMutex;
  std::string m_sMutexDesc;
public:
  AutoMutex(pthread_mutex_t *pLockedMutex = 0, std::string sMutexDesc = "");
  ~AutoMutex();

  void SetLockedMutex(pthread_mutex_t *pLockedMutex, std::string sMutexDesc = "");
  pthread_mutex_t* GetLockedMutex() const;
  int Unlock();
    
};


/********************************************************************
 *   Superclass or utility class for objects that need locking
 *   capabilities, taking advantage of the AutoMutex.
 *******************************************************************/
class LockableObject
{
  LockableObject(LockableObject &); // Not implemented: No copy semantics.
protected:
  Feedback m_fb;
  pthread_mutex_t *m_pMutex;
  std::string m_sMutexDesc;
public:
  LockableObject(std::string sMutexDesc = "");
  virtual ~LockableObject();
  int AcquireMutex(AutoMutex &mtx);
  pthread_mutex_t *GetMutexPtr() const;
};



template<class T> class Barrier
{
  LockableObject m_mtx;
  pthread_cond_t *m_pCond;
  std::set<T> m_data;
  Barrier(const Barrier &); // Not implemented: No copy semantics.
public:
  Barrier()
      : m_pCond(new pthread_cond_t)
  {
    pthread_cond_init(m_pCond, 0);
  }

  ~Barrier()
  {
    pthread_cond_destroy(m_pCond);
    delete m_pCond;
  }

  int Add(const T &t)
  {
    AutoMutex mtx;
    m_mtx.AcquireMutex(mtx);
    m_data.insert(t);
    return 0;
  }

  bool Found(const T &t) const
  {
    return m_data.find(t) != m_data.end();
  }

  int Erase(const T &t)
  {
    AutoMutex mtx;
    m_mtx.AcquireMutex(mtx);
    m_data.erase(t);
    if (m_data.empty())
      pthread_cond_broadcast(m_pCond);
    return 0;
  }

  int WaitSize(size_t nSize, int nTimeoutSecs = 0)
  {
    AutoMutex mtx;
    m_mtx.AcquireMutex(mtx);
    if (m_data.size() == nSize)
      return 0;
    pthread_mutex_t *pMtx = mtx.GetLockedMutex();
    if (nTimeoutSecs > 0)
    {
      time_t cur = time(0);
      timespec ts = { cur + nTimeoutSecs, 0 };
      pthread_cond_timedwait(m_pCond, pMtx, &ts);
    }
    else
      while (m_data.size() != nSize)
        pthread_cond_wait(m_pCond, pMtx);
    return 0;
  }

  int WaitEmpty(int nTimeoutSecs = 0)
  {
    return WaitSize(0, nTimeoutSecs);
  }

  size_t size() 
  {
    AutoMutex mtx;
    m_mtx.AcquireMutex(mtx);
    return m_data.size();    
  }

  bool empty() 
  {
    AutoMutex mtx;
    m_mtx.AcquireMutex(mtx);
    return m_data.empty();    
  }

};

typedef Barrier<pthread_t> ThreadBarrier;

#endif
