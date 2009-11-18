/********************************************************************
 *   		misc_utils.cpp
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
 *   See header file for description.
 *******************************************************************/

#include <simdist/misc_utils.h>

#include <set>
#include <vector>
#include <iostream>
#include <fstream>
#include <cassert>

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <poll.h>
#include <cassert>
#include <cstring>

// FeedbackError E_UTILS_PIPE("Failed to create pipe");
DEFINE_FEEDBACK_ERROR(E_UTILS_PIPE, "Failed to create pipe")
// FeedbackError E_UTILS_CONNECT("Failed to launch and/connect to child process");
DEFINE_FEEDBACK_ERROR(E_UTILS_CONNECT, "Failed to launch and/connect to child process")
// FeedbackError E_UTILS_FD("Failed assing unix file descriptor to stl stream");
DEFINE_FEEDBACK_ERROR(E_UTILS_FD, "Failed assing unix file descriptor to stl stream")
// FeedbackError E_WAITPID_FAIL("waitpid failed (return value -1)");
DEFINE_FEEDBACK_ERROR(E_WAITPID_FAIL, "waitpid failed (return value -1)")
// FeedbackError E_KILL_FAILED("Failed to kill child process (even after several attempts)");
DEFINE_FEEDBACK_ERROR(E_KILL_FAILED, "Failed to kill child process (even after several attempts)")

DEFINE_FEEDBACK_ERROR(E_UTILS_BROKEN_PIPE, "Broken pipe: Output pipe failed with data still in write buffer.")
DEFINE_FEEDBACK_ERROR(E_UTILS_SYS, "A system call failed unexpectedly")

custiobufbase::custiobufbase()
{
      // Initialize pointers to force underflow
  setg(m_szBuf + putbacksize, 
       m_szBuf + putbacksize,
       m_szBuf + putbacksize);
}


custiobufbase::int_type
custiobufbase::overflow (int_type c)
{
  if (!is_open())
    return traits_type::eof();

  if (!traits_type::eq_int_type(c, traits_type::eof()))
  {
    char z = c;
    if (DoWrite(&z, 1) != 1)
      return traits_type::eof();
  }
  return c;
}


std::streamsize
custiobufbase::xsputn(const char *s, std::streamsize num)
{
  if (is_open())
    return DoWrite(s, num);
  return -1;
}



custiobufbase::int_type
custiobufbase::underflow()
{
  if (!is_open())
    return traits_type::eof();

      // is read position before end of buffer?
  if (gptr() < egptr())
  {
    return traits_type::to_int_type(*this->gptr());
  }
  
      // Process size of putback area. Use number of characters read,
      // but at most four.
  int nNumPutback = gptr() - eback();
  if (nNumPutback > putbacksize)
  {
    nNumPutback = putbacksize;
  }

      // Copy up to putbacksize characters previously read into the
      // putback buffer (area of first putbacksize characters)
  memcpy(m_szBuf + (putbacksize - nNumPutback), 
              gptr() - nNumPutback,
              nNumPutback);
  
      // Read new characters.
  ssize_t nNumRead = DoRead(m_szBuf + putbacksize, buffersize - putbacksize);
//   std::cerr << "underflow: read " << nNumRead << " bytes (tried for " 
//             << buffersize - putbacksize << ").\n";

  if (nNumRead <= 0)
  {
//     std::cerr << "nNumRead <= 0, returning EOF.\n";
    return traits_type::eof();
  }

      // Reset buffer pointers
  setg(m_szBuf + (putbacksize - nNumPutback),  // beginning of putback area
       m_szBuf + putbacksize,                  // read position
       m_szBuf + putbacksize + nNumRead);      // end of buffer

      // Return next character
  return traits_type::to_int_type(*this->gptr());
}


std::streamsize
custiobufbase::xsgetn(char* s, std::streamsize nNum)
{
  if (!is_open())
    return 0;

  std::streamsize nNumCopied = std::min(this->egptr() - this->gptr(), nNum);
  traits_type::copy(s, this->gptr(), nNumCopied);
  this->gbump(nNumCopied);
  if (nNumCopied == nNum)
  {
    return nNumCopied;
  }

      // More requested than was available in the buffer: Read from
      // file.  Read until we either reach eof or we have as many
      // bytes as requested.  Two reasons for this: The caller will
      // mark an error on the stream if we return less than requested,
      // and a read on a pipe will not block correctly if there are
      // only a few bytes available.
  s += nNumCopied;
  ssize_t nNumRead = DoRead(s, nNum - nNumCopied);
  while (nNumCopied + nNumRead < nNum)
  {
    ssize_t nNumReadNow = DoRead(s + nNumRead, nNum - (nNumCopied + nNumRead));
    if (nNumReadNow <= 0)
      break;
    nNumRead += nNumReadNow;
  }
      // If we get more from file, we need to adjust the putback
      // buffer.
  if (nNumRead > 0)
  {
    int nNumPutbackRead = std::min(static_cast<size_t>(nNumRead), static_cast<size_t>(putbacksize));
    int nNumPutbackBuf = std::min(static_cast<int>(putbacksize - nNumPutbackRead), static_cast<int>(this->gptr() - this->eback()));
    if (nNumPutbackBuf < 0)
      nNumPutbackBuf = 0;
    int nNumPutback = nNumPutbackRead + nNumPutbackBuf;
    char *pPutback = m_szBuf + (putbacksize - nNumPutback);
    memmove(pPutback, this->gptr() - nNumPutbackBuf, nNumPutbackBuf);
    memcpy(pPutback + nNumPutbackBuf, s + nNumRead - nNumPutbackRead, nNumPutbackRead);
  
    this->setg(pPutback,
               pPutback + nNumPutback,
               pPutback + nNumPutback);
  }
  return nNumRead + nNumCopied;
}


fdiobuf::fdiobuf()
    : m_nFd(-1)
{
}


fdiobuf::fdiobuf(int nFd)
    : m_nFd(nFd)
{
}

void
fdiobuf::set_fd(int nFd)
{
  m_nFd = nFd;
}


bool
fdiobuf::is_open() const
{
  return m_nFd != -1;
}


int
fdiobuf::close()
{
  int nFd = m_nFd;
  m_nFd = -1;
  return ::close(nFd);
}


int
fdiobuf::fd() const
{
  return m_nFd;
}


ssize_t
fdiobuf::DoRead(void *pBuf, size_t nCount)
{
  if (!is_open())
    return -1;
  return ::read(m_nFd, pBuf, nCount);
}


ssize_t
fdiobuf::DoWrite(const void *pBuf, size_t nCount)
{
  if (!is_open())
    return -1;
  return ::write(m_nFd, pBuf, nCount);
}



mpiobuf::mpiobuf()
    : m_pPipe(0)
{
}


mpiobuf::mpiobuf(MemoryPipe *pPipe)
    : m_pPipe(pPipe)
{
}

void
mpiobuf::set_pipe(MemoryPipe *pPipe)
{
  m_pPipe = pPipe;
}


bool
mpiobuf::is_open() const
{
  return m_pPipe != 0 && m_pPipe->IsOpen();
}


int
mpiobuf::close()
{
  if (!m_pPipe)
    return -1;
  m_pPipe->Close();
  return 0;
}


MemoryPipe*
mpiobuf::pipe() const
{
  return m_pPipe;
}


ssize_t
mpiobuf::DoRead(void *pBuf, size_t nCount)
{
  if (!is_open())
    return -1;
  return m_pPipe->Read(pBuf, nCount);
}


ssize_t
mpiobuf::DoWrite(const void *pBuf, size_t nCount)
{
  if (!is_open())
    return -1;
  return m_pPipe->Write(pBuf, nCount);
}


#if !defined(USE_GNU_EXT_FILEBUF)

fdstream::fdstream(int nFd)
    : m_buf(nFd)
{
}


fdstream::fdstream()
{
}
  

void 
fdstream::set_fd(int nFd)
{
  m_buf.set_fd(nFd);
}


int
fdstream::close()
{
  return m_buf.close();
}


int
fdstream::fd() const
{
  return m_buf.fd();
}


bool
fdstream::is_open() const
{
  return m_buf.is_open();
}


fdostream::fdostream(int nFd)
    : fdstream(nFd), std::ostream(&m_buf)
{
}


fdostream::fdostream()
    : std::ostream(&m_buf)
{
}


fdistream::fdistream(int nFd)
    : fdstream(nFd), std::istream(&m_buf)
{
}


fdistream::fdistream()
    : std::istream(&m_buf)
{
}


#else

fdistream::fdistream(int nFd)
    : m_pBuf(new TFileBuf(nFd, std::ios_base::in))
{
  m_pOldBuf = rdbuf();
  std::basic_ios<char>::rdbuf(m_pBuf.get());
}


fdistream::fdistream()
    : m_pBuf(0)
{
  m_pOldBuf = rdbuf();
}


void 
fdistream::set_fd(int nFd)
{
  m_pBuf = std::auto_ptr<TFileBuf>(new TFileBuf(nFd, std::ios_base::in));
  std::basic_ios<char>::rdbuf(m_pBuf.get());
}


int
fdistream::close()
{
  int nFd = fd();
  m_pBuf.reset();
  std::basic_ios<char>::rdbuf(m_pOldBuf);
  return ::close(nFd);
}


int
fdistream::fd() const
{
  if (!m_pBuf.get())
    return -1;
  return m_pBuf->fd();
}


bool
fdistream::is_open() const
{
  return m_pBuf.get() != 0;
}


fdostream::fdostream(int nFd)
    : m_pBuf(new TFileBuf(nFd, std::ios_base::out))
{
  m_pOldBuf = rdbuf();
  std::basic_ios<char>::rdbuf(m_pBuf.get());
}


fdostream::fdostream()
    : m_pBuf(0)
{
  m_pOldBuf = rdbuf();
}


void 
fdostream::set_fd(int nFd)
{
  m_pBuf = std::auto_ptr<TFileBuf>(new TFileBuf(nFd, std::ios_base::out));
  std::basic_ios<char>::rdbuf(m_pBuf.get());
}


int
fdostream::close()
{
  int nFd = fd();
  m_pBuf.reset();
  std::basic_ios<char>::rdbuf(m_pOldBuf);
  return ::close(nFd);
}


int
fdostream::fd() const
{
  if (!m_pBuf.get())
    return -1;
  return m_pBuf->fd();
}


bool
fdostream::is_open() const
{
  return m_pBuf.get() != 0;
}


#endif


mpstream::mpstream(MemoryPipe *pPipe)
    : m_buf(pPipe)
{
}


mpstream::mpstream()
{
}
  

void 
mpstream::set_pipe(MemoryPipe *pPipe)
{
  m_buf.set_pipe(pPipe);
}


int
mpstream::close()
{
  return m_buf.close();
}


MemoryPipe*
mpstream::pipe() const
{
  return m_buf.pipe();
}


bool
mpstream::is_open() const
{
  return m_buf.is_open();
}


mpostream::mpostream(MemoryPipe *pPipe)
    : mpstream(pPipe), std::ostream(&m_buf)
{
}


mpostream::mpostream()
    : std::ostream(&m_buf)
{
}


mpistream::mpistream(MemoryPipe *pPipe)
    : mpstream(pPipe), std::istream(&m_buf)
{
}


mpistream::mpistream()
    : std::istream(&m_buf)
{
}



MemoryPipe::MemoryPipe(int nBufSize)
    : m_condBufData()
    , m_condBufSpace()
    , m_buf(nBufSize), m_nCurBufSize(0), m_bIsOpen(true)
{
}


bool 
MemoryPipe::BufferHasData() const
{
  return m_nCurBufSize > 0 || !m_bIsOpen;
}


bool
MemoryPipe::BufferHasSpace() const
{
  return m_nCurBufSize < m_buf.size();
}


/********************************************************************
 *   Read nNumBytes from pipe/memory buffer to pBuf.  Reads up to
 *   nNumBytes from pBuf.  Returns when the requested number of bytes
 *   has been read, or when the buffer is empty, whichever happens
 *   first.  If the buffer is empty on entry, the function will block.
 *   When the end of the stream has been reached, no bytes are read
 *   and the function returns immediately.
 *******************************************************************/
ssize_t
MemoryPipe::Read(void *pBuf, size_t nNumBytes)
{
  ssize_t nBytesRead = 0;
  if (nNumBytes > 0)
  {
    AutoMutex mtx;
    m_mtx.AcquireMutex(mtx);

    if (!m_bIsOpen && m_nCurBufSize == 0)
      return 0;
        // Wait for data to become available.  m_condBufData must also
        // check for !m_bIsOpen to avoid lock when an empty buffer is
        // closed, so check manually for data after wake-up.
    m_condBufData.Wait(mtx.GetLockedMutex(), std::mem_fun(&MemoryPipe::BufferHasData), this);
    if (m_nCurBufSize == 0)
      return 0;

        // Read data and adjust counters
    size_t nReadNow = std::min(nNumBytes, m_nCurBufSize);
    assert(nReadNow > 0 || !m_bIsOpen);
    memcpy(pBuf, &(m_buf[0]), nReadNow);
    nNumBytes -= nReadNow;
    nBytesRead += nReadNow;
    pBuf = static_cast<char*>(pBuf) + nReadNow;
    m_nCurBufSize -= nReadNow;
    memmove(&(m_buf[0]), &(m_buf[nReadNow]), m_nCurBufSize);
        // Up the "free space in buffer" semaphore, since we have
        // removed data.
    m_condBufSpace.Signal();
  }
  
  return nBytesRead;
}

  

/********************************************************************
 *   Write nNumBytes from pBuf to pipe/memory buffer.  This function
 *   will not return until all data in pBuf has been written (the
 *   return value is only to make it look like a normal write system
 *   call).
 *******************************************************************/
ssize_t
MemoryPipe::Write(const void *pBuf, size_t nNumBytes)
{
  ssize_t nBytesWritten = 0;

  while (nNumBytes > 0)
  {
    AutoMutex mtx;
    m_mtx.AcquireMutex(mtx);
        // Wait for free space in the buffer
    m_condBufSpace.Wait(mtx.GetLockedMutex(), std::mem_fun(&MemoryPipe::BufferHasSpace), this);
        // Signal broken pipe if writing when closed
    if (!m_bIsOpen)
      raise(SIGPIPE);
        // Write data and adjust counters
    size_t nWrittenNow = std::min(nNumBytes, m_buf.size() - m_nCurBufSize);
    assert(nWrittenNow > 0);
    memcpy(&(m_buf[m_nCurBufSize]), pBuf, nWrittenNow);
    nNumBytes -= nWrittenNow;
    pBuf = static_cast<const char*>(pBuf) + nWrittenNow;
    m_nCurBufSize += nWrittenNow;
    nBytesWritten += nWrittenNow;
        // Up the "data in buffer" semaphore if 0
    m_condBufData.Signal();
  }

  return nBytesWritten;
}


void 
MemoryPipe::Close()
{
  AutoMutex mtx;
  m_mtx.AcquireMutex(mtx);
  m_bIsOpen = false;
      // Wake up reader(s) waiting for data
  m_condBufData.Broadcast();
}


bool 
MemoryPipe::IsOpen()
{
  AutoMutex mtx;
  m_mtx.AcquireMutex(mtx);
      // Take a local copy; I'm not sure how the compiler orders
      // reading of m_bIsOpen vs destruction of mtx.
  bool bOpen = m_bIsOpen;
  return bOpen;
}



int
CreatePipe(int *fildes)
{
  Feedback fb("CreatePipe utility routine");

  if (!fildes)
    return fb.Error(E_UTILS_PIPE) << ". No storage for pipe file descriptors provided.";

  if (pipe(fildes))
  {
    std::string sErrMsg = strerror(errno);
    return fb.Error(E_UTILS_PIPE) << ". System error message: " << sErrMsg << ".";
  }

  return 0;
}
  

/********************************************************************
 *   Creates a buffered pipe.  nFdRead and nFdWrite are typically
 *   pipes, possibly to other processes.  This function will poll
 *   the pipes to a) read as much data as the internal buffer can
 *   store whenever data comes in, and b) only write data when
 *   the write will not block.
 *******************************************************************/
int
BufferPipe(size_t nBufSize, int nFdRead, int nFdWrite)
{
//   pthread_t self = pthread_self();
//   std::stringstream ss;
//   ss << "_bp_debug_output_" << self << ".txt";
//   std::ofstream dump(ss.str().c_str());
  
  Feedback fb("BufferPipe utility routine");

      // Set non-blocking mode for both pipes
  int nRFlags = fcntl(nFdRead, F_GETFL);
  int nWFlags = fcntl(nFdWrite, F_GETFL); 
  if (nRFlags < 0 || nWFlags < 0)
    return fb.Error(E_UTILS_SYS) << ". Could not get file flags from fcntl for descriptors "
                                 << nFdRead << " and/or " << nFdWrite << ". System error message: " << strerror(errno) << ".";
  if (fcntl(nFdRead, F_SETFL, nRFlags | O_NONBLOCK) == -1
      || fcntl(nFdWrite, F_SETFL, nWFlags | O_NONBLOCK) == -1)
    return fb.Error(E_UTILS_SYS) << ". Could not set flag O_NONBLOCK for descriptors " 
                                 << nFdRead << " and/or " << nFdWrite << ". System error message: " << strerror(errno) << ".";

      // Prepare buffer
  std::vector<char> buf(nBufSize);
  size_t nCurBufSize = 0;

      // Prepare polls.  Don't poll for POLLOUT yet, wait until
      // we have data in the buffer.
  static const int nNumPollFds = 2;
  struct pollfd pollFds[nNumPollFds] = {
    { nFdRead, POLLIN | POLLERR | POLLHUP, 0 },
    { nFdWrite, POLLERR | POLLHUP, 0 }};

  bool bReadClosed = false;

  while (true) // (nPollRet = poll(pollFds, nNumPollFds, -1)) >= 0) // Wait indefinitely for input or error
  {
//     dump << "polling. Read: " 
//               << (pollFds[0].events & POLLIN ? "POLLIN " : "")
//               << (pollFds[0].events & POLLOUT ? "POLLOUT " : "")
//               << (pollFds[0].events & POLLERR ? "POLLERR " : "")
//               << (pollFds[0].events & POLLHUP ? "POLLHUP " : "")
//               << ". Write: "
//               << (pollFds[1].events & POLLIN ? "POLLIN " : "")
//               << (pollFds[1].events & POLLOUT ? "POLLOUT " : "")
//               << (pollFds[1].events & POLLERR ? "POLLERR " : "")
//               << (pollFds[1].events & POLLHUP ? "POLLHUP " : "")
//               << "\n" << std::flush;
        // Accept EINTR; this will happen if we attach a debugger.
    int nPollRet = poll(pollFds, nNumPollFds, -1); // Wait indefinitely for input or error
    while (nPollRet == -1 && errno == EINTR)
      nPollRet = poll(pollFds, nNumPollFds, -1);
    if (nPollRet < 0)
      break;
//     dump << "Poll returned. Read: " 
//               << (pollFds[0].revents & POLLIN ? "POLLIN " : "")
//               << (pollFds[0].revents & POLLOUT ? "POLLOUT " : "")
//               << (pollFds[0].revents & POLLERR ? "POLLERR " : "")
//               << (pollFds[0].revents & POLLHUP ? "POLLHUP " : "")
//               << ". Write: "
//               << (pollFds[1].revents & POLLIN ? "POLLIN " : "")
//               << (pollFds[1].revents & POLLOUT ? "POLLOUT " : "")
//               << (pollFds[1].revents & POLLERR ? "POLLERR " : "")
//               << (pollFds[1].revents & POLLHUP ? "POLLHUP " : "")
//               << "\n" << std::flush;

        // Check the write end of the pipe first
    if (short revents = pollFds[1].revents)
    {
      if (revents & POLLERR || revents & POLLHUP)
      {
        close(nFdWrite);
        if (nCurBufSize == 0)
          return 0;
        close(nFdRead);
        return fb.Error(E_UTILS_BROKEN_PIPE);
      }
      
          // None of the above, then data can be written.  Write
          // as much as possible, then shift remaining buffer so
          // it starts at 0.
      assert(nCurBufSize || !"Error: Buffer should be non-empty when polling for POLLOUT.");
//       dump << self << " writing " << nCurBufSize << " bytes..." << std::flush;
      ssize_t nWritten = write(nFdWrite, &(buf[0]), nCurBufSize);
//       dump << self << " wrote " << nWritten << " bytes.\n" << std::flush;
      if (nWritten < 0)
        return fb.Error(E_UTILS_SYS) << ": \"write\" to descriptor " << nFdWrite 
                                     << " failed. System error message: " << strerror(errno) << "."; // Failed to write to pipe
      if (nWritten > 0)
        std::copy(buf.begin() + nWritten, buf.begin() + nCurBufSize, buf.begin());
      nCurBufSize -= nWritten;

          // We have written everything in the buffer.  Close and
          // return if there is no more input, otherwise pause
          // polling for POLLOUT.
      if (nCurBufSize == 0)
      {
        if (bReadClosed)
        {
          close(nFdWrite);
          return 0;
        }
        pollFds[1].events &= ~POLLOUT;
      }
          // Allow polling for POLLIN if the buffer is not full
          // (it shouldn't be).
      if (nCurBufSize != nBufSize)
        pollFds[0].events |= POLLIN;
    }

        // Now check read.  Errors first.
    if (short revents = pollFds[0].revents)
    {
      if (revents & POLLERR || revents & POLLHUP)
      {
        close(nFdRead);
        bReadClosed = true;
        if (nCurBufSize == 0)
        {
          close(nFdWrite);
          return 0;
        }
      }

          // I'm not sure if we may get POLLHUP and POLLIN at the
          // same time, check for POLLIN just to make sure.
      if (revents & POLLIN)
      {
            // Read as much as possible, adjust current buffer size accordingly.
        assert(nCurBufSize < nBufSize || "Error: Buffer should not be full when polling for POLLIN.");
//         dump << self << " reading " << nBufSize - nCurBufSize << " bytes..." << std::flush;
        ssize_t nRead = read(nFdRead, &(buf[nCurBufSize]), nBufSize - nCurBufSize);
//         dump << self << " read " << nRead << " bytes.\n" << std::flush;
        if (nRead < 0)
          return fb.Error(E_UTILS_SYS) << ": \"read\" from descriptor " << nFdRead 
                                       << " failed. System error message: " << strerror(errno) << "."; // Failed to read from pipe
        nCurBufSize += nRead;

            // Stop polling for more input if the buffer is full.
        if (nCurBufSize == nBufSize)
          pollFds[0].events &= ~POLLIN;
            // Poll for output if the buffer is non-empty.
        if (nCurBufSize > 0)
          pollFds[1].events |= POLLOUT;
      }
    }

        // Reset returned events.  Make sure to always reset
        // both, since we test these values even though we may
        // not have polled for I/O on both descriptors.
    pollFds[0].revents = 0;
    pollFds[1].revents = 0;
  }
  
  return fb.Error(E_UTILS_SYS) << ": \"poll\" failed. System error message: " << strerror(errno) << ".";
}




/********************************************************************
 *   Strip a stream of comments and empty lines.  Comment lines
 *   start with a pound sign (#).  Could perhaps have used
 *   boost::basic_regex_filter with boost::filtering_stream.
 *******************************************************************/
int
UncommentStream(std::istream &source, std::ostream &sink)
{
  const char comment_char = '#';

  std::string sLine;
  while(std::getline(source, sLine))
  {
        // Cut off everything after and including the comment character
    std::string::size_type nCommentPos = sLine.find(comment_char);
    std::string::size_type nNonBlankPos = sLine.find_first_not_of(" \t");
        // Blank line or pure comment line
    if (nCommentPos == nNonBlankPos) 
      continue;
        // Line with trailing comment # like this one..
    if (nCommentPos != std::string::npos)
      sLine.resize(nCommentPos);
    sink << sLine << "\n";
  }
  return 0;
}


/********************************************************************
 *   Parse a line of arguments into a vector of arguments.  Currently
 *   supports quote delimited arguments, but has no error detection.
 *   Supports single quotes inside double quotes and vice versa, but
 *   no further nesting, nor escaping of quotes.
 *******************************************************************/
int
CreateArgumentVector(const std::string &sArgLine, std::vector<std::string> &argv)
{
  std::string::const_iterator itFront = sArgLine.begin();
  std::string::const_iterator itEnd = sArgLine.end();

      // The default delimiter
  std::string::value_type cDelimDefault = ' ';
      // Pushable delimiters
  std::string sDelimPush = "\"'";
      // Current delimiter
  std::string::value_type cDelim = ' ';

  std::string sCurArg;
  while (itFront != itEnd)
  {
    std::string::value_type c = *itFront;
        // Is current character the delimiter?
    if (c == cDelim)
    {
      if (!sCurArg.empty())
        argv.push_back(sCurArg);
      sCurArg.clear();
      cDelim = cDelimDefault;
    }
    else
    {
          // Check if current char is an alternative delimiter.  If
          // so, and if we are not already using an alternative
          // delimiter (e.g., we find a ' inside a "" delimited
          // argument), restart search using this delim.
      if (sDelimPush.find(c) != std::string::npos
          && cDelim == cDelimDefault)
      {
        if (!sCurArg.empty())
          argv.push_back(sCurArg);
        sCurArg.clear();
        cDelim = c;
      }
      else
        sCurArg += c;
    }
    itFront++;
  }
  if (!sCurArg.empty())
    argv.push_back(sCurArg);

  return 0;
}


int 
ConnectProcess(const std::string &sFile, const std::string &sArgs, 
               fdostream *pWriteStdin, fdistream *pReadStdout, fdistream *pReadStderr,
               pid_t *pChildPid, int *pnFileDescriptors)
{
  Feedback fb("ConnectProcess utility routine");
  std::vector<std::string> argVec;
  if (CreateArgumentVector(sArgs, argVec))
    return fb.Error(E_UTILS_CONNECT) << ": Failed to parse child process argument line.";
  argVec.insert(argVec.begin(), sFile);
  return ConnectProcess(argVec, pWriteStdin, pReadStdout, pReadStderr, pChildPid, pnFileDescriptors);
}

// This one only prints out info
// #define PIDPIPE(x) std::cerr << "Pid " << getpid() << ": created pipe " << #x << "; fds " << x[0] << " and " << x[1] << "\n" << std::flush
// Whereas this one also performs the actual close.
// #define PIDCLOSE(x) { std::cerr << "Pid " << getpid() << ": closing " << #x << " (fd " << x << ").\n" << std::flush; close(x); }

int 
ConnectProcess(const std::vector<std::string> &argVec, 
               fdostream *pWriteStdin, fdistream *pReadStdout, fdistream *pReadStderr,
               pid_t *pChildPid, int *pnFileDescriptors)
{
  Feedback fb("ConnectProcess utility routine");
  
  int nChildStdIn[2], nChildStdOut[2], nChildStdErr[2], nDummy[2];
  
      // In rare cases (such as when running non-interactively
      // through qsub), stdin is closed, so 0 is not in use.  In
      // this case, 'pipe' will use 0 as one of the file
      // descriptors, and the close/dup2 calls below will create
      // problems.  In order to avoid this, we open a dummy pipe
      // first, so 0 is taken.
  bool bUseDummy = fcntl(0, F_GETFD) == -1 && errno == EBADF;
  if (bUseDummy)
  {
    if (CreatePipe(nDummy))
      return fb.Error(E_UTILS_CONNECT) << ": Failed to create dummy pipe.";
  }

  if (CreatePipe(nChildStdIn) 
      || CreatePipe(nChildStdOut)
      || CreatePipe(nChildStdErr))
    return fb.Error(E_UTILS_CONNECT) << ": Unable to establish communication channel with child process.";

  if (bUseDummy)
  {
    close(nDummy[0]);
    close(nDummy[1]);
  }

//   PIDPIPE(nChildStdIn);
//   PIDPIPE(nChildStdOut);
//   PIDPIPE(nChildStdErr);

      // Set the close-on-exec flag on the parent end of the
      // parent-child communication pipes.  This is necessary to
      // avoid duplication of the parent pipes in subsequent
      // children if ConnectProcess is called several times.  If
      // these are not closed, waitpid is likely to hang if there
      // are multiple child processes that keep running until
      // their standard input is closed.
  fcntl(nChildStdIn[1], F_SETFD, 1);
  fcntl(nChildStdOut[0], F_SETFD, 1);
  fcntl(nChildStdErr[0], F_SETFD, 1);

  pid_t nPid = fork();
  if (pChildPid && nPid)
    *pChildPid = nPid;

  if (nPid == -1)
    return fb.Error(E_UTILS_CONNECT) << ": Failed to fork. System error message: " << strerror(errno) << ".";

  if (nPid)
  {
    if (pReadStdout)
      pReadStdout->set_fd(nChildStdOut[0]);
    if (pWriteStdin)
      pWriteStdin->set_fd(nChildStdIn[1]);
    if (pReadStderr)
      pReadStderr->set_fd(nChildStdErr[0]);
    
        // If pnFileDescriptors is also 0, we may close the unused
        // pipes.  The equivalent test is performed below for the
        // child; only dup2'ing standard communications channels if
        // the inverse of one of the tests below holds.
    if (!pnFileDescriptors)
    {
      if (!pWriteStdin)
        close(nChildStdIn[1]);
      if (!pReadStdout)
        close(nChildStdOut[0]);
      if (!pReadStderr)
        close(nChildStdErr[0]);
    }
    close(nChildStdIn[0]);
    close(nChildStdOut[1]);
    close(nChildStdErr[1]);
  }
  else
  {
        // Only dup2 if we have a pipe on the parent end (see above).
    if (((pnFileDescriptors || pWriteStdin) && dup2(nChildStdIn[0], 0) == -1)
        || ((pnFileDescriptors || pReadStdout) && dup2(nChildStdOut[1], 1) == -1)
        || ((pnFileDescriptors || pReadStderr) && dup2(nChildStdErr[1], 2) == -1))
      return fb.Error(E_UTILS_CONNECT) << ": Failed to connect pipes for I/O communication "
                                       << "with parent process to stdio descriptors.";
    close(nChildStdIn[0]);
    close(nChildStdIn[1]);
    close(nChildStdOut[0]);
    close(nChildStdOut[1]);
    close(nChildStdErr[0]);
    close(nChildStdErr[1]);

    std::vector<char*> argv(argVec.size() + 1, 0);
    for (size_t n = 0; n < argVec.size(); n++)
      argv[n] = const_cast<char*>(argVec[n].c_str());

        // Unblock all signals with blocking inherited from
        // parent.
    sigset_t sigSet;
    sigfillset(&sigSet);
    sigprocmask(SIG_UNBLOCK, &sigSet, 0);

    if (execvp(argv[0],&(argv[0])) == -1)
      return fb.Error(E_UTILS_CONNECT) << ": Failed to change process image (execvp) in child process to " 
                                       << argv[0] << ". System error message: " << strerror(errno) << ".";
  }

  if (pnFileDescriptors)
  {
    pnFileDescriptors[0] = nChildStdIn[1];
    pnFileDescriptors[1] = nChildStdOut[0];
    pnFileDescriptors[2] = nChildStdErr[0];
  }

  return 0;
}


int
KillProcess(pid_t pid, const std::string &sChildName, Feedback &fb)
{
  if (pid)
  {
    fb.Info(2) << "About to kill process " << pid << ", aka " << sChildName << ".";
    
    static const int name_len = 1024;
    char szHostName[name_len];
    std::stringstream ssMsg;
    ssMsg << "Process: " << sChildName << ", pid: " << pid;
    if (!gethostname(szHostName, name_len))
      ssMsg << ", host: " << szHostName << ".";
    else
      ssMsg << " (Failed to get host name).";

    int nStat;
    pid_t wp = waitpid(pid, &nStat, WNOHANG);
    if (wp == -1)
      return fb.Error(E_WAITPID_FAIL) << "(errno " << errno << ", system error message \"" 
                                      << strerror(errno) << "\"). First attempt. " + ssMsg.str();

    if (wp == 0 || !(WIFEXITED(nStat) || WIFSIGNALED(nStat)))
    {
      fb.Info(2) << "Process " << pid << " is running, about to kill it with SIGINT.";
      kill(pid, SIGINT);
      sleep(2);
      wp = waitpid( pid, &nStat, WNOHANG);
      if (wp == -1)
        return fb.Error(E_WAITPID_FAIL) << "(errno " << errno << ", system error message \"" 
                                      << strerror(errno) << "\"). Second attempt (after SIGINT). " + ssMsg.str();
      
      if (wp == 0 || !(WIFEXITED(nStat) || WIFSIGNALED(nStat)))
      {
        fb.Info(2) << "Process " << pid << " survived SIGINT, sending SIGKILL.";
        kill( pid, SIGKILL );
        sleep(2);
        wp = waitpid( pid, &nStat, 0);
        if (wp == -1)
          return fb.Error(E_WAITPID_FAIL) << "(errno " << errno << ", system error message \"" 
                                      << strerror(errno) << "\"). Third attempt (after SIGKILL). " + ssMsg.str();
        
        if (wp == 0 || !(WIFEXITED(nStat) || WIFSIGNALED(nStat)))
          return fb.Error(E_KILL_FAILED) << ssMsg.str();
        else
          fb.Info(2) << "Process " << pid << " killed with signal SIGKILL.";

      }
      else
        fb.Info(2) << "Process " << pid << " killed with signal SIGINT.";
    }
    else
      fb.Info(2) << "Process " << pid << " has already terminated, no need to kill it.";
  }
  return 0;
}



int
Signal(int nSignal, void (*pHandler) (int), void (**ppOldHandler) (int) /*=0*/)
{
  struct sigaction action, oldAction;
  
  action.sa_handler = pHandler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = SA_RESTART;

  int nRet = sigaction(nSignal, &action, &oldAction);
  if (nRet == 0 && ppOldHandler)
    *ppOldHandler = oldAction.sa_handler;
  return nRet;
}



std::string
SignalToString(int nSignal)
{
  switch(nSignal)
  {
#if defined(SIGHUP)
      case SIGHUP: return std::string("SIGHUP");
#endif
#if defined(SIGINT)
      case SIGINT: return std::string("SIGINT");
#endif
#if defined(SIGQUIT)
      case SIGQUIT: return std::string("SIGQUIT");
#endif
#if defined(SIGILL)
      case SIGILL: return std::string("SIGILL");	
#endif
#if defined(SIGTRAP)
      case SIGTRAP: return std::string("SIGTRAP");	
#endif
#if defined(SIGABRT)
      case SIGABRT: return std::string("SIGABRT/SIGIOT");
#endif
#if defined(SIGBUS)
      case SIGBUS: return std::string("SIGBUS");
#endif
#if defined(SIGFPE)
      case SIGFPE: return std::string("SIGFPE");
#endif
#if defined(SIGKILL)
      case SIGKILL: return std::string("SIGKILL");
#endif
#if defined(SIGUSR1)
      case SIGUSR1: return std::string("SIGUSR1");
#endif
#if defined(SIGSEGV)
      case SIGSEGV: return std::string("SIGSEGV");
#endif
#if defined(SIGUSR2)
      case SIGUSR2: return std::string("SIGUSR2");
#endif
#if defined(SIGPIPE)
      case SIGPIPE: return std::string("SIGPIPE");
#endif
#if defined(SIGALRM)
      case SIGALRM: return std::string("SIGALRM");
#endif
#if defined(SIGTERM)
      case SIGTERM: return std::string("SIGTERM");
#endif
#if defined(SIGSTKFLT)
      case SIGSTKFLT: return std::string("SIGSTKFLT");
#endif
#if defined(SIGCHLD)
      case SIGCHLD: return std::string("SIGCHLD/SIGCLD");
#endif
#if defined(SIGCONT)
      case SIGCONT: return std::string("SIGCONT");
#endif
#if defined(SIGSTOP)
      case SIGSTOP: return std::string("SIGSTOP");
#endif
#if defined(SIGTSTP)
      case SIGTSTP: return std::string("SIGTSTP");
#endif
#if defined(SIGTTIN)
      case SIGTTIN: return std::string("SIGTTIN");
#endif
#if defined(SIGTTOU)
      case SIGTTOU: return std::string("SIGTTOU");
#endif
#if defined(SIGURG)
      case SIGURG: return std::string("SIGURG");	
#endif
#if defined(SIGXCPU)
      case SIGXCPU: return std::string("SIGXCPU");	
#endif
#if defined(SIGXFSZ)
      case SIGXFSZ: return std::string("SIGXFSZ");	
#endif
#if defined(SIGVTALRM)
      case SIGVTALRM: return std::string("SIGVTALRM");
#endif
#if defined(SIGPROF)
      case SIGPROF: return std::string("SIGPROF");		
#endif
#if defined(SIGWINCH)
      case SIGWINCH: return std::string("SIGWINCH");	
#endif
#if defined(SIGIO)
      case SIGIO: return std::string("SIGIO/SIGPOLL/SIGIOT"); 
#endif
#if defined(SIGPWR)
      case SIGPWR: return std::string("SIGPWR");		
#endif
#if defined(SIGSYS)
      case SIGSYS: return std::string("SIGSYS");		
#endif
#if defined(SIGEMT)
      case SIGEMT: return std::string("SIGEMT");
#endif
#if defined(SIGINFO)
      case SIGINFO: return std::string("SIGINFO");
#endif
      default:
        return std::string("Unknown signal number");
  }
}

/********************************************************************
 *   Wildcard string comparison function for null-terminated
 *   strings, adapted from Jack Handy's 'wildcmp' at
 *   http://www.codeproject.com/string/wildcmp.asp.
 *
 *   Changed the name so it contains 'match' rather than 'cmp',
 *   since the return value is interpreted differently than that
 *   of the stdlib string comparison functions.  strcmp returns 0
 *   on a match, whereas this returns nonzero when a match is
 *   found.
 *******************************************************************/
bool
WildcardMatch(const char *w, const char *s) 
{
  const char *cp = 0, *mp = 0;

  while ((*s) && (*w != '*')) {
    if ((*w != *s) && (*w != '?')) {
      return false;
    }
    w++;
    s++;
  }

  while (*s) {
    if (*w == '*') {
      if (!*++w) {
        return true;
      }
      mp = w;
      cp = s+1;
    } else if ((*w == *s) || (*w == '?')) {
      w++;
      s++;
    } else {
      w = mp;
      s = cp++;
    }
  }

  while (*w == '*') {
    w++;
  }
  return *w == 0;
}


/********************************************************************
 *   Search sString beginning at nIndex for sSearch.  Replace the
 *   first match with sReplace.  Return the index of the first
 *   character after the replaced string, or string::npos.  
 *
 *   Note that there is no way to tell if a replace has taken
 *   place or not, since both a match at the end of the string
 *   and no match at all will return npos.
 *******************************************************************/
std::string::size_type
FindReplace(std::string &sString, const std::string &sSearch, const std::string &sReplace, std::string::size_type nIndex /*=0*/)
{
  nIndex = sString.find(sSearch, nIndex);
  if (nIndex == std::string::npos)
    return nIndex;

  sString.replace(nIndex, sSearch.size(), sReplace, 0, sReplace.size());

  if (nIndex + sReplace.size() == sString.size())
    return std::string::npos;
  else
    return nIndex + sReplace.size();
}

void
FindReplaceAll(std::string &sString, const std::string &sSearch, const std::string &sReplace)
{
  std::string::size_type nIndex = 0;
  while (nIndex != std::string::npos)
    nIndex = FindReplace(sString, sSearch, sReplace);
}

