/********************************************************************
 *   		slave_mpi.h
 *   Created on Wed May 10 2006 by Boye A. Hoeverstad.
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
 *   Slave message passing and sending using MPI.
 *******************************************************************/

#if !defined(__SLAVE_MPI_H__)
#define __SLAVE_MPI_H__

#if HAVE_CONFIG_H
#include <config.h>
#endif

// Need mpi include before stdio include due to name conflict
// (SEEK_*) in the C++ bindings of some MPI implementations (see
// mpicxx of mpich (2?)).
#if HAVE_MPI2C___MPI___H
#include <mpi2c++/mpi++.h>
#endif
#include <mpi.h>

#include "slave_channel.h"

#include "feedback.h"
#include "syncutils.h"

#include <string>
#include <vector>
#include <map>

// extern FeedbackError E_MPICOMMUNICATOR_SEND;
DECLARE_FEEDBACK_ERROR(E_MPICOMMUNICATOR_SEND)
// extern FeedbackError E_MPICOMMUNICATOR_RECV;
DECLARE_FEEDBACK_ERROR(E_MPICOMMUNICATOR_RECV)

// extern FeedbackError E_MPISENDERRECEIVER_COMM;
DECLARE_FEEDBACK_ERROR(E_MPISENDERRECEIVER_COMM)

// extern FeedbackError E_MPISENDER_SEND;
DECLARE_FEEDBACK_ERROR(E_MPISENDER_SEND)
// extern FeedbackError E_MPISENDER_NOMORESERVERS;
DECLARE_FEEDBACK_ERROR(E_MPISENDER_NOMORESERVERS)
// extern FeedbackError E_MPISENDER_GETRANK;
DECLARE_FEEDBACK_ERROR(E_MPISENDER_GETRANK)
// extern FeedbackError E_MPISENDER_GETSERVER;
DECLARE_FEEDBACK_ERROR(E_MPISENDER_GETSERVER)
// extern FeedbackError E_MPISENDER_SETSERVER;
DECLARE_FEEDBACK_ERROR(E_MPISENDER_SETSERVER)

// extern FeedbackError E_MPIRECEIVER_RECV;
DECLARE_FEEDBACK_ERROR(E_MPIRECEIVER_RECV)

/********************************************************************
 *   Similar to the pvm_stream, we want an MPI stream.  We want
 *   much the same semantics as in the Feedback library: Force
 *   some parameters (here, the message source/destination and
 *   tag), then stream the rest.
 *
 *   Two problems have been discovered with MPI: 1. Although the MPI 2
 *   standard describes thread safety, most current implementations do
 *   not support it.  In particular, simultaneous calls to MPI
 *   functions from different threads is unsafe. 2. At least on
 *   LAM/MPI, both the Receive and the Probe functions use 100% cpu.
 *
 *   We solve the above problems by synchronizing access to the MPI,
 *   and by using an ad hoc polling mechanism when receiving.  
 *
 *   TODO: Edit the pvm_stream to have similar semantics as well, with
 *   a send on destroy for the object returned by the "stream" object?
 *
 *   Design principle: Only the functionality needed at the moment is
 *   implemented.
 *******************************************************************/
class MPICommunicator;
class MPICommunicatorStream
{
  friend class MPICommunicator;
  MPICommunicator &m_comm;
  int m_nRank, m_nTag;
  MPICommunicatorStream (MPICommunicator &c, int nRank, int nTag);

  const MPICommunicatorStream& SingleThreadedReceive(std::string &s) const;
  const MPICommunicatorStream& MultiThreadedReceive(std::string &s) const;
public:
  const MPICommunicatorStream& operator<<(const std::string &s) const;
  const MPICommunicatorStream& operator>>(std::string &s) const;
};


class MPICommunicator : public MPI::Intracomm
{
public:
  typedef enum { singlethreaded, multithreaded } TThreadMode;
private:
  friend class MPICommunicatorStream;
  Feedback m_fb;
  bool m_bGood;
  int m_nLastRank, m_nLastTag;
  TThreadMode m_threadMode;
  void SetLastRecvRankTag(int nRank, int nTag);
  LockableObject& Mutex(); // Return reference to static local variable.
public:

  MPICommunicator(const MPI::Intracomm &data = MPI::COMM_WORLD, TThreadMode tm = multithreaded);
  virtual ~MPICommunicator();
  MPICommunicatorStream operator() (int nRank, int nTag);
  bool good() const;

  void GetLastRecvRankTag(int &nRank, int &nTag) const;
};



class MPISender : public SlaveChannelSender
{
  Feedback m_fb;
  typedef std::map<std::string, int> TRankMap;
  typedef std::vector<std::string> TServerMap;
  TRankMap m_serverRanks;
  TServerMap m_rankServers;
  mutable LockableObject m_rankMtx;
  MPICommunicator m_comm;
  int m_nMasterRank;
  virtual int SendMessage(const std::string &sServer, const std::string &sMessage);
  int GetRank(const std::string &sServer, int &nRank);
public:
  static const int message_tag;

  MPISender(int *argc, char ***argv, std::istream *pInChannel);
  virtual ~MPISender();
  int GetServer(int nRank, std::string &sServer) const;
  int GetNumSlaves() const;
};



class MPIReceiver : public SlaveChannelReceiver
{
  Feedback m_fb;
  const MPISender &m_sender;
  MPICommunicator m_comm;
  virtual int ReceiveMessage(std::string &sServer, std::string &sMessage);
//   virtual int AbortReceiveMessage();
public:
  MPIReceiver(std::ostream *pOutChannel, const MPISender &sender);
  virtual ~MPIReceiver();
};


#endif
