/********************************************************************
 *   		slave_pvm.cpp
 *   Created on Fri Apr 07 2006 by Boye A. Hoeverstad.
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

#include <simdist/slave_pvm.h>
#include <pvm3.h>
#include <iostream>
#include <vector>

// FeedbackError E_PVMSTREAM_INITSEND("pvm_initsend failed");
DEFINE_FEEDBACK_ERROR(E_PVMSTREAM_INITSEND, "pvm_initsend failed")
// FeedbackError E_PVMSENDER_SEND("Failed to send a message");
DEFINE_FEEDBACK_ERROR(E_PVMSENDER_SEND, "Failed to send a message")
// FeedbackError E_PVMSENDER_RECV("Failed to receive a message");
DEFINE_FEEDBACK_ERROR(E_PVMSENDER_RECV, "Failed to receive a message")
FEEDBACK_ERROR(E_PVMSENDER_PACK, "Failed to pack/create pvm message");
// FeedbackError E_PVMSENDER_PVMSEND("pvm_send failed");
DEFINE_FEEDBACK_ERROR(E_PVMSENDER_PVMSEND, "pvm_send failed")
// FeedbackError E_PVMSENDER_SPAWN("Failed to spawn a new pvm task");
DEFINE_FEEDBACK_ERROR(E_PVMSENDER_SPAWN, "Failed to spawn a new pvm task")
// FeedbackError E_PVMSENDER_NORECVTID("Missing Pvm Task ID for client-side receiving process");
DEFINE_FEEDBACK_ERROR(E_PVMSENDER_NORECVTID, "Missing Pvm Task ID for client-side receiving process")
FEEDBACK_ERROR(E_PVMRECEIVER_UNPACK, "Failed to unpack message. Data not properly formatted?");
// FeedbackError E_PVMRECEIVER_RECV("pvm_recv failed");
DEFINE_FEEDBACK_ERROR(E_PVMRECEIVER_RECV, "pvm_recv failed")
// FeedbackError E_PVMRECEIVER_PVMSEND("pvm_send failed");
DEFINE_FEEDBACK_ERROR(E_PVMRECEIVER_PVMSEND, "pvm_send failed")


/********************************************************************
 *   Construct a pvm stream.  This normally doesn't amount to much.
 *   If the nInitSend argument is true, the constructor will call
 *   pvm_initsend to initiate a sending buffer, and set state
 *   according to how it went.  This is false by default, so multiple
 *   pvm_stream objects can work on the same buffer.
 *******************************************************************/
pvm_stream::pvm_stream(bool bInitSend /*=false*/)
    : m_fb("pvm_stream"), m_state(std::ios_base::goodbit)
{
  if (bInitSend)
    initsend();
}

pvm_stream::pvm_stream(const pvm_stream &copy)
    : m_fb("pvm_stream")
{
  m_state = copy.m_state;
}


pvm_stream& 
pvm_stream::operator=(const pvm_stream &copy)
{
  m_state = copy.m_state;
  return *this;
}

pvm_stream& 
pvm_stream::initsend()
{
  if (good())
  {
    if (pvm_initsend(PvmDataDefault) < 0)
    {
      m_fb.Error(E_PVMSTREAM_INITSEND);
      state(std::ios_base::failbit);
    }
  }
  return *this;
}     



std::ios_base::iostate 
pvm_stream::state(std::ios_base::iostate new_state)
{
  return m_state = new_state;
}

std::ios_base::iostate 
pvm_stream::state() const
{
  return m_state; 
}



/********************************************************************
 *   State of stream is good iff goodbit is set, and no other bits are
 *   set.
 *******************************************************************/
bool 
pvm_stream::good() const
{
      //!!-TODO: Find out how this should have been done.  goodbit is 0,
      //!!-so the original test failed miserably.
      // return (m_state & std::ios_base::goodbit) && !(m_state & ~std::ios_base::goodbit);
  return !(m_state & ~std::ios_base::goodbit);
}


bool 
pvm_stream::operator()() const
{
  return good();
}


pvm_stream& 
pvm_stream::operator<<(const int n)
{
  if (good())
  {
    int nTmp = n; // pvm lacks const spec.
    if (pvm_pkint(&nTmp, 1, 1) < 0)
      state(std::ios_base::failbit);
  }
  return *this;
}


pvm_stream& 
pvm_stream::operator<<(const std::string &s)
{
  if (good())
  {
    int nLen = s.size();
    if (pvm_pkint(&nLen, 1, 1) < 0
        || pvm_pkstr(const_cast<char*>(s.c_str())) < 0)
      state(std::ios_base::failbit);
  }
  return *this;
}


pvm_stream& 
pvm_stream::operator>>(int &n)
{
  if (good())
  {
    if (pvm_upkint(&n, 1, 1) < 0)
      state(std::ios_base::failbit);
  }
  return *this;
}


pvm_stream& 
pvm_stream::operator>>(std::string &s)
{
  if (good())
  {
    state(std::ios_base::failbit);

        // Read and sanity check length of string
    int n;
    if (pvm_upkint(&n, 1, 1) < 0)
      return *this;

    if (n < 0 || n > 10000)
    {
      m_fb.Warning() << "Unreasonable value for string length read from pvm: " << n << ". Aborting read";
      return *this;
    }
        // Read the actual string to temp buffer and copy to s
    std::vector<char> v(n+1);
    if (pvm_upkstr(&(v[0])) < 0)
      return *this;

        // pvm_upkstr adds a terminating zero
    s = &(v[0]);
    state(std::ios_base::goodbit);
  }
  return *this;
}


const int PvmSender::tid_message_id = 12;

PvmSender::PvmSender(std::istream *pInChannel, const std::string &sServerProg)
    : SlaveChannelSender(pInChannel)
    , m_fb("PvmSender")
    , m_sServerProg(sServerProg)
//    , m_pInChannel(&std::cin)
    , m_nReceiverTid(-1)
{
  if (pvm_recv(-1, tid_message_id) >= 0)
  {
    pvm_stream ps;
    ps >> m_nReceiverTid;
    if (!ps.good())
    {
      m_nReceiverTid = -1;
      m_fb.Error(E_PVMSENDER_RECV) << ": Couldn't extract receiver Tid from message.";
    }
    else
      m_fb.Info(2) << "Got receiver tid " << m_nReceiverTid;
  }
  else
    m_fb.Error(E_PVMSENDER_RECV) << ": Didn't get tid from receiver process.";
}


PvmSender::~PvmSender()
{
}


int 
PvmSender::SendMessage(const std::string &sServer, const std::string &sMessage)
{
  int nTid;
  if (GetTaskId(sServer, nTid))
    return m_fb.Error(E_PVMSENDER_SEND) << " to " << sServer;

  pvm_stream ps(true);
  ps << sServer << sMessage;
  if (!ps.good())
    return m_fb.Error(E_PVMSENDER_PACK);

  if (pvm_send(nTid, 0) < 0)
    return m_fb.Error(E_PVMSENDER_PVMSEND);

  m_fb.Info(3) << "Sent message to server " << sServer << ": " << sMessage << ".";
      //m_fb.Info(4) << "The message was: " << sMessage << ".";
  return 0;
}


int 
PvmSender::GetTaskId(const std::string &sServer, int &nTid)
{
  if (m_nReceiverTid < 0)
    return m_fb.Error(E_PVMSENDER_NORECVTID);
    
  TTidMap::const_iterator tit = m_serverTids.find(sServer);
  if (tit != m_serverTids.end())
  {
    nTid = tit->second;
    return 0;
  }

  m_fb.Info(2, "Spawning a pvm task on localhost..");
  std::string sHost = ".";
  if (pvm_spawn(const_cast<char*>(m_sServerProg.c_str()), 0, PvmTaskHost,
                const_cast<char*>(sHost.c_str()), 1, &nTid) != 1)
    return m_fb.Error(E_PVMSENDER_SPAWN) << ". Host: " << sHost << ", running " << m_sServerProg;

  pvm_stream ps(true);
  ps << m_nReceiverTid;
  if (!ps.good() || pvm_send(nTid, tid_message_id) < 0)
    return m_fb.Error(E_PVMSENDER_PVMSEND) << ", could not send client-side receiver tid to server.";
  
  m_serverTids[sServer] = nTid;
  return 0;
}


PvmReceiver::PvmReceiver(std::ostream *pOutChannel, int nSenderTid)
    : SlaveChannelReceiver(pOutChannel)
    , m_fb("PvmReceiver") //, m_pOutChannel(&std::cout)
{
  int nTid = pvm_mytid();
  m_fb.Info(2) << "Up and running. Now sending my tid (" << nTid << ") to PvmSender (tid " << nSenderTid << ").";
  
  pvm_stream ps(true);
  ps << nTid;
  if (!ps.good())
    m_fb.Error(E_PVMRECEIVER_PVMSEND) << ", could not prepare message with client-side receiver tid.";
  else if (pvm_send(nSenderTid, PvmSender::tid_message_id) < 0)
    m_fb.Error(E_PVMRECEIVER_PVMSEND) << ", could not send client-side receiver tid to client-side sender.";
}


PvmReceiver::~PvmReceiver()
{
}


int
PvmReceiver::ReceiveMessage(std::string &sServer, std::string &sMessage)
{
  int nBufId;
  if ((nBufId = pvm_recv(-1, -1)) < 0)
    return m_fb.Error(E_PVMRECEIVER_RECV);

  m_fb.Info(3, "Receiving pvm message...");
    
  pvm_stream ps;
  ps >> sServer >> sMessage;
  if (!ps.good())
    return m_fb.Error(E_PVMRECEIVER_UNPACK);

  return 0;
}

