 /********************************************************************
 *   		io_utils.cpp
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

#include <simdist/io_utils.h>

#include <simdist/misc_utils.h>

#include <cstdio>
#include <algorithm>
#include <fstream>
#include <functional>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <iostream>

// FeedbackError E_IO_UTILS_READJOB("Failed to read job data from child process");
DEFINE_FEEDBACK_ERROR(E_IO_UTILS_INIT, "Failed to initialize reader")
DEFINE_FEEDBACK_ERROR(E_IO_UTILS_READJOB, "Failed to read job data from child process");
// FeedbackError E_IO_UTILS_WRITEJOB("Failed to write job data to child process");
DEFINE_FEEDBACK_ERROR(E_IO_UTILS_WRITEJOB, "Failed to write job data to child process");
DEFINE_FEEDBACK_ERROR(E_CHECKPOINT_TEMP, "Failed to create temporary file for checkpoint data");
DEFINE_FEEDBACK_ERROR(E_CHECKPOINT_WRITE, "Failed to write checkpoint data to temporary file");
DEFINE_FEEDBACK_ERROR(E_CHECKPOINT_READ, "Failed to write checkpoint data from file");
DEFINE_FEEDBACK_ERROR(E_CHECKPOINT_RENAME, "Failed to rename temporary file to checkpoint"); 
DEFINE_FEEDBACK_ERROR(E_CHECKPOINT_CLEAR, "Failed to clear checkpoint (i.e. unlink checkpoint file)"); 
using namespace std;

JobReaderWriter::JobReaderWriter()
    : m_fb("JobReaderWriter"), m_mode(none), m_nModeArguments(0)
{
}


JobReaderWriter::JobReaderWriter(EIOMode mode, int nModeArgs /*=0*/)
    : m_fb("JobReaderWriter"), m_mode(mode), m_nModeArguments(nModeArgs)
{
}


JobReaderWriter::JobReaderWriter(const std::string &sIOMode)
    : m_fb("JobReaderWriter"), m_nModeArguments(0)
{
  SetMode(sIOMode);
}


void
JobReaderWriter::SetMode(EIOMode mode, int nModeArgs /*=0*/)
{
  m_mode = mode;
  m_nModeArguments = nModeArgs;
}



/********************************************************************
 *   See header for the available modes.
 *******************************************************************/
int 
JobReaderWriter::SetMode(const std::string &sIOMode)
{
  std::stringstream ssMode(sIOMode);
  std::string sMode;
  ssMode >> sMode;

  if (ssMode.fail())
    return m_fb.Error(E_IO_UTILS_INIT) << "Failed to get job IO mode from argument: " + sIOMode;

  if (sMode == "EOF")
    m_mode = eof;
  else if (sMode == "BYTES")
    m_mode = bytecount;
  else if (sMode == "BIN-EOF")
  {
    m_mode = bin_eof;
    ssMode >> m_nModeArguments;
    if (ssMode.fail())
      m_nModeArguments = default_bin_eof_length;
  }
  else if (sMode == "SIMPLE")
  {
    m_mode = simple;
    ssMode >> m_nModeArguments;
    if (ssMode.fail())
      m_nModeArguments = default_simple_length;
  }
  else
    return m_fb.Error(E_IO_UTILS_INIT) << "Invalid mode: " + sIOMode;
  return 0;
}


std::string 
JobReaderWriter::ModeToString() const
{
  stringstream ssMode;
  switch(m_mode)
  {
      case none:
        return "None";
      case eof:
        return "EOF";
      case bytecount:
        return "BYTES";
      case bin_eof:
        ssMode << "BIN-EOF " << m_nModeArguments;
        return ssMode.str();
      case simple:
        ssMode << "SIMPLE " << m_nModeArguments;
        return ssMode.str();
      default:
        m_fb.Error(E_IO_UTILS_INIT) << "Unknown mode: " + m_mode;
        return "Unknown";
  }
}
/********************************************************************
 *   Custom substring matching function.  In addition to searching
 *   data for containing delimiter, it also accepts part of the
 *   delimiter iff this is found at the very end of the data string.
 *
 *   The return value is an iterator pointing to the position where a
 *   match starts.  If no match is found, dataEnd is returned.
 *
 *   To see if the entire delimiter is found, compare length of
 *   delimiter to distance(returned iterator, dataEnd).
 *******************************************************************/
template<class it1, class it2>
it1
JobReaderWriter::MatchDelim(it1 dataBegin, it1 dataEnd, it2 delimBegin, it2 delimEnd) const
{
  for (; dataBegin != dataEnd; dataBegin++)
  {
    if (*dataBegin == *delimBegin)
    {
      it1 dataCur = dataBegin;
      it2 delimCur = delimBegin;
      while (*dataCur == *delimCur)
      {
        if (++dataCur == dataEnd || ++delimCur == delimEnd)
          return dataBegin;
      }
    }
  }

  return dataEnd;
}


#if defined(__INTEL_COMPILER) && __INTEL_COMPILER < 900
#  pragma message("==================================================")
#  pragma message("NOTE: Compiling " __FILE__ " with a special hack for old ICC compilers.")
#  pragma message("==================================================")
#  define OLD_ICC_RETURN(fb, code, msg) { std::cerr << msg << "\n"; return 1; }
#else
#  define OLD_ICC_RETURN(fb, code, msg) return m_fb.Error(code) << msg;
#endif

/********************************************************************
 *   Optimized reading function: Read as large chunks as possible each
 *   time.  We can read at most as much as the length of the delimiter
 *   at once.
 *******************************************************************/
int
JobReaderWriter::ReadDelimData(std::istream &s, std::string &sData, const std::string &sDelim) const
{
  if (sDelim.empty())
    return m_fb.Error(E_IO_UTILS_READJOB) << ": Empty delimiter.";
  
  const size_t nDelimLen = sDelim.size();
  char buf[nDelimLen + 1];
  s.read(buf, nDelimLen);
  if (s.gcount() != std::streamsize(nDelimLen) || s.eof() || !s.good())
    OLD_ICC_RETURN(fb, E_IO_UTILS_READJOB, ": Reading failed while searching for trailing message delimiter (" << sDelim << ").");

  int nCtr = 0;
  char *mit;
  while((mit = MatchDelim(buf, buf + nDelimLen, sDelim.begin(), sDelim.end())) != buf)
  {
    nCtr++;
    sData.append(buf, mit - buf);
    size_t nSubMatch = buf + nDelimLen - mit;
    if (nSubMatch > 0)
      memmove(buf, mit, nSubMatch);
    s.read(buf + nSubMatch, nDelimLen - nSubMatch);
    if (s.gcount() != std::streamsize(nDelimLen - nSubMatch) || s.eof() || !s.good())
    {
      buf[std::max(nDelimLen, nSubMatch + static_cast<size_t>(s.gcount()))] = 0;
      OLD_ICC_RETURN(fb, E_IO_UTILS_READJOB, ": Reading failed while searching for trailing message delimiter (" 
                     << sDelim << ").  Last block read was: '" << &(buf[0]) << "'.");
    }
  }
  return 0;
}
    

JobReaderWriter::int_type
JobReaderWriter::Read(std::istream &s, std::string &sData) const
{
  sData.clear();
  switch (m_mode)
  {
      case none:
        return m_fb.Error(E_INTERNAL_LOGIC) << ": Reading mode has not been set.";
      case eof:
        {
          std::string sLine, sEOF;
          if (!std::getline(s, sEOF))
          {
            if (s.eof())
              return traits_type::eof();
            return m_fb.Error(E_IO_UTILS_READJOB) << ": Failed to read leading EOF mark.  No more jobs?";
          }
          if (int nRet = ReadDelimData(s, sData, "\n" + sEOF + "\n"))
            return nRet;
        }
        break;
      case bytecount:
        {
          uint32_t nBytes;
          s.read(reinterpret_cast<char*>(&nBytes), sizeof(nBytes));
          if (s.gcount() == 0 && s.eof())
            return traits_type::eof();
          if (s.gcount() != sizeof(nBytes))
            return m_fb.Error(E_IO_UTILS_READJOB) << ": Failed to read byte count.";
          std::vector<char> v(nBytes);
          s.read(&(v[0]), nBytes);
          sData.resize(nBytes);
          std::copy(v.begin(), v.end(), sData.begin());
          if (s.gcount() != static_cast<std::streamsize>(nBytes))
            return m_fb.Error(E_IO_UTILS_READJOB) << ": Failed to read " << nBytes << " bytes of job data.";
        }
        break;
      case bin_eof:
        {
          int nTagLen = m_nModeArguments;
          std::vector<char> tag(nTagLen);
          s.read(&tag[0], nTagLen);
          string sTag;
          copy(tag.begin(), tag.end(), back_inserter(sTag));
          if (s.gcount() == 0 && s.eof())
            return traits_type::eof();
          if (s.gcount() != nTagLen)
            return m_fb.Error(E_IO_UTILS_READJOB) << ": Failed to read " << nTagLen << " bytes of leading binary eof tag.";
          if (int nRet = ReadDelimData(s, sData, sTag)) // Can't do 'std::string(&tag[0])))', for the char vector is not 0-terminated.
            return nRet;
        }
        break;
      case simple:
        {
          int nNumLines = m_nModeArguments;
          if (nNumLines < 1 || nNumLines > 10000)
            return m_fb.Error(E_IO_UTILS_READJOB) << "Won't obey instructions to read " << nNumLines << " lines of result data.";

          std::string sLine;
          for (int nLine = 0; nLine < nNumLines; nLine++)
          {
            if (!std::getline(s, sLine))
            {
              if (nLine == 0 && s.eof())
                return traits_type::eof();
              return m_fb.Error(E_IO_UTILS_READJOB) << ": No more jobs?";
            }
            sData += (nLine ? "\n" : "") + sLine;
          }
        }
        break;
      default:
        return m_fb.Error(E_IO_UTILS_READJOB) << ": Unknown message mode: " << m_mode << ".";
  }
  m_fb.Info(4) << "Read message in " << ModeToString() << " mode: " << sData << ".";
  return 0;
}


/********************************************************************
 *   Increment a sequence interpreted as an unsigned number of
 *   any size.  The elements in the sequence MUST be unsigned for
 *   this algorithm to work (but I'm not quite sure how to
 *   enforce that, while still keeping the algorithm as a
 *   template.  A run-time test such as "assert(*itEnd >= 0)"
 *   gives a warning, saying that this test will always be true
 *   for unsigned numbers).
 *******************************************************************/
template<class T>
void
JobReaderWriter::IncSequence(T itBegin, T itEnd) const
{
  if (itEnd == itBegin)
    return;
  itEnd--;
  ++(*itEnd);
  if (*itEnd == 0)
    IncSequence(itBegin, itEnd);
}


JobReaderWriter::int_type
JobReaderWriter::Write(std::ostream &s, const std::string &sData) const
{
  std::string sMessage;
  switch(m_mode)
  {
      case none:
        return m_fb.Error(E_INTERNAL_LOGIC) << ": Writing mode has not been set.";
      case eof:
        {
          std::stringstream ssEOF;
          ssEOF << "EOF";
          while (sData.find(ssEOF.str()) != std::string::npos)
            ssEOF << "-" << rand();
          sMessage = ssEOF.str() + "\n" + sData + "\n" + ssEOF.str() + "\n";
        }
        break;
      case simple:
        sMessage = sData + "\n";
        break;
      case bytecount:
        {
          uint32_t nBytes = sData.size();
          std::stringstream ssBytes;
          ssBytes.write(reinterpret_cast<char*>(&nBytes), sizeof(nBytes));
          sMessage = ssBytes.str() + sData;
        }
        break;
      case bin_eof:
        {
          int nTagLen = m_nModeArguments;
          std::vector<unsigned char> tag(nTagLen), tagOrig = tag;
          std::string::const_iterator mit;
          while ((mit = MatchDelim(sData.begin(), sData.end(), tag.begin(), tag.end()))
                 != sData.end() && std::distance(mit, sData.end()) >= static_cast<int>(tag.size()))
          {
            IncSequence(tag.begin(), tag.end());
            if (tag == tagOrig)
              return m_fb.Error(E_IO_UTILS_WRITEJOB) << ": Failed to find a " << nTagLen 
                                                     << " byte delimiter not already present in job data! Data was: " << sData;
          }
          std::string sTag;
          sTag.resize(tag.size());
          std::copy(tag.begin(), tag.end(), sTag.begin());
          sMessage = sTag + sData + sTag;
        }
        break;
      default:
        return m_fb.Error(E_IO_UTILS_WRITEJOB) << ": Unknown input mode: " << m_mode << ".";
  }

  m_fb.Info(4) << "Writing job data: " << sMessage;
  s.write(sMessage.c_str(), sMessage.size());
  s.flush();
  
  if (!s.good())
    return m_fb.Error(E_IO_UTILS_WRITEJOB) << ": Output stream failure.";

  return 0;
}


JobReaderWriter::int_type
JobReaderWriter::Read(std::istream &s, char **ppData, size_t &nCount) const
{
  std::string sData;
  if (int nRet = Read(s, sData))
    return nRet;
  nCount = sData.size();
  *ppData = new char[nCount];
  std::copy(sData.begin(), sData.end(), *ppData);
  return 0;
}


JobReaderWriter::int_type
JobReaderWriter::Write(std::ostream &s, const char *pData, size_t nCount) const
{
  std::string sData;
  sData.resize(nCount);
  std::copy(pData, pData + nCount, sData.begin());
  return Write(s, sData);
}



Checkpointer::Checkpointer()
    : m_fb("Checkpointer")
{
}


Checkpointer::Checkpointer(const string sFilename)
    : m_fb("Checkpointer")
{
  SetFilename(sFilename);
}


Checkpointer::~Checkpointer()
{
}


void
Checkpointer::SetFilename(const string &sFilename)
{
  m_sFilename = sFilename;
}


std::string
Checkpointer::GetFilename() const
{
  return m_sFilename;
}


int
Checkpointer::Store(const char *pData, size_t nCount) const
{
  if (m_sFilename.empty())
    return m_fb.Error(E_INTERNAL_LOGIC) << ": No checkpoint filename given.";

  vector<char> sTempName(m_sFilename.size() + 8, 'X');
  copy(m_sFilename.begin(), m_sFilename.end(), sTempName.begin());
  sTempName[m_sFilename.size()] = '.';
  sTempName[sTempName.size() - 1] = 0;
  int nFd = mkstemp(&(sTempName[0]));
  if (nFd == -1)
    return m_fb.Error(E_CHECKPOINT_TEMP) << ". Template was " << &(sTempName[0]);

  ssize_t nWritten = write(nFd, pData, nCount);
  if (nWritten != static_cast<ssize_t>(nCount))
    return m_fb.Error(E_CHECKPOINT_WRITE) << ". Tried to write " << nCount 
                                          << " chars, only " << nWritten
                                          << " written.";

  if (rename(&(sTempName[0]), m_sFilename.c_str()))
    return m_fb.Error(E_CHECKPOINT_RENAME) << ". Temporary name: " << &(sTempName[0])
                                           << ", checkpoint name: " << m_sFilename;

  return 0;
}


int
Checkpointer::Load(char **ppData, size_t *pnCount) const
{
  if (m_sFilename.empty())
    return m_fb.Error(E_INTERNAL_LOGIC) << ": No checkpoint filename given.";

  *pnCount = 0;
  ifstream f(m_sFilename.c_str());
  if (!f)
  {
    m_fb.Info(1, "Unable to open checkpoint file.");
    return 1;
  }

  const streamsize block_size = 1024*10;
  char *pData = 0;
  streamsize nAlloced = 0, nRead = 0;
  while (f.good())
  {
    if (nRead == nAlloced)
    {
      nAlloced += block_size;
      pData = static_cast<char*>(realloc(pData, nAlloced));
      if (!pData)
        return m_fb.Error(E_CHECKPOINT_READ) << " Unable to allocate more memory (asked for " 
                                             << nAlloced << " bytes).";
    }
    f.read(pData + nRead, nAlloced - nRead);
    nRead += f.gcount();
  }

  if (nRead <= 0)
    return m_fb.Error(E_CHECKPOINT_READ) << " Checkpoint file exists, but no data could be read" ;

  *ppData = new char[nRead];
  memcpy(*ppData, pData, nRead);
  free(pData);
  *pnCount = nRead;
  return 0;
}


int
Checkpointer::Store(const std::string &sData) const
{
  return Store(sData.c_str(), sData.size());
}


int
Checkpointer::Load(std::string &sData) const
{
  char *pBuf = 0;
  size_t nCount;
  if (int nRet = Load(&pBuf, &nCount))
    return nRet;

  sData.resize(nCount);
  copy(pBuf, pBuf + nCount, sData.begin());
  return 0;
}


int
Checkpointer::Clear() const
{
  if (!m_sFilename.empty())
  {
    int nRet = unlink(m_sFilename.c_str());
    if (nRet == -1 && errno != ENOENT)
      return m_fb.Error(E_CHECKPOINT_CLEAR);
  }
  return 0; 
}
