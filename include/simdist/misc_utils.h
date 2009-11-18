/********************************************************************
 *   		misc_utils.h
 *   Created on Mon May 22 2006 by Boye A. Hoeverstad.
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
 *   Miscellaneous utilities that I'm unable to find a better name
 *   for...
 *******************************************************************/

#if !defined(__MISC_UTILS_H__)
#define __MISC_UTILS_H__

#include "feedback.h"
#include "syncutils.h"

#include <vector>
#include <functional>

//#define USE_GNU_EXT_FILEBUF 1

#if defined(USE_GNU_EXT_FILEBUF)
#include <ext/stdio_filebuf.h>
#endif

// extern FeedbackError E_UTILS_PIPE;
DECLARE_FEEDBACK_ERROR(E_UTILS_PIPE)
// extern FeedbackError E_UTILS_CREATE;
DECLARE_FEEDBACK_ERROR(E_UTILS_CREATE)
// extern FeedbackError E_UTILS_FD;
DECLARE_FEEDBACK_ERROR(E_UTILS_FD)
DECLARE_FEEDBACK_ERROR(E_UTILS_BROKEN_PIPE)
DECLARE_FEEDBACK_ERROR(E_UTILS_SYS)


/********************************************************************
 *   Base class for custom I/O buffers, derived from descriptions in
 *   Josuttis: The C++ Standard Library, 1999, ca p. 670, but with a
 *   different input buffer implementation (see source for further
 *   comments).
 *
 *******************************************************************/
class custiobufbase : public std::streambuf
{
public:
      // Size of buffer and putback area inside buffer for input.
  enum { putbacksize = 8, buffersize = 256 } EBufSize;
protected:
      // Output buffer functions
  virtual int_type overflow (int_type c);
  virtual std::streamsize xsputn(const char *s, std::streamsize num);
      // Input buffer functions
  char m_szBuf[buffersize];
  virtual int_type underflow();
  virtual std::streamsize xsgetn(char *s, std::streamsize num);

  virtual ssize_t DoRead(void *pBuf, size_t nCount) = 0;
  virtual ssize_t DoWrite(const void *pBuf, size_t nCount) = 0;
public:
  custiobufbase();
  virtual bool is_open() const = 0;
};


/********************************************************************
 *   I/O buffer intended to be connected to an open posix file
 *   descriptor.
 *
 *   NOTE: Will not automatically close the file descriptor on destroy.
 *******************************************************************/
class fdiobuf : public custiobufbase
{
protected:
  int m_nFd;
  virtual ssize_t DoRead(void *pBuf, size_t nCount);
  virtual ssize_t DoWrite(const void *pBuf, size_t nCount);
public:
  fdiobuf();
  fdiobuf(int nFd);
  void set_fd(int nFd);
  virtual bool is_open() const;
  int close();
  int fd() const;
};


class MemoryPipe;

/********************************************************************
 *   I/O buffer intended to be connected to a MemoryPipe.
 *
 *   NOTE: Will not close or delete the pipe to which it is connected
 *   on destroy.
 *******************************************************************/
class mpiobuf : public custiobufbase
{
protected:
  int m_nFd;
  MemoryPipe *m_pPipe;
  virtual ssize_t DoRead(void *pBuf, size_t nCount);
  virtual ssize_t DoWrite(const void *pBuf, size_t nCount);
public:
  mpiobuf();
  mpiobuf(MemoryPipe *pPipe);
  void set_pipe(MemoryPipe *pPipe);
  virtual bool is_open() const;
  int close();
  MemoryPipe* pipe() const;
};



/********************************************************************
 *   I/O streams intended for use with a posix file descriptor (using
 *   above buffer).  Construct or open with a file descriptor that
 *   already represents an open file.
 *
 *   NOTE: Since the file has already been opened externally, the
 *   default behaviour is to NOT close the file in the destructor.
 *******************************************************************/
#if !defined(USE_GNU_EXT_FILEBUF)
class fdstream
{
protected:
  fdiobuf m_buf;
public:
  fdstream(int nFd);
  fdstream();
  void set_fd(int nFd);
  bool is_open() const;
  int close();
  int fd() const;
};

class fdostream : public fdstream, public std::ostream
{
public:  
  fdostream(int nFd);
  fdostream();
};

class fdistream : public fdstream, public std::istream
{
public:  
  fdistream(int nFd);
  fdistream();
};

#else

typedef __gnu_cxx::stdio_filebuf<char> TFileBuf;
class fdostream : public std::ofstream
{
  std::auto_ptr<TFileBuf> m_pBuf;
  __filebuf_type *m_pOldBuf;
public:  
  fdostream(int nFd);
  fdostream();
  void set_fd(int nFd);
  int close();
  int fd() const;
  bool is_open() const;
};

class fdistream : public std::ifstream
{
  std::auto_ptr<TFileBuf> m_pBuf;
  __filebuf_type *m_pOldBuf;
public:  
  fdistream(int nFd);
  fdistream();
  void set_fd(int nFd);
  int close();
  int fd() const;
  bool is_open() const;
};

#endif


/********************************************************************
 *   I/O streams intended for use with a MemoryPipe.  The pipe should
 *   be created beforehand.
 *
 *   NOTE: Since the pipe has already been created, the default
 *   behaviour is to NOT close or delete the pipe in the destructor.
 *
 *   This class and fdstream above could perhaps be combined into a
 *   single class (template on argument to constructor?)
 *
 *******************************************************************/
class mpstream 
{
protected:
  mpiobuf m_buf;
public:
  mpstream(MemoryPipe *pPipe);
  mpstream();
  void set_pipe(MemoryPipe *pPipe);
  bool is_open() const;
  int close();
  MemoryPipe* pipe() const;
};

class mpostream : public mpstream, public std::ostream
{
public:  
  mpostream(MemoryPipe *pPipe);
  mpostream();
};

class mpistream : public mpstream, public std::istream
{
public:  
  mpistream(MemoryPipe *pPipe);
  mpistream();
};



/********************************************************************
 *   A memory-only pipe which implements per-thread blocking, as
 *   opposed to the per-process blocking performed by POSIX pipes.
 *******************************************************************/
class MemoryPipe
{
  bool BufferHasData() const;
  bool BufferHasSpace() const;
  LockableObject m_mtx;
  Condition m_condBufData, m_condBufSpace;
  std::vector<char> m_buf;
  size_t m_nCurBufSize;
  MemoryPipe(MemoryPipe&);
  bool m_bIsOpen;
public:
  explicit MemoryPipe(int nBufSize = 1024);
  ssize_t Read(void *pBuf, size_t nNumBytes);
  ssize_t Write(const void *pBuf, size_t nNumBytes);

  void Close();
  bool IsOpen();
};


/********************************************************************
 *   Create a unix pipe and return the unix file descriptors in
 *   fildes.  Fildes must be a pointer to an array of at least two
 *   integers.
 *******************************************************************/
int CreatePipe(int *fildes);


/********************************************************************
 *   Imitates a buffered pipe by adding a memory buffer in
 *   between two unix pipes.  The pipes should be opened
 *   beforehand.
 *******************************************************************/
int BufferPipe(size_t nBufSize, int nFdRead, int nFdWrite);


/********************************************************************
 *   Strip a stream of comments and empty lines.  Comment lines start
 *   with a pound sign (#).
 *******************************************************************/
int UncommentStream(std::istream &source, std::ostream &sink);

/********************************************************************
 *   Build an argv-like vector an argument string.  Extremely simple
 *   processing so far.
 *******************************************************************/
int CreateArgumentVector(const std::string &sArgLine, std::vector<std::string> &argv);


/********************************************************************
 *   Load a process and connect standard input, output and error of
 *   the program to the stl streams given as arguments.  All or any of
 *   the stream arguments may be 0.  The first version takes the first
 *   argument of argVec to be the process to load.  The other version
 *   loads sFile, in which case the string sArgs should NOT contain
 *   the program name as first argument.  
 *
 *   pnFileDescriptors are the descriptors of the pipes connected
 *   to the child process.  pnFileDescriptors[0] is the WRITE end
 *   of child STDIN, pnFileDescriptors[1] and
 *   pnFileDescriptors[2] are the READ ends of child STDOUT and
 *   STDERR, respectively.
 *******************************************************************/
int ConnectProcess(const std::vector<std::string> &argVec, 
                   fdostream *pWriteStdin, fdistream *pReadStdout, fdistream *pReadStderr,
                   pid_t *pChildPid, int *pnFileDescriptors);
int ConnectProcess(const std::string &sFile, const std::string &sArgs, 
                   fdostream *pWriteStdin, fdistream *pReadStdout, fdistream *pReadStderr,
                   pid_t *pChildPid, int *pnFileDescriptors);


/********************************************************************
 *   Kill a process.  Try to kill it nicely first. Give it a little
 *   time to go down, then kill it the hard way.
 *******************************************************************/
int KillProcess(pid_t pid, const std::string &sChildName, Feedback &fb);


/********************************************************************
 *   Wrapper for new-style POSIX signal handling.  Based on
 *   "Computer Systems, a Programmer's Perspective", by Bryant &
 *   O'Hallaron, 2003, sec. 8.5 p. 631.
 *******************************************************************/
int Signal(int nSignal, void (*pHandler) (int), void (**ppOldHandler) (int) = 0);


/********************************************************************
 *   Return string description (i.e. "SIGINT") for a signal.
 *******************************************************************/
std::string SignalToString(int nSignal);

/********************************************************************
 *   Helper function to be able to send a functor as reference.
 *   Kept for reference, but other and better methods exist: Use
 *   the return from for_each, or check Josuttis p. 298.
 *******************************************************************/
// template<class Op, class Arg, class Ret>
// struct FunctorReference
// {
//   Op &m_op;
//   FunctorReference(Op &op)
//       : m_op(op)
//   {
//   }
  
//   Ret operator()(Arg &arg)
//   {
//     return m_op(arg);
//   }
// };




/********************************************************************
 *   Functor to facilitate streaming in to a sequence with a call to
 *   for_each.
 *******************************************************************/
template<class T, class Stream = std::istream>
struct stream_in
{
  Stream &m_s;
  stream_in(Stream &s)
      : m_s(s)
  {
  }

  void operator()(T &t)
  {
    m_s >> t;
  }
};



/********************************************************************
 *   Structs to remove reference for a function that normally takes a
 *   reference.  For use with the binder functions of stl, which will
 *   otherwise run into a "reference to reference" error (see for
 *   instance http://gcc.gnu.org/bugzilla/show_bug.cgi?id=7412 or
 *   http://gcc.gnu.org/ml/libstdc++/2000-06/msg00220.html).  Another
 *   option is /usr/include/c++/4.0.3/bits/stl_function.h.
 *******************************************************************/

template<class Arg, class Ret> 
class unref1_t : public std::unary_function<Arg, Ret>
{
  Ret (*m_op) (Arg const&);
public:
  explicit unref1_t(Ret (*op)(Arg const&))
  : m_op(op)
  {
  }

  Ret
  operator()(Arg arg) const
  {
    return m_op(arg);
  }
};

template<class Arg, class Ret>
unref1_t<Arg, Ret>
inline unref(Ret (*op)(Arg const&))
{
  return unref1_t<Arg, Ret>(op);
}



template<class Arg1, class Arg2, class Ret> 
class unref2_t : public std::binary_function<Arg1, Arg2, Ret>
{
  Ret (*m_op) (Arg1 const&, Arg2 const&);
public:
  explicit unref2_t(Ret (*op)(Arg1 const&, Arg2 const&))
  : m_op(op)
  {
  }

  Ret
  operator()(Arg1 arg1, Arg2 arg2) const
  {
    return m_op(arg1, arg2);
  }
};

template<class Arg1, class Arg2, class Ret>
unref2_t<Arg1, Arg2, Ret>
inline unref(Ret (*op)(Arg1 const&, Arg2 const&))
{
  return unref2_t<Arg1, Arg2, Ret>(op);
}

// template<class OP> 
// struct unref_t : public std::unary_function<typename OP::argument_type,
//                                           typename OP::result_type>
// {
//   OP m_op;

//   unref_t(OP op)
//       : m_op(op)
//   {
//   }

//   typename OP::result_type
//   operator()(const typename OP::argument_type arg) const
//   {
//     return m_op(arg);
//   }
// };

// template<class OP>
// unref_t<OP>
// inline unref(const OP &op)
// {
//   return unref_t<OP>(op);
// }


// template<class OP> 
// struct unref2_t : public std::binary_function<typename OP::first_argument_type,
//                                               typename OP::second_argument_type,
//                                               typename OP::result_type>
// {
//   OP m_op;

//   unref2_t(OP op)
//       : m_op(op)
//   {
//   }

//   typename OP::result_type
//   operator()(const typename OP::first_argument_type arg1, const typename OP::second_argument_type arg2) const
//   {
//     return m_op(arg1, arg2);
//   }
// };


// /********************************************************************
//  *   Is there a way to overload unref, so the compiler will choose the
//  *   correct struct to create?  See stl_function.h, mem_fun
//  *   implementation.
//  *******************************************************************/
// template<class OP>
// unref2_t<OP>
// inline unref2(const OP &op)
// {
//   return unref2_t<OP>(op);
// }


template <class C, class Arg, class Ret>
class mem_fun_ref_unref1_t : public std::binary_function<C, Arg, Ret>
{
  Ret (C::*m_f)(Arg const&);
public:
  explicit mem_fun_ref_unref1_t(Ret (C::*pf)(Arg const&)) 
  : m_f(pf) 
  {
  }
  
  Ret
  operator()(C &c, Arg x) const 
  { 
    return (c.*m_f)(x); 
  }
};


template <class C, class Arg, class Ret>
class const_mem_fun_ref_unref1_t : public std::binary_function<C, Arg, Ret>
{
  Ret (C::*m_f)(Arg const&) const;
public:
  explicit const_mem_fun_ref_unref1_t(Ret (C::*pf)(Arg const&) const) 
  : m_f(pf) 
  {
  }
  
  Ret
  operator()(const C &c, Arg x) const 
  { 
    return (c.*m_f)(x); 
  }
};


template <class C, class Arg, class Ret>
inline mem_fun_ref_unref1_t<C, Arg, Ret>
mem_fun_ref_unref(Ret (C::*f)(Arg const&))
{ 
  return mem_fun_ref_unref1_t<C, Arg, Ret>(f);
}

template <class C, class Arg, class Ret>
inline const_mem_fun_ref_unref1_t<C, Arg, Ret>
mem_fun_ref_unref(Ret (C::*f)(Arg const&) const)
{ 
  return const_mem_fun_ref_unref1_t<C, Arg, Ret>(f);
}


bool
WildcardMatch(const char *w, const char *s);

std::string::size_type
FindReplace(std::string &sString, const std::string &sSearch, 
            const std::string &sReplace, std::string::size_type nIndex = 0);

void
FindReplaceAll(std::string &sString, const std::string &sSearch, const std::string &sReplace);


template<class T>
std::basic_string<T>
Trim(const std::basic_string<T> &t)
{
  typename std::basic_string<T>::size_type nBegin = t.find_first_not_of(" \t");
  typename std::basic_string<T>::size_type nEnd = t.find_last_not_of(" \t");
  if (nBegin == std::string::npos)
    return std::basic_string<T>();
  else
    return t.substr(nBegin, nEnd - nBegin + 1);
}
  
  
// template <class Ret, class C, class Arg1, class Arg2>
// class mem_fun_ref_unref2_t
// {
//   Ret (C::*m_f)(Arg1 const&, Arg2 const&);
// public:
//   explicit mem_fun_ref_unref2_t(Ret (C::*pf)(Arg1 const&, Arg2 const&)) 
//   : m_f(pf)
//   {
//   }
  
//   Ret
//   operator()(C* p, Arg1 x, Arg2 y) const 
//   { 
//     return (p->*m_f)(x); 
//   }
// };


// template <class Ret, class C, class Arg1, class Arg2>
// inline mem_fun_ref_unref2_t<Ret, C, Arg1, Arg2>
// mem_fun_ref_unref(Ret (C::*f)(Arg1 const&, Arg2 const&))
// { 
//   return mem_fun_ref_unref2_t<Ret, C, Arg1, Arg2>(f);
// }



#endif
