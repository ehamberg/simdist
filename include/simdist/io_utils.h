/********************************************************************
 *   		io_utils.h
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
 *   JobReaderWriter: Read/write job data to/from a stream, using
 *   the input/output mode specified.
 *   
 *   The JobReaderWriter accepts the following message formats:
 *
 *     - eof: Each job is wrapped on both sides with identical
 *     End-of-file (EOF) tags.  Each EOF tag should be terminated
 *     with a newline.  The EOF tag can be anything, as long as
 *     it fits on a single line and is the same on each side of
 *     the job.  Both the EOF tag and the job data may contain
 *     binary data.  Example:
 *
 *       This-is-job-1
 *         job data
 *         more job data
 *       This-is-job-1
 *
 *     - bin_eof: Like eof, but expects a binary end-of-file tag
 *     with a fixed length and no newline termination.
 *     Technically, the only difference between this and the
 *     'eof' mode is that here the length is fixed, whereas in
 *     'eof' mode, a newline terminates each tag.  The length of
 *     the EOF tag CAN be provided as argument, or will default
 *     to default_bin_eof_length.
 *
 *     - simple: Each message spans a constant number of lines.
 *     The number of lines CAN be given as argument, or it will
 *     default to default_simple_length.
 *
 *     - bytecount: Each message starts with a 32 byte unsigned
 *     integer (uint32_t) indicating the size of the message to
 *     come.
 *
 *   NOTE: The EOF/BIN-EOF tags are NOT sent across the network,
 *   so you can NOT write code that is dependent on receiving a
 *   particular tag.
 *
 *   Checkpointer: Save/load checkpoints to/from file.  Save
 *   operation should be 'atomic', in the sense that the whole
 *   block of data should be written to file, if anything.
 *
 *******************************************************************/

#if !defined(__IO_UTILS_H__)
#define __IO_UTILS_H__

#include "feedback.h"

DECLARE_FEEDBACK_ERROR(E_IO_UTILS_READJOB)
DECLARE_FEEDBACK_ERROR(E_IO_UTILS_WRITEJOB)
DECLARE_FEEDBACK_ERROR(E_IO_UTILS_INIT)

DECLARE_FEEDBACK_ERROR(E_CHECKPOINT_TEMP)
DECLARE_FEEDBACK_ERROR(E_CHECKPOINT_WRITE)
DECLARE_FEEDBACK_ERROR(E_CHECKPOINT_RENAME)
DECLARE_FEEDBACK_ERROR(E_CHECKPOINT_READ)
DECLARE_FEEDBACK_ERROR(E_CHECKPOINT_CLEAR)

class JobReaderWriter
{
public:
  typedef enum { none, eof, bin_eof, simple, bytecount } EIOMode;
  enum { default_simple_length = 1, default_bin_eof_length = 4 };
  typedef std::istream::traits_type traits_type;
  typedef traits_type::int_type int_type;
private:
  Feedback m_fb;
  EIOMode m_mode;
  int m_nModeArguments;

  template<class it1, class it2>
  it1 MatchDelim(it1 dataBegin, it1 dataEnd, it2 delimBegin, it2 delimEnd) const;
  int ReadDelimData(std::istream &s, std::string &sData, const std::string &sDelim) const;
  template<class T>
  void IncSequence(T itBegin, T itEnd) const;
public:
  JobReaderWriter();
  JobReaderWriter(const std::string &sIOMode);
  JobReaderWriter(EIOMode mode, int nModeArgs = 0);
  
  int SetMode(const std::string &sIOMode);
  void SetMode(EIOMode mode, int nModeArgs = 0);
  std::string ModeToString() const;

  int_type Read(std::istream &s, std::string &sData) const;
  int_type Write(std::ostream &s, const std::string &sData) const;
  int_type Read(std::istream &s, char **ppData, size_t &nCount) const;
  int_type Write(std::ostream &s, const char *pData, size_t nCount) const;
};



class Checkpointer
{
  Feedback m_fb;
  std::string m_sFilename;
//   std::string m_sTemplate;
public:
  Checkpointer();
  ~Checkpointer();
  Checkpointer(const std::string sFilename);

  void SetFilename(const std::string &sFileName);
  std::string GetFilename() const;

  int Store(const char *pData, size_t nCount) const;
  int Store(const std::string &sData) const;

  int Load(char **ppData, size_t *pnCount) const;
  int Load(std::string &sData) const;

  int Clear() const;
};


#endif
