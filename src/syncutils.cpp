/********************************************************************
 *   		syncutils.cpp
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
 *   See header file for description.
 *******************************************************************/

#include <simdist/syncutils.h>
#include <cassert>


Semaphore::Semaphore(int nValue, bool bShared /*=false*/)
    : m_pSem(new sem_t)
{
  int nShared = bShared ? 1 : 0;
  int nRet = sem_init(m_pSem, nShared, nValue);
  assert(nRet != -1);
}

Semaphore::~Semaphore()
{
  sem_destroy(m_pSem);
  delete m_pSem;
}

int
Semaphore::Post()
{
  return sem_post(m_pSem);
}


int
Semaphore::Wait()
{
  return sem_wait(m_pSem);
}


int
Semaphore::Value(int &nVal)
{
  return sem_getvalue(m_pSem, &nVal);
}


int
Semaphore::Value()
{
  int nVal;
  if (Value(nVal) == -1)
    return -1;
  return nVal;
}

    
// BinarySemaphore::BinarySemaphore(int nValue)
//     : m_pCond(new pthread_cond_t), m_pMtx(new pthread_mutex_t)
//     , m_nValue(0), m_bUnlocking(false)
// {
//   pthread_cond_init(m_pCond, 0);
//   pthread_mutex_init(m_pMtx, 0);
//   assert(nValue == 1 || nValue == 0);
//   if (nValue > 0)
//     m_nValue = 1;
// }


// BinarySemaphore::~BinarySemaphore()
// {
//   pthread_cond_destroy(m_pCond);
//   pthread_mutex_destroy(m_pMtx);
//   delete m_pCond;
//   delete m_pMtx;
// }


// int
// BinarySemaphore::Post()
// {
//   pthread_mutex_lock(m_pMtx);
//       // Another thread is waiting. Set unlock flag and signal.
//   if (m_nValue < 0)
//   {
//     m_bUnlocking = true;
//     pthread_cond_signal(m_pCond);
//   }
//       // Inc value, but never to more than 1 
//   if (m_nValue <= 0)
//     m_nValue++;
//   pthread_mutex_unlock(m_pMtx);
//   return 0;
// }


// int
// BinarySemaphore::Wait()
// {
//   pthread_mutex_lock(m_pMtx);
//   m_nValue--;
//   if (m_nValue < 0)
//   {
//     do 
//     { 
//           // Make sure we wait at least once.  This in case thread 1
//           // Posts to wake up thread 2, followed by a wait on t 1
//           // before t 2 has been resumed and had the time to reset the
//           // unlock flag.
//       pthread_cond_wait(m_pCond, m_pMtx);
//     } while (!m_bUnlocking);
//   }
//   m_bUnlocking = false;
//   pthread_mutex_unlock(m_pMtx);
//   return 0;
// }


// int
// BinarySemaphore::Value(int &nVal)
// {
//   assert(m_nValue <= 1);
//   nVal = std::max(m_nValue, 0);
//   return 0;
// }


// int
// BinarySemaphore::Value()
// {
//   int nVal;
//   Value(nVal);
//   return nVal;
// }




AutoMutex::AutoMutex(pthread_mutex_t *pLockedMutex /*=0*/, std::string sMutexDesc /*=""*/)
    : m_fb("AutoMutex")
    , m_pLockedMutex(pLockedMutex), m_sMutexDesc(sMutexDesc)
      
{
}


AutoMutex::~AutoMutex() 
{
  if (m_pLockedMutex)
    Unlock();
}


void 
AutoMutex::SetLockedMutex(pthread_mutex_t *pLockedMutex, std::string sMutexDesc /*=""*/) 
{
  m_pLockedMutex = pLockedMutex;
  if (!sMutexDesc.empty())
    m_sMutexDesc = sMutexDesc;
}

  
pthread_mutex_t*
AutoMutex::GetLockedMutex() const 
{
  return m_pLockedMutex;
}


int
AutoMutex::Unlock()
{
//   if (m_sMutexDesc.empty())
//     m_fb.Info(5) << "Unlocking mutex " << m_pLockedMutex << ".";
//   else
//     m_fb.Info(5) << "Unlocking mutex " << m_sMutexDesc << " (" << m_pLockedMutex << ").";


  if (m_pLockedMutex)
  {
        // Note: Set to 0 before actually unlocking the mutex,
        // otherwise the test above may fail, if two threads call Unlock simultaneously.
    pthread_mutex_t *pMutex = m_pLockedMutex;
    m_pLockedMutex = 0;
        //!!- No error handling.
    int nRet = pthread_mutex_unlock(pMutex);
    return m_fb.ErrorIfNonzero(nRet, E_MUTEX_UNLOCK);
  }
  return m_fb.Error(E_MUTEX_UNLOCK_TWICE);
}
    

LockableObject::LockableObject(std::string sMutexDesc /*=""*/)
    : m_fb("LockableObject")
    , m_pMutex(new pthread_mutex_t), m_sMutexDesc(sMutexDesc)
{
      //!!- No error handling.  Problems will arise if
      //initialization fails.  Consider moving to separate class
      //with automatic creation/destructio and initialization.
  int nInit = pthread_mutex_init(m_pMutex, 0);
  assert(nInit == 0 || !m_fb.Error(E_MUTEX_INIT));
}


LockableObject::~LockableObject()
{
      //!!- The mutex should be unlocked now.
  int nDest = pthread_mutex_destroy(m_pMutex);
  assert(nDest == 0 || !m_fb.Error(E_MUTEX_DESTROY));
  delete m_pMutex;
}


/********************************************************************
 *   Lock and acquire a mutex.  This function will block until the
 *   mutex is available.  The mutex, in locked state, is returned in
 *   the AutoMutex mtx.  Returns 0 on success.  Will fail if
 *   phtread_mutex_lock fails.
 *******************************************************************/
int
LockableObject::AcquireMutex(AutoMutex &mtx)
{
  assert(m_pMutex != 0);
  assert(!mtx.GetLockedMutex());

//   std::stringstream ssMtxID;
//   if (m_sMutexDesc.empty())
//     ssMtxID << m_pMutex;
//   else
//     ssMtxID << m_sMutexDesc << " (" << m_pMutex << ")";
//   m_fb.Info(5) << "Attempting to lock mutex " << ssMtxID.str() << "...";

  if (pthread_mutex_lock(m_pMutex))
    return m_fb.Error(E_MUTEX_LOCK);
    
//   m_fb.Info(5) << "Mutex " << ssMtxID.str() << " successfully locked.";

  mtx.SetLockedMutex(m_pMutex, m_sMutexDesc);
  return 0;
}


pthread_mutex_t* 
LockableObject::GetMutexPtr() const
{
  return m_pMutex;
}
