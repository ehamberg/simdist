/********************************************************************
 *   		messages.cpp
 *   Created on Thu Mar 23 2006 by Boye A. Hoeverstad.
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

#include <simdist/messages.h>
#include <iostream>
#include <cstdlib>

// FeedbackError E_MESSAGEPASSER_SEND("Failed to send message");
DEFINE_FEEDBACK_ERROR(E_MESSAGEPASSER_SEND, "Failed to send message")
// FeedbackError E_MESSAGEPASSER_RECEIVE("Failed to receive message");
DEFINE_FEEDBACK_ERROR(E_MESSAGEPASSER_RECEIVE, "Failed to receive message")
// FeedbackError E_MESSAGEPASSER_RECVLOOP("Message receive loop returned error");
DEFINE_FEEDBACK_ERROR(E_MESSAGEPASSER_RECVLOOP, "Message receive loop returned error")
// FeedbackError E_MESSAGERECEIVER_CREATE("Failed to create synchronized receiver");
DEFINE_FEEDBACK_ERROR(E_MESSAGERECEIVER_CREATE, "Failed to create synchronized receiver")
// FeedbackError E_MESSAGERECEIVER_THREAD("The message receiver thread should be running, but it isn't");
DEFINE_FEEDBACK_ERROR(E_MESSAGERECEIVER_THREAD, "The message receiver thread should be running, but it isn't")
// FeedbackError E_MESSAGERECEIVER_READ("Error while reading messages");
DEFINE_FEEDBACK_ERROR(E_MESSAGERECEIVER_READ, "Error while reading messages")
// FeedbackError E_MESSAGERECEIVER_STOP("Failed to stop message receiver thread");
DEFINE_FEEDBACK_ERROR(E_MESSAGERECEIVER_STOP, "Failed to stop message receiver thread")
// FeedbackError E_MESSAGERECEIVER_DESTROY("Failed to destroy message receive queue");
DEFINE_FEEDBACK_ERROR(E_MESSAGERECEIVER_DESTROY, "Failed to destroy message receive queue")
// FeedbackError E_MESSAGESTREAMER_READ("Error while reading messages from stream");
DEFINE_FEEDBACK_ERROR(E_MESSAGESTREAMER_READ, "Error while reading messages from stream")
// FeedbackError E_MESSAGESTREAMER_SEND("Error while writing messages to stream");
DEFINE_FEEDBACK_ERROR(E_MESSAGESTREAMER_SEND, "Error while writing messages to stream")



const char *MessagePasser::master_node_id = "Master_node";


MessagePasser::MessagePasser()
    : m_fb("MessagePasser")
{
}

int
MessagePasser::Send(const std::string &sServer, const std::string &sMessage)
{
  return m_fb.ErrorIfNonzero(MessageRouter::Instance().Send(sServer, sMessage),
                             E_MESSAGEPASSER_SEND);
}


int
MessagePasser::Receive(const std::string &sServer, std::string &sMessage)
{
  return m_fb.ErrorIfNonzero(MessageRouter::Instance().Receive(sServer, sMessage),
                             E_MESSAGEPASSER_RECEIVE);  
}


/*static*/ int
MessagePasser::SendShutdownMessage()
{
  Feedback fb("MessagePasser::SendShutdownMessage");
  return fb.ErrorIfNonzero(MessageRouter::Instance().Send(master_node_id, std::string("SHUTDOWN_MASTER\n")+master_node_id),
                             E_MESSAGEPASSER_SEND);
}


MessageRouter::MessageRouter()
    : m_fb("MessageRouter"), m_pInChannel(&std::cin), m_pOutChannel(&std::cout)
      , m_inputMutex("input-mutex"), m_outputMutex("output-mutex"), m_threadStateMtx("external-receiver-state")
      , m_threadState(not_started)
{
//   m_nIOCtr[0] = m_nIOCtr[1] = 0;
}


MessageRouter::~MessageRouter()
{
  for (TReceiverMap::iterator recvit = m_receivers.begin(); recvit != m_receivers.end(); recvit++)
  {
    pthread_cond_destroy(recvit->second.pCondition);
    delete recvit->second.pCondition;
  }
}

void 
MessageRouter::SetIOChannels(std::istream *pIs, std::ostream *pOs)
{
  if (pIs)
  {
    AutoMutex mtx;
    m_inputMutex.AcquireMutex(mtx);
    m_pInChannel = pIs;
  }
  if (pOs)
  {
    AutoMutex mtx;
    m_outputMutex.AcquireMutex(mtx);
    m_pOutChannel = pOs;
  }
}

void* thread_receive_func(void *pTD)
{
  Feedback::RegisterThreadDescription("MessageRouter-receiver");

      // Can't call Instance() here, because that will cause infinite
      // recursion.
  if (static_cast<MessageRouter*>(pTD)->RunExtReceiveLoop())
    Feedback("Thread function").Error(E_MESSAGEPASSER_RECVLOOP);

  return 0;
}


/********************************************************************
 *   Start up the receive loop thread.  Should only happen once.
 *   Can't be part of Instance() as first proposed, since
 *   changing input channel will not work if the receiver thread
 *   has already blocked on the original channel.
 *******************************************************************/
int
MessageRouter::StartReceiver()
{
  AutoMutex mtx;
  if (Instance().m_threadStateMtx.AcquireMutex(mtx))
    return m_fb.Error(E_MUTEX_LOCK) << ", couldn't check message receiver thread state.  May not be able to start receiver thread.";
  else
  {
    if (Instance().m_threadState == not_started)
    {
      Instance().m_threadState = running;
          // Create high priority thread to avoid filling up the I/O
          // channel.
      Instance().m_fb.Info(2, "Creating message receiver thread with highest possible priority");
      pthread_attr_t attr;
      pthread_attr_init(&attr);
      sched_param sched;
      sched.sched_priority = sched_get_priority_max(SCHED_OTHER);// + sched_get_priority_min(SCHED_OTHER)) / 2;
      if (pthread_attr_setschedparam(&attr, &sched) || 
          pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED))
        return m_fb.Error(E_THREAD_CREATE) << ", failed to set scheduling priorities for message receiving thread.";

      if (pthread_create(&(Instance().m_recvThreadId), &attr, thread_receive_func, &(Instance())))
        return m_fb.Error(E_THREAD_CREATE) << ", failed to start message receiving thread.";
      pthread_attr_destroy(&attr);
    }

    if (Instance().m_threadState != running)
      return m_fb.Error(E_MESSAGERECEIVER_THREAD);
  }

  return 0;
}


pthread_t 
MessageRouter::GetReceiverThreadId() const
{
  return m_recvThreadId;
}


// /********************************************************************
//  *   Stop the receive loop thread.
//  *******************************************************************/
// int 
// MessageRouter::StopReceiver()
// {
//   Feedback fb("MessageRouter::StopReceiver");

//   AutoMutex mtx;
//   if (Instance().m_threadStateMtx.AcquireMutex(mtx))
//     return fb.Error(E_MESSAGERECEIVER_STOP) << ", couldn't check message receiver thread state.  Unable to stop receiver thread.";

//   pthread_t tid = Instance().m_recvThreadId;
//   mtx.Unlock();

//   if (pthread_cancel(tid))
//     return fb.Error(E_MESSAGERECEIVER_STOP) << ", failed to cancel receiver thread.";

//   return fb.ErrorIfNonzero(pthread_join(tid, 0),
//                              E_MESSAGERECEIVER_STOP) << ", failed to join thread " << tid << ".";
// }


/********************************************************************
 *   Return Singleton instance
 ********************************************************************/
/*static*/ MessageRouter&
MessageRouter::Instance()
{
  static MessageRouter instance;
  return instance;
}


int
MessageRouter::Send(const std::string &sServer, const std::string &sMessage)
{
  m_fb.Info(4) << "MessageRouter " << this << " sending message to " << sServer << "...";

  AutoMutex mtx;
  if (m_outputMutex.AcquireMutex(mtx))
    return m_fb.Error(E_MUTEX_LOCK) << ". Failed to send message to server " << sServer << ".";

  MessageStreamer streamer;
  streamer.StreamEncode(*m_pOutChannel, sServer, sMessage);


      //!!- Only works if synched send doesn't add any extra data.
      //!!- But then again, this is just for debugging.
//   AutoMutex mtxCtr;
//   m_ctrLock.AcquireMutex(mtxCtr);
//   m_nIOCtr[0] += sMessage.size();
//   m_fb.Info(3) << "I/O balance now " << m_nIOCtr[0] - m_nIOCtr[1] << " send-receive.";
//   mtxCtr.Unlock();

  m_fb.Info(3) << "MessageRouter " << this << " successfully sent message to " 
               << sServer << " (" << sMessage.size() << " bytes).";
  return 0;
}



/********************************************************************
 *   Set state to the state of the external receiver thread.
 *******************************************************************/
int 
MessageRouter::GetExtReceiverState(TThreadState &state)
{
  AutoMutex mtx;
  if (m_threadStateMtx.AcquireMutex(mtx))
    return m_fb.Error(E_MUTEX_LOCK);
  state = m_threadState;
  return 0;
}


/********************************************************************
 *   Receive a message from server sServer.  This function blocks
 *   until the right message is received, or until the external
 *   receiver loop has failed.
 *******************************************************************/
int
MessageRouter::Receive(const std::string &sServer, std::string &sMessage)
{
  m_fb.Info(4) << "MessageRouter " << this << " receiving message from " << sServer << "...";

  AutoMutex mtx;
  if (m_inputMutex.AcquireMutex(mtx))
    return m_fb.Error(E_MUTEX_LOCK);

  m_fb.Info(4) << "MessageRouter " << this << " about to block waiting for message";

  TMessageQueue *pQueue;
  if (GetReceiver(sServer, &pQueue))
    return m_fb.Error(E_MESSAGERECEIVER_CREATE);
  TThreadState state;
  while (pQueue->messages.empty() && !GetExtReceiverState(state) && state == running)
  {
    if (pthread_cond_wait(pQueue->pCondition, mtx.GetLockedMutex()))
      return m_fb.Error(E_COND_WAIT);
// NOT NECESSARY, INSERTING ELEMENTS NEVER INVALIDATES LIST POINTERS.
//         // Get pQueue again, since the wait above releases the
//         // condition, and another thread may step in and change the
//         // receiver list.
//     if (GetReceiver(sServer, &pQueue))
//       return m_fb.Error(E_MESSAGERECEIVER_CREATE);
  }

  if (GetExtReceiverState(state) || state != running)
    return m_fb.Error(E_MESSAGEPASSER_RECEIVE) << ". Unable to check external receiver state or external receiver failed.";

  m_fb.Info(3) << "MessageRouter " << this << " received message from " 
               << sServer << ", now about to process it.";

  sMessage = pQueue->messages.front();
  pQueue->messages.pop_front();

  bool bShutdown;
  if (CheckShutdown(sServer, sMessage, bShutdown))
    return m_fb.Error(E_MESSAGEPASSER_RECEIVE);
  if (bShutdown) 
    DestroyReceiver(sServer);

  return 0;
}


/********************************************************************
 *   Return a message queue for server sServer.  If the queue doesn't
 *   exist yet, it will be created.
 *
 *   The mutex m_inputMutex should be locked upon entry to this
 *   function.
 ******************************************************************/
int 
MessageRouter::GetReceiver(const std::string &sServer, TMessageQueue **ppQueue)
{
  if (m_receivers.find(sServer) == m_receivers.end())
  {
    TMessageQueue queue;
    queue.pCondition = new pthread_cond_t;
    if (pthread_cond_init(queue.pCondition, 0))
      return m_fb.Error(E_COND_INIT) << ".  Unable to receive messages from " << sServer << ".";

    m_receivers[sServer] = queue;
  }

  *ppQueue = &(m_receivers[sServer]);
  return 0;
}


/********************************************************************
 *   Delete the message queue for server sServer.
 *
 *   The mutex m_inputMutex should be locked upon entry to this
 *   function.
 ******************************************************************/
int
MessageRouter::DestroyReceiver(const std::string &sServer)
{
  TReceiverMap::iterator recvit = m_receivers.find(sServer);
  if (recvit == m_receivers.end())
    return m_fb.Error(E_MESSAGERECEIVER_DESTROY) << ": No receive queue available for server " << sServer << ".";
  pthread_cond_destroy(recvit->second.pCondition);
  delete recvit->second.pCondition;
  m_receivers.erase(recvit);
  return 0;
}


/********************************************************************
 *   If we receive a shutdown-message, pass it on to all slave clients
 *   waiting for a response from their servers.
 *
 *   The mutex m_inputMutex should be locked upon entry to this
 *   function.
 *   
 *******************************************************************/
int 
MessageRouter::Shutdown()
{
  for (TReceiverMap::iterator recvit = m_receivers.begin(); recvit != m_receivers.end(); recvit++)
  {
    TMessageQueue &queue = recvit->second;
    queue.messages.clear();
    queue.messages.push_back("SHUTDOWN_MASTER\n" + recvit->first);
    if (pthread_cond_signal(queue.pCondition))
      throw int(m_fb.Error(E_COND_SIGNAL) << ", message receiver thread failed to signal receiver queue condition.");
  }
  return 0;
}

/********************************************************************
 *   Check if the received message is a shutdown-message.  At the
 *   moment, SHUTDOWN_MASTER is the only message the master
 *   understands.
 *
 *   The mutex m_inputMutex should be locked upon entry to this
 *   function.
 *   
 *******************************************************************/
int
MessageRouter::CheckShutdown(const std::string &sServer, const std::string &sMessage, bool &bShutdown) const
{
  bShutdown = false;

  std::stringstream ss(sMessage);
  std::string sTag, sServerMsg;
  std::getline(ss, sTag);
  std::getline(ss, sServerMsg);

  bShutdown = (sServer == sServerMsg && sTag == "SHUTDOWN_MASTER");

  if (sServerMsg == MessagePasser::master_node_id 
      && sTag != "SHUTDOWN_MASTER")
    return m_fb.Error(E_MESSAGERECEIVER_READ) << "Master received the message \"" 
                                              << sMessage << "\", the only accepted message reads \""
                                              << "SHUTDOWN_MASTER\n" << MessagePasser::master_node_id 
                                              << "\".";
  return 0;
}


/********************************************************************
 *   This function is called from a separate thread, and polls the
 *   (external) input stream continuosly.  Every time a message
 *   arrives, the entire message is read and queued.  Queue is
 *   selected based on server ID.  Message format is described in
 *   header.
 *
 *   Errors may occur at several points in this function.  Errors
 *   result in a throw with error code, upon which the catch section
 *   cleans up. Cleaning up means setting state to failed, and then
 *   waking up all receiving threads.
 *******************************************************************/
int 
MessageRouter::RunExtReceiveLoop()
{
  m_fb.Info(2, "Up and running and ready to receive messages");

  MessageStreamer streamer;
  try 
  {

    while (m_pInChannel->good() && !m_pInChannel->eof())
    {
      std::string sServer, sMessage;
      if (streamer.StreamDecode(*m_pInChannel, sServer, sMessage))
        throw int(m_fb.Error(E_MESSAGERECEIVER_READ));

      AutoMutex mtx;
      if (m_inputMutex.AcquireMutex(mtx))
        throw int(m_fb.Error(E_MUTEX_LOCK) << ", unable to pass on new message to receiver.");

      bool bShutdown;
      if (int nErr = CheckShutdown(sServer, sMessage, bShutdown))
        throw nErr;
      if (bShutdown)
      {
        Shutdown();
        break;
      }

      TMessageQueue *pQueue;
      if (GetReceiver(sServer, &pQueue))
        throw int(m_fb.Error(E_MESSAGERECEIVER_CREATE) << ", unable to pass on new message to receiver.");

      pQueue->messages.push_back(sMessage);
      if (pthread_cond_signal(pQueue->pCondition))
        throw int(m_fb.Error(E_COND_SIGNAL) << ", message receiver thread failed to signal receiver queue condition.");
    }
  } 
  catch (int nErrCode)
  {
    AutoMutex mtx;
    m_threadStateMtx.AcquireMutex(mtx);
    m_threadState = failed;
    mtx.Unlock();
    for (TReceiverMap::iterator recvit = m_receivers.begin(); recvit != m_receivers.end(); recvit++)
      pthread_cond_signal(recvit->second.pCondition);
    return nErrCode;
  }

  m_fb.Info(2, "Receiver thread completed successfully.");
  return 0;
}


MessageStreamer::MessageStreamer()
    : m_fb("MessageStreamer")
{
}


int 
MessageStreamer::StreamEncode(std::ostream &s, const std::string &sServer, const std::string &sMessage)
{
      // Stream in the first EOF.  When simply assigning, it is
      // overwritten by the subsequent stream op.
  std::stringstream ssEOF;
  ssEOF << "EOF";
  while (sMessage.find(ssEOF.str()) != std::string::npos)
    ssEOF << "-" << rand();

  s << sServer << std::endl << ssEOF.str().c_str() << std::endl;
  s << sMessage << std::endl;
  s << ssEOF.str().c_str() << std::endl << std::flush;

  if (s.good())
    return 0;
  return m_fb.Error(E_MESSAGESTREAMER_SEND) << ": Stream not good after write.";
}


int 
MessageStreamer::StreamDecode(std::istream &s, std::string &sServer, std::string &sMessage)
{
  bool bEOFReached = false;
  std::string sEOF, sLine;

//   m_fb.Info(2, "sleeeeping..");
//   sleep(10);
  m_fb.Info(3, "reading..");

  s >> sServer >> sEOF;

  m_fb.Info(4, "About to read message from ") << sServer << " until next EOF-mark: " << sEOF << ".";

      // Chomp off newline after sEOF
  std::getline(s, sLine);

  int nLineCtr = 0;
  while (std::getline(s, sLine))
  {
    m_fb.Info(4, "read line: ") << sLine;
    if (sLine == sEOF)
    {
      bEOFReached = true;
      break;
    }
    else
    {
      if (nLineCtr++) // Can't test on sMessage.empty(). See comment in utils/ReadJob.
        sMessage += "\n";
      sMessage += sLine;
    }
  }

  if (!bEOFReached)
    return m_fb.Error(E_MESSAGESTREAMER_READ) << ", EOF mark never reached.";

  m_fb.Info(4) << "Received message from " << sServer << ":\n" << sMessage << ".";

  return 0;
}
