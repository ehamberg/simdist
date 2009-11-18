/********************************************************************
 *   		logio.cpp
 *   Created on Wed Jun 21 2006 by Boye A. Hoeverstad.
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
 *   Log any of standard input, output and/or error of a process to
 *   file.
 *******************************************************************/

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <errno.h>
#include <getopt.h>
#include <iterator>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <simdist/misc_utils.h>
#include <sys/wait.h>
#include <cassert>
#include <memory>
#include <cstring>
#include <cstdlib>

using namespace std;

static bool bVerbose = false;
static bool bVeryVerbose = false;
static string sProgramName;
static bool bBinary = false;
static bool bZip = false;
static bool bDebug = false;

class Log {
  bool m_bLog;
  string m_sFilename;
  string m_sDirection;
  fdostream *m_pfLog;
  pid_t m_nZipPid;
  bool m_bLogOwner;
public:
  Log(string sDir)
      : m_bLog(false), m_sDirection(sDir)
      , m_pfLog(0), m_nZipPid(0), m_bLogOwner(false)
  {}

  ~Log()
  {
    if (m_bLogOwner && m_pfLog)
      delete m_pfLog;
  }

  bool
  IsLogging() const
  {
    return m_bLog;
  }

  void
  SetZipPid(pid_t nPid)
  {
    m_nZipPid = nPid;
  }

  pid_t
  ZipPid() const
  {
    return m_nZipPid;
  }

  void
  SetFilename(const char *szFname)
  {
    m_bLog = true;
    m_sFilename = szFname;
  }

  const std::string&
  Filename() const
  {
    return m_sFilename;
  }

  const std::string&
  Direction() const
  {
    return m_sDirection;
  }

  void
  LogToSameStream(const Log &rhs)
  {
    m_pfLog = rhs.m_pfLog;
    m_bLogOwner = false;
  }

  void
  SetLogStream(fdostream *ps)
  {
    m_pfLog = ps;
    m_bLogOwner = true;
  }

  fdostream*
  LogStream()
  {
    return m_pfLog;
  }

  void
  Close()
  {
    if (IsLogging() && m_bLogOwner)
    {
      LogStream()->close();
      if (bZip)
      {
            // Give gzip time to finish up
        int nStat;
        if (bVerbose)
          cerr << "Waiting for gzip (pid " << ZipPid() << ") to complete compressing child "
               << Direction() << " log.\n";
        waitpid(ZipPid(), &nStat, 0);
        if (bVerbose)
          cerr << "gzip completed.\n";
      }
    }
  }
};

static Log logs[3] = { Log("input"), Log("output"), Log("error") };

int
parse_args(int argc, char *argv[])
{
  struct option opts[] = {
    {"input-log"    , required_argument, 0, 'i'},
    {"output-log"   , required_argument, 0, 'o'},
    {"error-log"    , required_argument, 0, 'e'},
    {"binary"       , no_argument, 0, 'b'},
    {"verbose"      , no_argument, 0, 'v'},
    {"very-verbose" , no_argument, 0, 'V'},
    {"zip"          , no_argument, 0, 'z'},
    {"debug"        , no_argument, 0, 'd'},
    { 0 }};
 
  int ch;
      // Add a + to the opstring to avoid permuting the option string.
      // This is necessary when logging processes with options.
  while ((ch = getopt_long(argc, argv, "+be:i:o:vVz", opts, 0)) != -1)
  {
    switch (ch)
    {
        case 'b':
          bBinary = true;
          break;
        case 'd':
          bDebug = true;
          break;
        case 'e':
          logs[2].SetFilename(optarg);
          break;
        case 'i':
          logs[0].SetFilename(optarg);
          break;
        case 'o':
          logs[1].SetFilename(optarg);
          break;
        case 'v':
          bVerbose = true;
          break;
        case 'V':
          bVerbose = true;
          bVeryVerbose = true;
          break;
        case 'z':
          bZip = true;
          break;
        default:
          cerr << "Unregonized option: " << ch << ".\n";
          return 1;
    }
  }
  return 0;
}


int 
OpenLogs()
{
  for (int nLogIdx = 0; nLogIdx < 3; nLogIdx++)
  {
    Log &log = logs[nLogIdx];
    if (!log.IsLogging())
      continue;

        // Check to see if multiple directions should be logged
        // to the same stream.
    bool bSame = false;
    for (int nOtherIdx = 0; nOtherIdx < nLogIdx; nOtherIdx++)
    {
      Log &other = logs[nOtherIdx];
      if (log.Filename() == other.Filename())
      {
        log.LogToSameStream(other);
        bSame = true;
        if (bVerbose)
          cerr << sProgramName << ": Logging " << log.Direction() 
               << " data to the same file as " << other.Direction() << " data.\n";
        break;
      }
    }
    if (bSame)
      continue;

        // Create log file, which will be either written to
        // directly, or written to by gzip, to whom we will pipe
        // the logged output.
    int nFdOut = open(log.Filename().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (nFdOut == -1)
    {
      cerr << "Failed to open " << log.Direction() << " log file " 
           << log.Filename() << ". System error message: " << strerror(errno) << ".";
      return 1;
    }

    if (bZip)
    {
      int fd[2];
      int nRet = pipe(fd);
      if (nRet == -1)
      {
        cerr << "Failed to create zip pipe.\n";
        exit(1);
      }
      pid_t nZipPid = fork();
      if (!nZipPid)
      {
            // Create output file for gzip and connect in child.
        close(fd[1]);
        dup2(fd[0], 0);
        dup2(nFdOut, 1);
        if (execlp("gzip", "-c", static_cast<char*>(0)))
        {
          cerr << "Failed to load gzip for compressing logged data. System error message: "
               << strerror(errno) << ".";
          return 1;
        }
      }
      else
      {
            // Replace log fd with write end of gzip pipe in parent.
        close(fd[0]);
        log.SetZipPid(nZipPid);
        log.SetLogStream(new fdostream(fd[1]));
      }
    }
    else
      log.SetLogStream(new fdostream(nFdOut));

    if (bVerbose)
      cerr << sProgramName << ": Logging " << log.Direction() << " data to " << log.Filename() << ".\n";
  }
  return 0;
}


ssize_t
Shuffle(int nFdIn, int nFdOut, Log &log)
{
  static const size_t nReadMax = 1024;
  char szBuf[nReadMax];
  ssize_t nNumRead = read(nFdIn, szBuf, nReadMax);
  if (nNumRead > 0)
  {
    if (log.IsLogging())
    {
      if (!log.LogStream()->write(szBuf, nNumRead))
      {
        cerr << "Failed to write " << nNumRead << " bytes to " << log.Direction()
             << " log file " << log.Filename() << ".\n";
        return ssize_t(-1);
      }
      log.LogStream()->flush();
    }

    if (write(nFdOut, szBuf, nNumRead) != nNumRead)
    {
      cerr << "Failed to write " << nNumRead << " bytes to " << log.Direction()
           << ".\n System error message: " << strerror(errno) << ".\n";
      return ssize_t(-1);
    }
  }

  if (nNumRead == -1)
    cerr << "Failed to read from descriptor " << nFdIn
         << " (the data read should have been shuffled on to " << log.Direction()
         << ".\n System error message: " << strerror(errno) << ".\n";

  return nNumRead;
}



class Poller
{

  typedef struct TPipeVar
  {
    Log *pLog;
    int nFdIn;
    string sDescIn;
    int nFdOut;
    string sDescOut;
  } TPipe;

  vector<TPipe> m_pipes;

  vector<pollfd> CreatePollFds() const
  {
    vector<pollfd> pfds;
    for (size_t nIdx = 0; nIdx < m_pipes.size(); nIdx++)
    {
      pollfd pfd;
      pfd.fd = m_pipes[nIdx].nFdIn;
      pfd.events = POLLIN | POLLERR | POLLHUP;
      pfd.revents = 0;
      pfds.push_back(pfd);

      pfd.fd = m_pipes[nIdx].nFdOut;
      pfd.events = POLLERR | POLLHUP;
      pfd.revents = 0;
      pfds.push_back(pfd);
    }
    return pfds;
  }


public:
  void
  AddListenedPipe(Log *pLog, int nFdIn, string sDescIn, int nFdOut, string sDescOut)
  {
    TPipe p;
    p.pLog = pLog;
    p.nFdIn = nFdIn;
    p.sDescIn = sDescIn;
    p.nFdOut = nFdOut;
    p.sDescOut = sDescOut;
    m_pipes.push_back(p);
  }

  bool
  HavePipes() const
  {
    return !m_pipes.empty();
  }

  int
  Poll()
  {
    vector<pollfd> pollFds = CreatePollFds();

    int nPollRet = poll(&(pollFds[0]), pollFds.size(), -1);
    if (nPollRet <= 0)
      return 1;

        // First check for available data, so all data is logged
        // even if the child process completes or stdin is
        // closed.
    for (size_t nPollIdx = 0; nPollIdx < pollFds.size(); nPollIdx++)
    {
      struct pollfd &pfd = pollFds[nPollIdx];

      if (pfd.revents == 0)
        continue;

          // Find the corresponding pipe in our internal
          // representation.
      typedef enum { input, output } EPollDir;
      EPollDir pollDir = input;
      vector<TPipe>::iterator itPipe = m_pipes.begin();
      for (; itPipe != m_pipes.end(); itPipe++)
      {
        if (itPipe->nFdIn == pfd.fd)
          break;
        if (itPipe->nFdOut == pfd.fd)
        {
          pollDir = output;
          break;
        }
      }
      if (itPipe == m_pipes.end())
      {
        cerr << "Internal error! Could not find polled fd in internal representation!\n";
        return 1;
      }

          // Check for available input data
      if (pfd.revents & POLLIN)
      {
        if (pollDir != input)
        {
          cerr << "Internal error: poll returned input ready on output pipe..";
          return 1;
        }
        if (bVeryVerbose)
          cerr << "Reading possible from " << itPipe->sDescIn << ".  Passing and logging data.\n";
        ssize_t nNumMoved = Shuffle(itPipe->nFdIn, itPipe->nFdOut, *(itPipe->pLog));
        if (nNumMoved == 0) // indicates end-of-file on a regular file
        {
          if (bVerbose)
            cerr << "Reached end-of-file on " << itPipe->sDescIn << ".\n";
          if (itPipe->nFdOut != 2)
            close(itPipe->nFdOut);
          m_pipes.erase(itPipe);
        }
        return (nNumMoved == -1 ? 1 : 0);
      }

          // Check for errors / hangups (only check input, assume
          // output will follow)
      if (pfd.revents & POLLERR || pfd.revents & POLLHUP)
      {
        string sDescThis = (pollDir == input ? itPipe->sDescIn : itPipe->sDescOut);
        string sDescOther = (pollDir == input ? itPipe->sDescOut : itPipe->sDescIn);
        int nFdOther = (pollDir == input ? itPipe->nFdOut : itPipe->nFdIn);
        if (bVerbose)
          cerr << sDescThis << " closed, closing " << sDescOther << ".\n";
            // Don't close our own stdin/-out/-err, as this may cause error.
        if (nFdOther > 2)
          close(nFdOther);
            // Erase the closed pipe, so we don't poll it again.
        m_pipes.erase(itPipe);
      }
          // Only process one returned event at a time.
      return 0;
    }
    cerr << "ERROR! This code should never have been executed...\n";
    return 1;
  }

};
    

  
int
main(int argc, char *argv[])
{
  sProgramName = argv[0];
  if (parse_args(argc, argv) || argc - optind < 1)
  {
    cerr << "Usage: " << sProgramName << " [options] process [process-specific options]\n"
         << "Log input and/or output of process to file.\n"
         << "\nOptions:\n"
         << "-i, --input-log filename\tLog standard input to process to given file.\n"
         << "-o, --output-log filename\tLog standard output from process to given file.\n"
         << "-e, --error-log filename\tLog standard error from process to given file.\n"
         << "-z, --zip\tCompress logged data with gzip.\n"
         << "-v, --verbose \tPrint some diagnostics.\n"
         << "-V, --very-verbose \tPrint more diagnostics.\n"
         << "-d, --debug \tPause after launch and print process ID of parent and child processes, so a debugger may be attached.\n";
    return 1;
  }

  if (bDebug)
  {
    cerr << "Pid is " << getpid() << " .\n";
    sleep(5);
  }

  vector<string> argVec;
  int nArgIdx = optind;
  while (nArgIdx < argc)
    argVec.push_back(argv[nArgIdx++]);

  if (argVec.empty())
  {
    cerr << "Failed to find process name and optional process-specific arguments on command line.\n";
    return 1;
  }

  if (OpenLogs())
  {
    cerr << "Failed to open logs.\n";
    return 1;
  }
  
  if (bVerbose)
  {
    cerr << sProgramName << ": Loading the logged process as follows:\n\t";
    copy(argVec.begin(), argVec.end(), ostream_iterator<string>(cerr, " "));
    cerr << "\n";
  }

  int nfd[3];

  if (ConnectProcess(argVec, 0, 0, 0, 0, nfd))
  {
    cerr << "Failed to connect to the logged process.\n";
    return 1;
  }

  Poller p;
  p.AddListenedPipe(&(logs[0]), 0, "parent stdin", nfd[0], "child stdin");
  p.AddListenedPipe(&(logs[1]), nfd[1], "child stdout", 1, "parent stdout");
  p.AddListenedPipe(&(logs[2]), nfd[2], "child stderr", 2, "parent stderr");

  while (p.HavePipes())
    if (int nRet = p.Poll())
      return nRet;
  
  if (bVerbose)
    cerr << "Closing logs...\n";

  for (int nLog = 0; nLog < 3; nLog++)
    logs[nLog].Close();

  if (bVerbose)
    cerr << "Logs closed.\n";

  return 0;
}

