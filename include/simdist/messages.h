/********************************************************************
 *   		messages.h
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
 *   This file contains classes to read and write messages sent from
 *   the master node (containing slave clients) to the slave nodes
 *   (containing slave servers).  
 *
 *   The following messages are defined in the distributed evaluation
 *   system: CONNECT, FAIL, READY, ABORT, ABORTED_READY, JOBDATA,
 *   RESULTS, TERMINATE, TERMINATED and SHUTDOWN.
 *   
 *    * CONNECT: Sent from the master to establih a connection with a
 *      slave server
 *   
 *      - server: string.  Name identifying the server.  This name
 *        should be used by both server and client in all subsequent
 *        communication.
 *   
 *      - version: integer.  The message set version employed by the
 *        master, to which the server should comply.
 *   
 *      - input mode: string.  How the slave expects its input data
 *        formatted.  Currently supported modes are EOF, indicating
 *        that each job data stream is terminated on both ends by a
 *        line containing an EOF mark, or SIMPLE, indicating the
 *        data lines are simplqy written out as they are.  Inferred
 *        from master output mode.
 *
 *      - output mode: string.  Equivalent to input mode, except
 *        that the SIMPLE mode is substituted by an integer indicating
 *        the number of lines to read from the slave before sending
 *        results to the master.
 *
 *      - program: string.  The evaluation program the server should
 *        start (performing the actual work).
 *      
 *      - arguments: string. Arguments sent to the slave program.
 *   
 *    * FAIL: Sent from a slave server to indicate that something has
 *      gone wrong.
 *   
 *      - server: string.
 *   
 *      - code: integer.  An error code.  0 indicates an unregistered
 *        or unexpected error.  Optional once the message format
 *        supports optional tags (i.e. once the xml format is in
 *        place).
 *   
 *      - description: string.  A textual description of the error.  This
 *        error will typically be passed on to the user of the system,
 *        and so need not be machine processed, but should be human
 *        readable.
 *   
 *    * READY: Sent from a slave server to indicate it is ready to
 *      receive job data.
 *   
 *      - server: string.
 *   
 *    * JOB: Job data sent from the master.  In the future, the master
 *      could be extended to pack several jobs in a job set.  In that
 *      case, the RESULTS and ABORT messages will have to be revised
 *      as well.
 *   
 *      - server: string.
 *   
 *      - count: string. The number of jobs in this message.
 *
 *      - jobID: string. Unique identifier of this job, as designated by
 *        the master.
 *
 *      - size: int.  The number of bytes of data in this job.
 *   
 *      - data: string. The data to be sent to the program to which the
 *        server is connected.
 *   
 *    * RESULTS: The result of a job processed on the server.  This message
 *      implies that the server is once again ready to receive new job(s).
 *   
 *      - server: string.
 *
 *      - count: string. The number of results in this message.
 *
 *      - jobID: string
 *   
 *      - time: string.  The actual time (as opposed to cpu time) spent
 *        in the server processing the job.
 *
 *      - size: int.  The number of bytes of data in this result.
 *   
 *      - result: string.  The output from the server evaluation program.
 *   
 *    * ABORT: Sent from the master in order to abort the current job on
 *      the server.
 *   
 *      - server: string.
 *   
 *      - jobID: string.
 *   
 *    * ABORTED_READY: Response from an aborted server, indicating that it
 *      is once again ready to receive data.
 *   
 *      - server: string. 
 *   
 *      - jobID: string.
 *   
 *    * TERMINATE: Sent from the master to request termination of a server.
 *   
 *      - server: string.
 *   
 *    * TERMINATED: Response from a terminated server, indicating that it
 *      is going down.
 *   
 *      - server: string.
 *   
 *    * SHUTDOWN_MASTER: Sent once from the master to itself
 *      after all slaves have been shut down, to indicate that
 *      all threads in the message passing subsystem should
 *      terminate.  The ID of the master is "Master_node", as
 *      defined by MessagePasser::master_node_id.
 *
 *      - server: string
 *
 *   Users will use the MessagePasser class to send and receive
 *   messages.  Messages will be sent with the Send() command, and
 *   received with the Receive() command.
 *
 *   Message identifiers are written in all uppercase as the first
 *   line of a message, followed by the above described elements on
 *   separate lines.
 *
 *   The MessageRouter class provides synchronized access to the
 *   system-wide input/output channels for all the MessagePasser
 *   objects.  Users will normally not come in contact with the
 *   MessageRouter.
 * 
 *   The MessageRouter sending thread (actually the
 *   MessageStreamer) inserts an EOF tag in the same manner as
 *   with here-documents; so that the second line of a message
 *   identifies the EOF-mark, this being a string not found
 *   anywhere in the message.  When reading, entire messages are
 *   read and queued waiting for a receiver.
 *
 *   Messages are read and written in the following format:
 *      LINE 1:   Server ID
 *      LINE 2:   EOF-Tag
 *      LINE 3 - n: Message
 *      LINE n+1: EOF-Tag
 *
 *   TODO: The server id occurs twice now, once in the actual
 *   message, and once as an argument to the send/receive functions.
 *   Is this necessary?  It must be present in the Receive function,
 *   and should logically be present in the Send function for use when
 *   aborting other servers, so the question must be if it serves any
 *   purpose in the message itself.  Perhaps for debugging or
 *   consistency?
 *
 *******************************************************************/


#if !defined(__MESSAGES_H__)
#define __MESSAGES_H__

#include "feedback.h"
#include "syncutils.h"

#include <list>
#include <string>
#include <pthread.h>

// extern FeedbackError E_MESSAGEPASSER_SEND;
DECLARE_FEEDBACK_ERROR(E_MESSAGEPASSER_SEND)
// extern FeedbackError E_MESSAGEPASSER_RECEIVE;
DECLARE_FEEDBACK_ERROR(E_MESSAGEPASSER_RECEIVE)
// extern FeedbackError E_MESSAGEPASSER_RECVLOOP;
DECLARE_FEEDBACK_ERROR(E_MESSAGEPASSER_RECVLOOP)

// extern FeedbackError E_MESSAGERECEIVER_CREATE;
DECLARE_FEEDBACK_ERROR(E_MESSAGERECEIVER_CREATE)
// extern FeedbackError E_MESSAGERECEIVER_THREAD;
DECLARE_FEEDBACK_ERROR(E_MESSAGERECEIVER_THREAD)
// extern FeedbackError E_MESSAGERECEIVER_READ;
DECLARE_FEEDBACK_ERROR(E_MESSAGERECEIVER_READ)
// extern FeedbackError E_MESSAGERECEIVER_STOP;
DECLARE_FEEDBACK_ERROR(E_MESSAGERECEIVER_STOP)
// extern FeedbackError E_MESSAGERECEIVER_DESTROY;
DECLARE_FEEDBACK_ERROR(E_MESSAGERECEIVER_DESTROY)

// extern FeedbackError E_MESSAGESTREAMER_READ;
DECLARE_FEEDBACK_ERROR(E_MESSAGESTREAMER_READ)
// extern FeedbackError E_MESSAGESTREAMER_SEND;
DECLARE_FEEDBACK_ERROR(E_MESSAGESTREAMER_SEND)


class MessagePasser
{
  Feedback m_fb;
public:
  static const char *master_node_id;

  MessagePasser();
  int Send(const std::string &sServer, const std::string &sMessage);
  int Receive(const std::string &sServer, std::string &sMessage);

  static int SendShutdownMessage();
};


class MessageRouter
{
  typedef struct TMessageQueueVar
  {
    std::list<std::string> messages;
    pthread_cond_t *pCondition;
  } TMessageQueue;

  typedef std::map<std::string, TMessageQueue> TReceiverMap;
  typedef enum { not_started, running, failed } TThreadState;

  MessageRouter();

  Feedback m_fb;
  std::istream *m_pInChannel;
  std::ostream *m_pOutChannel;
  LockableObject m_inputMutex, m_outputMutex;

//   LockableObject m_ctrLock;
//   int m_nIOCtr[2];

  TReceiverMap m_receivers;

  LockableObject m_threadStateMtx;
  TThreadState m_threadState;
  pthread_t m_recvThreadId;
  friend void* thread_receive_func(void *);
  int RunExtReceiveLoop(); // Function called from thread, this does the actual reception

  int GetExtReceiverState(TThreadState &state);
  int GetReceiver(const std::string &sServer, TMessageQueue **ppQueue);
  int DestroyReceiver(const std::string &sServer);

  int Shutdown();
public:
  static MessageRouter& Instance();
  virtual ~MessageRouter();

  int Send(const std::string &sServer, const std::string &sMessage);
  int Receive(const std::string &sServer, std::string &sMessage);
  int StartReceiver();
  pthread_t GetReceiverThreadId() const;
  
  int CheckShutdown(const std::string &sServer, const std::string &sMessage, bool &bShutdown) const;

  void SetIOChannels(std::istream *pIs, std::ostream *pOs);
};



/********************************************************************
 *   Utility class that codes/decodes a message block to a
 *   streamed format.  Doesn't contain any synchronization.  Used
 *   by the MessageRouter class and by slaves.
 *******************************************************************/
class MessageStreamer
{
  Feedback m_fb;
public:
  MessageStreamer();
  int StreamEncode(std::ostream &s, const std::string &sServer, const std::string &sMessage);
  int StreamDecode(std::istream &s, std::string &sServer, std::string &sMessage);
};



#endif
