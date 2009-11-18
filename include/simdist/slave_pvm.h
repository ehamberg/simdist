/********************************************************************
 *   		slave_pvm.h
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
 *   Slave message passing and sending using Pvm
 *******************************************************************/

#if !defined(__SLAVE_PVM_H__)
#define __SLAVE_PVM_H__

#include "feedback.h"
#include "slave_channel.h"

#include <map>

// extern FeedbackError E_PVMSTREAM_INITSEND;
DECLARE_FEEDBACK_ERROR(E_PVMSTREAM_INITSEND)

// extern FeedbackError E_PVMSENDER_SEND;
DECLARE_FEEDBACK_ERROR(E_PVMSENDER_SEND)
// extern FeedbackError E_PVMSENDER_RECV;
DECLARE_FEEDBACK_ERROR(E_PVMSENDER_RECV)
FEEDBACK_ERROR(E_PVMSENDER_PACK, "Failed to pack/create pvm message");
// extern FeedbackError E_PVMSENDER_PVMSEND;
DECLARE_FEEDBACK_ERROR(E_PVMSENDER_PVMSEND)
// extern FeedbackError E_PVMSENDER_SPAWN;
DECLARE_FEEDBACK_ERROR(E_PVMSENDER_SPAWN)
// extern FeedbackError E_PVMSENDER_NORECVTID;
DECLARE_FEEDBACK_ERROR(E_PVMSENDER_NORECVTID)

FEEDBACK_ERROR(E_PVMRECEIVER_UNPACK, "Failed to unpack message. Data not properly formatted?");
// extern FeedbackError E_PVMRECEIVER_RECV;
DECLARE_FEEDBACK_ERROR(E_PVMRECEIVER_RECV)
// extern FeedbackError E_PVMRECEIVER_PVMSEND;
DECLARE_FEEDBACK_ERROR(E_PVMRECEIVER_PVMSEND)


/********************************************************************
 *   This class wraps pvm read and write operations.  It is inspired
 *   by the class of the same name in "Parallel and Distributed
 *   Programming using C++" by Hughes and Hughes, 2003, but has been
 *   stripped down quite a bit.  It does not send on each stream op,
 *   and it does NOT include any locking.  Rather, it only supplies <<
 *   and >> operator overloads, and maintains state in a way similar
 *   to the std stream classes.
 *
 *   IMPORTANT: Data written using this class should also be read
 *   using this class.  The main reason for this is that the string
 *   overloads of << and >> store data by writing the length of the
 *   string first, followed by the actual string.
 *
 *   Further << and >> overloads are written as they are needed.
 *
 *   TODO: See MPI stream.
 *******************************************************************/
class pvm_stream
{
  Feedback m_fb;
  std::ios_base::iostate m_state;
  std::ios_base::iostate state(std::ios_base::iostate new_state);
  std::ios_base::iostate state() const;
      // static pvm_stream m_instance;
public:
  pvm_stream(bool initsend = false);
  pvm_stream(const pvm_stream &copy);
  pvm_stream& operator=(const pvm_stream &copy);
      // static pvm_stream& Instance(bool initsend = false);

  pvm_stream& initsend();
  bool good() const;
  bool operator()() const;

  pvm_stream& operator<<(const int n);
  pvm_stream& operator<<(const std::string &s);
  pvm_stream& operator>>(int &n);
  pvm_stream& operator>>(std::string &s);
};



class PvmSender : public SlaveChannelSender
{
  Feedback m_fb;
  std::string m_sServerProg;
  typedef std::map<std::string, int> TTidMap;
  TTidMap m_serverTids;
  int m_nReceiverTid;

  virtual int SendMessage(const std::string &sServer, const std::string &sMessage);
  int GetTaskId(const std::string &sServer, int &nTid);
public:
  static const int tid_message_id;

  PvmSender(std::istream *pInChannel, const std::string &sServerProg);
  virtual ~PvmSender();
};



class PvmReceiver : public SlaveChannelReceiver
{
  Feedback m_fb;
  virtual int ReceiveMessage(std::string &sServer, std::string &sMessage);
public:
  PvmReceiver(std::ostream *pOutChannel, int nSenderTid);
  virtual ~PvmReceiver();
};

#endif
