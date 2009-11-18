/********************************************************************
 *   		feedback.cpp
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
 *   See header file for description.
 *******************************************************************/

#include <simdist/feedback.h>
#include <simdist/misc_utils.h>

#include <iostream>

int FeedbackError::m_nCodeCounter = 1;

FeedbackError::FeedbackError(const std::string &sDescription)
  : m_nCode(m_nCodeCounter++), m_sDescription(sDescription)
{
}

FeedbackError::operator int() const
{
  return m_nCode;
}

std::string 
FeedbackError::Description() const
{
  return m_sDescription;
}

int 
FeedbackError::Code() const
{
  return m_nCode;
}


/*static*/ FeedbackCentral& 
FeedbackCentral::Instance()
{
  static FeedbackCentral instance(std::cerr);
  return instance;
}


FeedbackCentral::FeedbackCentral(std::ostream &outputstream)
    : m_nInfoLevel(0), m_outputstream(outputstream)
    , m_pOutputMutex(&m_defaultOutputMutex)
{
  if (pthread_mutex_init(&m_defaultOutputMutex, 0) ||
      pthread_mutex_init(&m_selfMutex, 0))
    std::cerr << "Error in feedback library while initializing mutexes.\n"
              << "Continuing, but errors may occur.\n";
}

FeedbackCentral::~FeedbackCentral()
{
  pthread_mutex_destroy(&m_defaultOutputMutex);
  pthread_mutex_destroy(&m_selfMutex);
}


void 
FeedbackCentral::SetOutputMutex(pthread_mutex_t *pMtx /*=0*/)
{
  LockMutex(&m_selfMutex);
  if (pMtx)
    m_pOutputMutex = pMtx;
  else
    m_pOutputMutex = &m_defaultOutputMutex;
  UnlockMutex(&m_selfMutex);
}



/********************************************************************
 *   Lock the mutex pMtx.  Return type is void, because if an error
 *   occurs, there is really not much we can do other than either
 *   print a message and continue working, or exiting the program.  We
 *   opt for the former.
 *******************************************************************/
void 
FeedbackCentral::LockMutex(pthread_mutex_t *pMtx) const
{
  if (pthread_mutex_lock(pMtx))
  {
//     static bool bFirst = true;
//     if (bFirst)
//     {
//     bFirst = false;
    std::cerr << "ERROR IN FEEDBACK LIBRARY!  Failed to lock mutex " << pMtx 
              << ". Will keep on running, but nasty stuff may happen...\n";
//     }
  }
}


/********************************************************************
 *   Unlock the mutex pMtx previously locked by a call to LockMutex.
 *   See LockMutex for further comments.
 *******************************************************************/
void 
FeedbackCentral::UnlockMutex(pthread_mutex_t *pMtx) const
{
  if (pthread_mutex_unlock(pMtx))
  {
//     static bool bFirst = true;
//     if (bFirst)
//     {
//       bFirst = false;
    std::cerr << "ERROR IN FEEDBACK LIBRARY!  Failed to unlock mutex " << pMtx 
              << ". Will keep on running, but nasty stuff may happen...\n";
//     }
  }
}



/********************************************************************
 *   Register a description of the current thread.  This description
 *   will be used in all subsequent output from the current thread.
 *   If no description is registered, thread id is used.
 *******************************************************************/
void
FeedbackCentral::RegisterThreadDescription(std::string sDescription)
{
  LockMutex(&m_selfMutex);
  m_threadDescriptions[pthread_self()] = sDescription;
  UnlockMutex(&m_selfMutex);
}


/********************************************************************
 *   Set an upper limit on the level of info that is printed.  High
 *   numbers mean verbose output.
 *******************************************************************/
void 
FeedbackCentral::SetInfoLevel(int nLevel)
{
  LockMutex(&m_selfMutex);
  m_nInfoLevel = nLevel;
  UnlockMutex(&m_selfMutex);
}


int
FeedbackCentral::GetInfoLevel() const
{
  LockMutex(&m_selfMutex);
  int nLevel = m_nInfoLevel;
  UnlockMutex(&m_selfMutex);
  return nLevel;
}


/********************************************************************
 *   sShow and sHide should both be comma-separated lists.  If sShow
 *   is non-empty, only modules named in this list will have their
 *   info printed.  If sHide is non-empty, modules in this list will
 *   never have their info printed.
 *******************************************************************/
void 
FeedbackCentral::SetShowHide(std::string sShow, std::string sHide)
{
  LockMutex(&m_selfMutex);

  std::string *psShowHideOpt[2] = { &sShow, &sHide };
  std::set<std::string> *pShowHideSet[2] = { &m_showSet, &m_hideSet };

  for (int i = 0; i < 2; i++)
  {
    pShowHideSet[i]->clear();

    std::string::size_type n;
    while ((n = psShowHideOpt[i]->find(",")) != std::string::npos)
      psShowHideOpt[i]->replace(n, 1, "\n");

    std::stringstream ss(*(psShowHideOpt[i]));
    std::string sLine, sSpace = " ";
    while (std::getline(ss, sLine))
    {
      if ((n = sLine.find_first_not_of(sSpace)) == std::string::npos)
        continue;
      sLine = sLine.substr(n, sLine.find_last_not_of(sSpace) - n + 1);
      pShowHideSet[i]->insert(sLine);
    }
  }

  UnlockMutex(&m_selfMutex);
}



/********************************************************************
 *   Synchronized output.  All writing should go through this call,
 *   thus guaranteeing that the entire string is written atomically.
 *   See top of header for explanation to why we can't use syncutils.
 *******************************************************************/
void  
FeedbackCentral::WriteOutput(const std::string &sOutput) const
{
  LockMutex(m_pOutputMutex);
  m_outputstream << sOutput << std::flush;
  UnlockMutex(m_pOutputMutex);
}


int 
FeedbackCentral::Error(const Feedback *pSender, const FeedbackError &error, const std::string &sAdditionalInfo) const
{
  std::stringstream ss;
  ss << "Error in thread " << GetThreadDescription() 
     << " reported by " << pSender->Identify() 
     << ": " << error.Description()
     << (sAdditionalInfo.empty() ? "." : sAdditionalInfo) << "\n";
  WriteOutput(ss.str());
  return error.Code();
}


void 
FeedbackCentral::Warning(const Feedback *pSender, const std::string &sMessage) const
{
  std::stringstream ss;
  ss << "Warning in thread " << GetThreadDescription() 
     << " from " << pSender->Identify() << ": " << sMessage << "\n" << std::flush;
  WriteOutput(ss.str());
}



/********************************************************************
 *   Search a set of strings sSet for a match to sQry.  sSet can
 *   contain wildcards.
 *******************************************************************/
bool
FeedbackCentral::FindMatch(const std::string &sQry, const std::set<std::string> &sSet) const
{
  for (std::set<std::string>::const_iterator sit = sSet.begin(); sit != sSet.end(); sit++)
    if (WildcardMatch(sit->c_str(), sQry.c_str()))
      return true;
  return false;
}


/********************************************************************
 *   Print info if the given level is low enough.  The info check is
 *   unprotected, but since it only amounts to reading an int, not
 *   changing any memory, we take the chance.  Infolevel is checked
 *   frequently, and should be fast.  GetThreadDescription locks
 *   m_selfMutex, so unlock before calling this function.
 *******************************************************************/
void 
FeedbackCentral::Info(const Feedback *pSender, int nLevel, const std::string &sMessage) const
{
  std::string sIdentity = pSender->Identify();
  if (nLevel <= m_nInfoLevel)
  {
    LockMutex(&m_selfMutex);
    if ((m_showSet.empty() || FindMatch(sIdentity, m_showSet))
        && (m_hideSet.empty() || !FindMatch(sIdentity, m_hideSet)))
    {
      UnlockMutex(&m_selfMutex); // Unlock before GetThreadDescription() below.

      std::stringstream ss;
      ss << "Info (" << nLevel << ") in thread " << GetThreadDescription() 
         << " from " << sIdentity << ": " << sMessage << "\n" << std::flush;
      WriteOutput(ss.str());
    }
    else
      UnlockMutex(&m_selfMutex);
  }
}



/********************************************************************
 *   Basic output: Write message without any wrapping.  The only
 *   reason for going through Feedback instead of writing directly to
 *   m_outputstream is to get synchronized output.
 *******************************************************************/
void 
FeedbackCentral::Output(const Feedback */*pSender*/, const std::string &sMessage) const
{
  WriteOutput(sMessage);
}

/********************************************************************
 *   Return description of current thread.  If a description has been
 *   registered, the string returned reads "description (id
 *   thread_id)", otherwise it reads only "thread_id", where
 *   'description' is the description previously registered, and
 *   thread_id is the value returned from pthread_self().
 *  
 *   !!- This assumes that pthread_t can be implicitly casted to a type
 *       understood by stringstream::operator<<.
 *******************************************************************/
std::string 
FeedbackCentral::GetThreadDescription() const
{
  pthread_t thread_id = pthread_self();
  std::stringstream ss;
  LockMutex(&m_selfMutex);
  TThreadMap::const_iterator tdit = m_threadDescriptions.find(thread_id);
  if (tdit == m_threadDescriptions.end())
    ss << thread_id;
  else
    ss << tdit->second << " (id " << thread_id << ")";
  UnlockMutex(&m_selfMutex);
  return ss.str();
}


IFeedbackStream::IFeedbackStream(const Feedback *pSender)
    : m_pSender(pSender)
{
}


IFeedbackStream::IFeedbackStream(const IFeedbackStream &rhs)
{
  m_ss << rhs.m_ss.str();
}


IFeedbackStream::~IFeedbackStream()
{
}


ErrorStream::ErrorStream(const Feedback *pSender, const FeedbackError &error, bool bHaveError /*=true*/)
    : IFeedbackStream(pSender), m_error(error), m_bHaveError(bHaveError)
{
}

  
ErrorStream::operator int() const
{
  return m_bHaveError ? m_error.Code() : 0;
}


ErrorStream::~ErrorStream()
{
  if (m_bHaveError)
    FeedbackCentral::Instance().Error(m_pSender, m_error, m_ss.str());
}


WarningStream::WarningStream(const Feedback *pSender, const std::string &sMessage, bool bHaveError /*=true*/)
    : IFeedbackStream(pSender), m_bHaveError(bHaveError)
{
  m_ss << sMessage;
}


WarningStream::~WarningStream() 
{
  if (m_bHaveError)
    FeedbackCentral::Instance().Warning(m_pSender, m_ss.str());
}



InfoStream::InfoStream(const Feedback *pSender, int nLevel, const std::string &sMessage)
    : IFeedbackStream(pSender), m_nLevel(nLevel)
{
  m_ss << sMessage;
}


InfoStream::~InfoStream() 
{
  FeedbackCentral::Instance().Info(m_pSender, m_nLevel, m_ss.str());
}


OutputStream::OutputStream(const Feedback *pSender, const std::string &sMessage)
    : IFeedbackStream(pSender)
{
  m_ss << sMessage;
}

OutputStream::~OutputStream()
{
  FeedbackCentral::Instance().Output(m_pSender, m_ss.str());
}

Feedback::Feedback(const std::string &sIdentifier)
    : m_sIdentifier(sIdentifier)
{
}


Feedback::Feedback(const Feedback &rhs)
    : m_sIdentifier(rhs.m_sIdentifier)
{
}


Feedback& 
Feedback::operator =(const Feedback &rhs)
{
  m_sIdentifier = rhs.m_sIdentifier;
  return *this;
}

  
/*static*/ void
Feedback::RegisterThreadDescription(std::string sDescription)
{
  FeedbackCentral::Instance().RegisterThreadDescription(sDescription);
}


ErrorStream
Feedback::Error(const FeedbackError &error) const
{
  return ErrorStream(this, error);
}

ErrorStream
Feedback::Error(const FeedbackError &(*provider)()) const
{
  return Error(provider());
}

WarningStream 
Feedback::Warning(const std::string &sMessage /*=""*/) const
{
  return WarningStream(this, sMessage);
}


InfoStream 
Feedback::Info(int nLevel, const std::string &sMessage /*=""*/) const
{
  return InfoStream(this, nLevel, sMessage);
}


ErrorStream 
Feedback::ErrorIfNonzero(int nCond, const FeedbackError &error) const
{
  return ErrorStream(this, error, nCond != 0);
}


ErrorStream 
Feedback::ErrorIfNonzero(int nCond, const FeedbackError &(*provider)()) const
{
  return ErrorIfNonzero(nCond, provider());
}


WarningStream 
Feedback::WarningIfNonzero(int nCond, const std::string &sMessage /*=""*/) const
{
  return WarningStream(this, sMessage, nCond != 0);
}


const std::string& 
Feedback::Identify() const
{
  return m_sIdentifier;
}


void 
Feedback::SetIdentifier(const std::string &sNewIdentifier)
{
  m_sIdentifier = sNewIdentifier;
}


/*static*/ void
Feedback::SetInfoLevel(int nLevel)
{
  FeedbackCentral::Instance().SetInfoLevel(nLevel);
}


/*static*/ int
Feedback::GetInfoLevel()
{
  return FeedbackCentral::Instance().GetInfoLevel();
}


/*static*/ void 
Feedback::SetShowHide(const std::string &sShow, const std::string &sHide)
{
  FeedbackCentral::Instance().SetShowHide(sShow, sHide);
}
