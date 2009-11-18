/********************************************************************
 *   		slave_mpi.cpp
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
 *   See header file for description.
 *******************************************************************/

#include <simdist/slave_mpi.h>
#include <simdist/messages.h>

#include <time.h>
#include <cstdlib>

// FeedbackError E_MPICOMMUNICATOR_SEND("Failed to send MPI message.");
DEFINE_FEEDBACK_ERROR(E_MPICOMMUNICATOR_SEND, "Failed to send MPI message.")
// FeedbackError E_MPICOMMUNICATOR_RECV("Failed to receive MPI message.");
DEFINE_FEEDBACK_ERROR(E_MPICOMMUNICATOR_RECV, "Failed to receive MPI message.")
// FeedbackError E_MPISENDERRECEIVER_COMM("Error in the MPI communication object");
DEFINE_FEEDBACK_ERROR(E_MPISENDERRECEIVER_COMM, "Error in the MPI communication object")
// FeedbackError E_MPISENDER_SEND("Failed to send message using MPI");
DEFINE_FEEDBACK_ERROR(E_MPISENDER_SEND, "Failed to send message using MPI")
// FeedbackError E_MPISENDER_NOMORESERVERS("Failed to allocate rank (server ID) to server name: No more slaves available");
DEFINE_FEEDBACK_ERROR(E_MPISENDER_NOMORESERVERS, "Failed to allocate rank (server ID) to server name: No more slaves available")
// FeedbackError E_MPISENDER_GETRANK("Failed to get rank of server");
DEFINE_FEEDBACK_ERROR(E_MPISENDER_GETRANK, "Failed to get rank of server")
// FeedbackError E_MPISENDER_GETSERVER("Failed to get server corresponding to rank");
DEFINE_FEEDBACK_ERROR(E_MPISENDER_GETSERVER, "Failed to get server corresponding to rank")
// FeedbackError E_MPISENDER_SETSERVER("Failed to assign server ID to rank");
DEFINE_FEEDBACK_ERROR(E_MPISENDER_SETSERVER, "Failed to assign server ID to rank")
// FeedbackError E_MPIRECEIVER_RECV("Failed to receive message using MPI");
DEFINE_FEEDBACK_ERROR(E_MPIRECEIVER_RECV, "Failed to receive message using MPI")


MPICommunicator::MPICommunicator(const MPI::Intracomm &data, TThreadMode tm /*=multithreaded*/)
    : MPI::Intracomm(data), m_fb("MPICommunicator"), m_bGood(true), m_threadMode(tm)
{
}


MPICommunicator::~MPICommunicator()
{
}


MPICommunicatorStream
MPICommunicator::operator() (int nRank, int nTag)
{
  return MPICommunicatorStream(*this, nRank, nTag);
}
 

bool 
MPICommunicator::good() const
{
      // Note: MPICommunicatorStream will modify the below
      // variable directly.
  return m_bGood;
}


LockableObject&
MPICommunicator::Mutex()
{
  static LockableObject mutex("MPI Mutex");
  return mutex;
}


void
MPICommunicator:: SetLastRecvRankTag(int nRank, int nTag)
{
  m_nLastRank = nRank;
  m_nLastTag = nTag;
}


void 
MPICommunicator::GetLastRecvRankTag(int &nRank, int &nTag) const
{
  nRank = m_nLastRank;
  nTag = m_nLastTag;
}


MPICommunicatorStream::MPICommunicatorStream (MPICommunicator &c, int nRank, int nTag)
    : m_comm(c), m_nRank(nRank), m_nTag(nTag)
{
}


const MPICommunicatorStream& 
MPICommunicatorStream::operator<<(const std::string &s) const
{
  AutoMutex mtx;
  try {
    m_comm.m_fb.Info(4) << "Sending the following MPI-message to rank " << m_nRank << ":\n" << s;
    if (m_comm.Mutex().AcquireMutex(mtx))
      m_comm.m_fb.Error(E_MPICOMMUNICATOR_SEND);
    else
      m_comm.Send(s.c_str(), static_cast<int>(s.size()), MPI::CHAR, m_nRank, m_nTag);
  } catch (MPI::Exception e) {
    if (m_comm.good())
    {
      m_comm.m_bGood = false;
      m_comm.m_fb.Error(E_MPICOMMUNICATOR_SEND) 
        << ". Error code: " << e.Get_error_code() 
        << ". Error class: " << e.Get_error_class()
        << ". Description: " << e.Get_error_string() << ".";
    }
  }
  return *this;
}



/********************************************************************
 *   This receive function is made for single-threaded users or
 *   users who provide separate synchronization of their MPI
 *   calls.  It blocks the calling thread on a receive, and will
 *   therefore return as soon as a message arrives.
 *
 *   This function recursively calls receive if the initial
 *   string is too short, rather than calling probe first and
 *   then initializing the string to the necessary length.  The
 *   reason for this is that the LAM implementation of probe uses
 *   up 100% cpu.
 *
 *   NOTE: The recursion code may or may not work. LAM goes down if
 *   the receive buffer is too small.  Perhaps an MPI error handler
 *   setting?
 *******************************************************************/
const MPICommunicatorStream& 
MPICommunicatorStream::SingleThreadedReceive(std::string &s) const
{
  if (s.empty())
    s.resize(10000); 

  MPI::Status status;
  try {
    std::vector<char> v(s.size());
    m_comm.Recv(&(v[0]), static_cast<int>(v.size()), MPI::CHAR, m_nRank, m_nTag, status);
    s.assign(reinterpret_cast<const char*>(&v[0]), status.Get_count(MPI::CHAR)); // Use assign in case of non-text message.
    m_comm.SetLastRecvRankTag(status.Get_source(), status.Get_tag());

  } catch (MPI::Exception e) {
        // MPI::ERR_COUNT can't be found on LAM (although ERR_BUFFER
        // etc is found), so we use the C constant and hope that the
        // C++ variant has the same value.
    if (e.Get_error_code() == MPI_ERR_COUNT) 
    {
      int nSize = status.Get_count(MPI::CHAR);
      if (static_cast<int>(s.size()) < nSize)
      {
        s.resize(nSize);
        return SingleThreadedReceive(s);
      }
      m_comm.m_fb.Warning() << "Error code is count, but the buffer is large enough to contain "
                       << "the message! Buffer size: " << s.size() << ". Message size: " 
                       << nSize << ". Detailed error description follows.";
    }
    throw;
  }
  return *this;
}



/********************************************************************
 *   This receive function is made for use with multithreaded
 *   access to the MPI library.  Instead of blocking on a
 *   receive, it will repeatedly perform a protected probe,
 *   followed by a sleep, until a message arrives.
 *******************************************************************/
const MPICommunicatorStream& 
MPICommunicatorStream::MultiThreadedReceive(std::string &s) const
{

  const struct timespec sleepTime = { 0, 50*1000 }; // seconds, nanoseconds

  MPI::Status status;

  bool bMessage = false;
  AutoMutex mtx;
  while (!bMessage)
  {
    m_comm.Mutex().AcquireMutex(mtx);
    if (!(bMessage = m_comm.Iprobe(m_nRank, m_nTag, status)))
    {
      mtx.Unlock();
//       m_comm.m_fb.Info(3, "No incoming messages, sleeping...");
//      sleep(1);
      nanosleep(&sleepTime, 0);
    }
  }

  m_comm.SetLastRecvRankTag(status.Get_source(), status.Get_tag());
      // Add terminating 0
  std::vector<char> v(status.Get_count(MPI::CHAR));
  m_comm.Recv(&(v[0]), static_cast<int>(v.size()), MPI::CHAR, m_nRank, m_nTag);
  s.assign(reinterpret_cast<const char*>(&v[0]), v.size()); // Use assign in case of non-text message.
  return *this;
}


const MPICommunicatorStream& 
MPICommunicatorStream::operator>>(std::string &s) const
{
  s.clear(); // Must clear for recursion to work correctly in single threaded mode.

  try {
    if (m_comm.m_threadMode != MPICommunicator::singlethreaded
        && m_comm.m_threadMode != MPICommunicator::multithreaded)
    {
      m_comm.m_fb.Error(E_INTERNAL_LOGIC) << "Unknown threading mode: " << m_comm.m_threadMode << "! Exiting.";
      exit(1);
    }

    m_comm.m_fb.Info(4) << "Receiving an MPI-message in " << (m_comm.m_threadMode == MPICommunicator::singlethreaded ? "single" : "multi")
                        << "threaded mode from rank " << m_nRank 
                        << (m_nRank == MPI::ANY_SOURCE ? " (MPI::ANY_SOURCE)." : ".");
    if (m_comm.m_threadMode == MPICommunicator::singlethreaded)
      return SingleThreadedReceive(s);
    else
      return MultiThreadedReceive(s);

  } catch (MPI::Exception e) {
    if (m_comm.good())
    {
      m_comm.m_bGood = false;
      m_comm.m_fb.Error(E_MPICOMMUNICATOR_RECV) 
        << ". Error code: " << e.Get_error_code() 
        << ". Error class: " << e.Get_error_class()
        << ". Description: " << e.Get_error_string() << ".";
    }
  }
  return *this;
}


const int 
MPISender::message_tag = 1;


MPISender::MPISender(int *argc, char ***argv, std::istream *pInChannel)
    : SlaveChannelSender(pInChannel), m_fb("MPISender"), m_rankMtx("MPI rank mutex")
{
//   m_fb.Info(1, "MPISender ctor calls MPI_Init. Make singleton with static initialization?");
//   MPI_Init(argc, argv);
  m_nMasterRank = m_comm.Get_rank();
}


MPISender::~MPISender()
{
//   MPI_Finalize();
}


int
MPISender::GetNumSlaves() const
{
  return m_comm.Get_size() - 1;
}


int
MPISender::SendMessage(const std::string &sServer, const std::string &sMessage)
{
  if (!m_comm.good())
    return m_fb.Error(E_MPISENDERRECEIVER_COMM);

  int nRank;
  if (GetRank(sServer, nRank))
  {
    m_fb.Warning() << "Unable to send message to server " << sServer 
                   << ": Could not get rank (ID) for that server. Message discarded.";
    return 0;
  }

  m_fb.Info(4) << "About to send message to server " << sServer << ", with rank " << nRank << ".";

  m_comm(nRank, message_tag) << sMessage;
  if (!m_comm.good())
    return m_fb.Error(E_MPISENDER_SEND);
  return 0;
}


/********************************************************************
 *   Return the server id corresponding to rank nRank.
 *
 *   This only works if communication is always initiated by the
 *   master.
 *******************************************************************/
int
MPISender::GetServer(int nRank, std::string &sServer) const
{
  AutoMutex mtx;
  if (const_cast<LockableObject*>(&m_rankMtx)->AcquireMutex(mtx))
    return m_fb.Error(E_MPISENDER_GETSERVER) << " " << nRank << ".";

  if (nRank == m_nMasterRank)
  {
    sServer = MessagePasser::master_node_id;
    return 0;
  }

  if (nRank >= static_cast<int>(m_rankServers.size()))
    return m_fb.Error(E_MPISENDER_GETSERVER) << " " << nRank
                                             << ". No such server!";
  
  sServer = m_rankServers[nRank];
  return 0;
}



/********************************************************************
 *   Return the rank of server sServer.  server names are
 *   assigned ranks sequentially as long as there are available
 *   slaves.  The number of slaves is determined as communicator
 *   size - 1 (the master).  
 *******************************************************************/
int
MPISender::GetRank(const std::string &sServer, int &nRank)
{
  AutoMutex mtx;
  if (m_rankMtx.AcquireMutex(mtx))
    return m_fb.Error(E_MPISENDER_GETRANK) << " " << sServer << ".";

      // Return our own rank if the master node is requested.  Do this
      // independently of the reservation of a slot for master below,
      // so that things work even if the master node has rank > 0 and
      // the servers with lower rank have not been used yet.  See if
      // server already has been assigned a slave process.
  if (sServer == MessagePasser::master_node_id)
  {
    nRank = m_nMasterRank;
    return 0;
  }

  TRankMap::const_iterator rit = m_serverRanks.find(sServer);
  if (rit != m_serverRanks.end())
  {
    nRank = rit->second;
    return 0;
  }

      // Assign slave process to server.  First check if the last
      // available rank is the master.
  if (static_cast<int>(m_rankServers.size()) == m_nMasterRank)
  {
    m_rankServers.push_back(MessagePasser::master_node_id);
  }

      // Check for free slave processes.
  if (static_cast<int>(m_rankServers.size()) == m_comm.Get_size())
    return m_fb.Error(E_MPISENDER_NOMORESERVERS);

      // Assign rank/slave process to server ID.
  nRank = m_rankServers.size();
  m_rankServers.push_back(sServer);
  m_serverRanks[sServer] = nRank;

  return 0;
}


MPIReceiver::MPIReceiver(std::ostream *pOutChannel, const MPISender &sender)
    : SlaveChannelReceiver(pOutChannel), m_fb("MPIReceiver"), m_sender(sender)
{
}


MPIReceiver::~MPIReceiver()
{
}


int 
MPIReceiver::ReceiveMessage(std::string &sServer, std::string &sMessage)
{
  if (!m_comm.good())
    return m_fb.Error(E_MPISENDERRECEIVER_COMM);

  m_comm(MPI::ANY_SOURCE, MPI::ANY_TAG) >> sMessage;
  if (!m_comm.good())
    return m_fb.Error(E_MPIRECEIVER_RECV);

  int nRank, nTag;
  m_comm.GetLastRecvRankTag(nRank, nTag);
  if (m_sender.GetServer(nRank, sServer))
    return m_fb.Error(E_MPIRECEIVER_RECV) << ": Received a message from unknown sender.";

  return 0;
}


// int 
// MPIReceiver::AbortReceiveMessage()
// {
//   pthread_cancel(m_threadId);
//   return 0;
// }
