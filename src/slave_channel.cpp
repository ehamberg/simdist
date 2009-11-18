/********************************************************************
 *   		slave_channel.cpp
 *   Created on Tue May 09 2006 by Boye A. Hoeverstad.
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

#include <simdist/slave_channel.h>
#include <simdist/messages.h>
#include <cstdlib>

// FeedbackError E_SLAVECHANNELSENDER_THREAD("Failed to create thread running slave send channel loop");
DEFINE_FEEDBACK_ERROR(E_SLAVECHANNELSENDER_THREAD, "Failed to create thread running slave send channel loop")
// FeedbackError E_SLAVECHANNELRECEIVER_THREAD("Failed to create thread running slave receive channel loop");
DEFINE_FEEDBACK_ERROR(E_SLAVECHANNELRECEIVER_THREAD, "Failed to create thread running slave receive channel loop")
// FeedbackError E_SLAVECHANNELSENDER_RUN("The main loop failed");
DEFINE_FEEDBACK_ERROR(E_SLAVECHANNELSENDER_RUN, "The main loop failed")
// FeedbackError E_SLAVECHANNELRECEIVER_RUN("The main loop failed");
DEFINE_FEEDBACK_ERROR(E_SLAVECHANNELRECEIVER_RUN, "The main loop failed")
// FeedbackError E_SLAVECHANNELRECEIVER_WRITE("Failed to write message to output channel.");
DEFINE_FEEDBACK_ERROR(E_SLAVECHANNELRECEIVER_WRITE, "Failed to write message to output channel.")
// FeedbackError E_SLAVECHANNELSENDER_STOP("Failed to stop slave channel sender thread");
DEFINE_FEEDBACK_ERROR(E_SLAVECHANNELSENDER_STOP, "Failed to stop slave channel sender thread")
// FeedbackError E_SLAVECHANNELRECEIVER_STOP("Failed to stop slave channel receiver thread");
DEFINE_FEEDBACK_ERROR(E_SLAVECHANNELRECEIVER_STOP, "Failed to stop slave channel receiver thread")


template<class T> void*
SlaveChannel_thread_func(void *pArg)
{
  T *pChannel = static_cast<T*>(pArg);
  int nRet = pChannel->Run();
  pChannel->m_fb.Info(2) << "Channel completed processing and returned with code " << nRet << ".";
  return reinterpret_cast<void*>(nRet);
}


SlaveChannelSender::SlaveChannelSender(std::istream *pInChannel)
    : m_fb("SlaveChannelSender"), m_pInChannel(pInChannel), m_threadId(0)
{
}


SlaveChannelSender::~SlaveChannelSender()
{
}


int 
SlaveChannelSender::Start()
{
  return m_fb.ErrorIfNonzero(pthread_create(&m_threadId, 0, SlaveChannel_thread_func<SlaveChannelSender>, this),
                             E_SLAVECHANNELSENDER_THREAD);

}

pthread_t  
SlaveChannelSender::GetThreadId() const
{
  return m_threadId;
}

// int 
// SlaveChannelSender::Stop()
// {
//  m_pInChannel->setstate(std::ios::eofbit);
//  return m_fb.ErrorIfNonzero(pthread_join(m_threadId, 0),
//                              E_SLAVECHANNELSENDER_STOP) << ", failed to join thread " << m_threadId << ".";
// }


void 
SlaveChannelSender::SetInputChannel(std::istream *pIc)
{
  m_pInChannel = pIc;
}


int 
SlaveChannelSender::Run()
{
  std::string sServer;
  while (std::getline(*m_pInChannel, sServer))
  {
    std::string sEOF, sLine, sMessage;
    std::getline(*m_pInChannel, sEOF);
    int nLineCtr = 0;
    while (std::getline(*m_pInChannel, sLine))
    {
      if (sLine == sEOF)
        break;
      sMessage += (nLineCtr++ ? "\n" : "") + sLine; // Can't test on sMessage.empty(). See comment in utils/ReadJob.
    }
    if (SendMessage(sServer, sMessage))
      return m_fb.Error(E_SLAVECHANNELSENDER_RUN);
    
    bool bShutdown;
    if (MessageRouter::Instance().CheckShutdown(sServer, sMessage, bShutdown))
      return m_fb.Error(E_SLAVECHANNELSENDER_RUN);
    if (bShutdown)
      break;
  }
  return 0;
}


SlaveChannelReceiver::SlaveChannelReceiver(std::ostream *pOutChannel)
    : m_fb("SlaveChannelReceiver"), m_pOutChannel(pOutChannel), m_threadId(0)
{
}


SlaveChannelReceiver::~SlaveChannelReceiver()
{
}


int 
SlaveChannelReceiver::Start()
{
  return m_fb.ErrorIfNonzero(pthread_create(&m_threadId, 0, SlaveChannel_thread_func<SlaveChannelReceiver>, this),
                             E_SLAVECHANNELRECEIVER_THREAD);

}


pthread_t  
SlaveChannelReceiver::GetThreadId() const
{
  return m_threadId;
}


// int 
// SlaveChannelReceiver::Stop()
// {
//   if (AbortReceiveMessage())
//     return m_fb.Error(E_SLAVECHANNELRECEIVER_STOP) << ", failed to abort receive block.";
//   return m_fb.ErrorIfNonzero(pthread_join(m_threadId, 0),
//                              E_SLAVECHANNELRECEIVER_STOP) << ", failed to join thread " << m_threadId << ".";
// }


void
SlaveChannelReceiver::SetOutputChannel(std::ostream *pOc)
{
  m_pOutChannel = pOc;
}


int
SlaveChannelReceiver::Run()
{
  m_fb.Info(2, "Starting Run loop.");
  std::string sServer, sMessage;
  while (!ReceiveMessage(sServer, sMessage))
  {
    std::stringstream ssEOF;
    ssEOF << "EOF";
    while (sMessage.find(ssEOF.str()) != std::string::npos)
      ssEOF << "-" << rand();

//     m_fb.Info(3) << "Received a message from " << sServer << ":" << sMessage << ".";
//     m_fb.Info(4) << "The message received was: " << sMessage << ".";
    
    *m_pOutChannel << sServer << "\n" 
                   << ssEOF.str() << "\n"
                   << sMessage << "\n"
                   << ssEOF.str() << "\n";
    m_pOutChannel->flush();
    if (!m_pOutChannel->good())
      return m_fb.Error(E_SLAVECHANNELRECEIVER_WRITE);
    m_fb.Info(3) << "Message received and passed on.";

    bool bShutdown;
    if (MessageRouter::Instance().CheckShutdown(sServer, sMessage, bShutdown))
      return m_fb.Error(E_SLAVECHANNELRECEIVER_RUN);
    if (bShutdown)
      break;
  }
  return 0;
}

