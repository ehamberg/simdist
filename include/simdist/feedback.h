/********************************************************************
 *   		feedback.h
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
 *   A utility library in order to easily provide feedback to users.  
 *
 *   Key design ideas:
 * 
 *   - Every error has a unique integer code with a corresponding
 *     symbolic value.
 * 
 *   - All functions return int by default.  A nonzero return value
 *     indicates an error, and the number can be used to identify the
 *     error.
 * 
 *   - There are three levels of user feedback: Error, Warning and
 *     Info, with additional levels of detail in Info messages (low
 *     numbers mean low repeat count).  As a guide, use level 1 for
 *     stuff that happens only once, such as successful initialization
 *     of Singletons.  Extremely verbose output should come as level 4
 *     or above.
 * 
 *   - An additional option to show info only from certain feedback
 *     objects, or alternatively exclude info from certain objects, is
 *     desirable.
 * 
 *   - The user should not need to worry about assigning an integer
 *     value to each error.  The user selects the symbolic name, and an
 *     integer value is designated by the system.  Additionally:
 *     
 *     * Error codes and messages should be easily declared in a header
 *       file, to be included by users of the given error. Errors are
 *       declared in a header file as follows:
 * 
 *         extern FeedbackError symbolic_name;
 *         or
 *         DECLARE_FEEDBACK_ERROR(symbolic_name)
 *
 *       and defined in the corresponding source code file as follows:
 *
 *         FeedbackError symbolic_name("textual_description");
 *         or
 *         DEFINE_FEEDBACK_ERROR(symbolic_name)
 *
 *       The latter form is for use with libraries, where global
 *       static variables are not always initialized correctly.
 *
 *     * The integer value associated with the symbolic name of an
 *       error is not persistent between runs of the system.  In other
 *       words, integer values may be assigned to each symbolic name
 *       during system startup.
 * 
 *   - The Error and Warning functions have corresponding
 *     ErrorIfNonzero and WarningIfNonzero functions, which call their
 *     counterparts conditionally based on the value of an integer
 *     variable.
 * 
 *   - All above functions are member functions in a Feedback object.
 *     This object takes the name of its container/owner (typically a
 *     class or a function), or any other relevant textual description,
 *     as input to its constructor.  This name/description will then be
 *     prepended to every feedback message sent through the object.  It
 *     can also be used to implement selective toggling of info
 *     messages.
 * 
 *   - The actual reporting of feedback to the user is handled
 *     centrally by a singleton FeedbackCentral object.  All functions
 *     in a Feedback object will consequently channel all their
 *     messages through the FeedbackCentral.
 *     
 *     * The FeedbackCentral is not accessible by outside users of the
 *       feedback library (?)
 * 
 *     * The FeedbackCentral is responsible for assigning integer
 *       values to symbolic error names.
 * 
 *   - Every feedback function should also support operator<<, in order
 *     to easily append extra information to an error message.
 * 
 *   - Registered error descriptions should not contain a period at
 *     the end.  A period will be appended by the Feedback system iff
 *     there is no additional info <<'ed by the user.  If the user
 *     <<'s additional info, he/she should also take care to insert
 *     the correct delimiters.
 * 
 *   - The feedback library synchronizes its output through the use of
 *     a mutex.  By default, the mutex used is a class member, but
 *     this can be changed by the user by calling SetOutputMutex.  It
 *     is thus possible to install a system-wide output
 *     synchronization mutex.
 *
 *     * Synchronization option 2: Install synchronized stdio
 *       wrappers.  This would be a nice solution, since this library
 *       would then work independently of whether multithreading is
 *       enabled or not.  However, it would be rather hard to
 *       implement, since each of the << and >> operators are a
 *
 *   - Multithreading is assumed to be enabled by default.  Note that
 *     we can't use the Syncutils here, since that library uses this
 *     one.  In particular, LockableObject uses Feedback, which could
 *     lead to a deadlock if an error occurs during mutex locking.
 *
 *   Regarding error handling:
 * 
 *   - All errors should be checked, but need not necessarily be
 *     handled.  This means that functions likely to succeed, such as
 *     pthread_mutex_init, can be used in constructors.
 * 
 *******************************************************************/

#if !defined(__FEEDBACK_H__)
#define __FEEDBACK_H__

#include <map>
#include <set>
#include <string>
#include <sstream>
#include <iostream>

#include <pthread.h>

class FeedbackError
{
  int m_nCode;
  std::string m_sDescription;
  static int m_nCodeCounter;
public:
  FeedbackError(const std::string &sDescription);
  operator int() const;
  std::string Description() const;
  int Code() const;
};


/********************************************************************
 *   These defines are for use with libraries, where global
 *   static variables are not always initialized correctly.
 *   Instead of creating global statics, they will create a
 *   function which returns a reference to a local static.  The
 *   local static approach seems to work reliably, as local
 *   statics are not initialized until the first call to the
 *   function.  
 *
 *   Notice that function call syntax must be used when using
 *   Description() from one error message to build another error
 *   message.
 *******************************************************************/
#define DEFINE_FEEDBACK_ERROR(Identifier, Description) \
const FeedbackError& Identifier() \
{ \
  static FeedbackError e(Description); \
  return e; \
}

#define DECLARE_FEEDBACK_ERROR(Identifier) \
const FeedbackError& Identifier();

// Forward declaration
class Feedback;


/********************************************************************
 *   class FeedbackCentral.  Provides centralized and synchronized
 *   processing of output.  Due to synchronization issues, be very
 *   careful when changing visibility (private->public) of member
 *   functions.
 *******************************************************************/
class FeedbackCentral
{
  int m_nInfoLevel;
  FeedbackCentral(std::ostream &outputstream);

  typedef std::map<pthread_t, std::string> TThreadMap;
  mutable TThreadMap m_threadDescriptions;
  std::string GetThreadDescription() const;

  std::ostream &m_outputstream;
  mutable pthread_mutex_t m_defaultOutputMutex, *m_pOutputMutex, m_selfMutex;

  std::set<std::string> m_showSet, m_hideSet;

  void SetOutputMutex(pthread_mutex_t *pMtx = 0);
  void LockMutex(pthread_mutex_t *pMtx) const;
  void UnlockMutex(pthread_mutex_t *pMtx) const;

  void WriteOutput(const std::string &sOutput) const;
  bool FindMatch(const std::string &sQry, const std::set<std::string> &sSet) const;
public:
  ~FeedbackCentral();
  static FeedbackCentral& Instance();

  void SetInfoLevel(int nLevel);
  int GetInfoLevel() const;
  void SetShowHide(std::string sShow, std::string sHide);

  void RegisterThreadDescription(std::string sDescription);

  int Error(const Feedback *pSender, const FeedbackError &error, const std::string &sAdditionalInfo) const;
  void Warning(const Feedback *pSender, const std::string &sMessage) const;
  void Info(const Feedback *pSender, int nLevel, const std::string &sMessage) const;
  void Output(const Feedback *pSender, const std::string &sMessage) const;
};



/********************************************************************
 *   IFeedbackStream and its descendants are a series of classes
 *   providing streaming abilities to the Feedback class.  Descendants
 *   should call their respective Error/Warning/Info function in
 *   FeedbackCentral from their destructors.
 *******************************************************************/
class IFeedbackStream
{
protected:
  std::stringstream m_ss;
  const Feedback *m_pSender;
public:
  IFeedbackStream(const Feedback *pSender);
  IFeedbackStream(const IFeedbackStream &rhs);
  virtual ~IFeedbackStream();
  template<class T>IFeedbackStream& operator<<(const T &t)
  {
    m_ss << t;
    return *this;
  }
};


class ErrorStream : public IFeedbackStream
{
  const FeedbackError &m_error;
  bool m_bHaveError;
public:
  ErrorStream(const Feedback *pSender, const FeedbackError &error, bool bHaveError = true);
  virtual ~ErrorStream();
  operator int() const;
      // Need to reimplement operator<< in this class, in order to return
      // ErrorStream and not IFeedbackStream, otherwise the operator
      // int() doesn't work.
  template<class T>ErrorStream& operator<<(const T &t)
  {
    IFeedbackStream::operator<<(t);
    return *this;
  }
};


class WarningStream : public IFeedbackStream
{
  bool m_bHaveError;
public:
  WarningStream(const Feedback *pSender, const std::string &sMessage, bool bHaveError = true);
  virtual ~WarningStream();
};


class InfoStream : public IFeedbackStream
{
  int m_nLevel;
public:
  InfoStream(const Feedback *pSender, int nLevel, const std::string &sMessage);
  virtual ~InfoStream();
};


class OutputStream : public IFeedbackStream
{
public:
  OutputStream(const Feedback *pSender, const std::string &sMessage);
  virtual ~OutputStream();
};


/********************************************************************
 *   Feedback is the class to be used outside of this library.
 *******************************************************************/
class Feedback
{
  std::string m_sIdentifier;
public:
  Feedback(const std::string &sIdentifier);
  Feedback(const Feedback &rhs);

  Feedback& operator=(const Feedback &rhs);
  
  static void RegisterThreadDescription(std::string sDescription);

  ErrorStream Error(const FeedbackError &error) const;
  ErrorStream Error(const FeedbackError &(*provider)()) const;
  WarningStream Warning(const std::string &sMessage = "") const;
  InfoStream Info(int nLevel, const std::string &sMessage = "") const;
  OutputStream Output(const std::string &sMessage = "") const;

  ErrorStream ErrorIfNonzero(int nCond, const FeedbackError &error) const;
  ErrorStream ErrorIfNonzero(int nCond, const FeedbackError &(*provider)()) const;
  WarningStream WarningIfNonzero(int nCond, const std::string &sMessage = "") const;

  const std::string& Identify() const;
  void SetIdentifier(const std::string &sNewIdentifier);

  static void SetInfoLevel(int nLevel);
  static int GetInfoLevel();
  static void SetShowHide(const std::string &sShow, const std::string &sHide);
};


    // Include commonly used error codes.
#include "errorcodes_common.h"
#include "errorcodes_thread.h"


#endif
