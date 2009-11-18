/********************************************************************
 *   		test_messages.cpp
 *   Created on Mon Mar 27 2006 by Boye A. Hoeverstad.
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
 *   This program tests the message passer library.
 *
 *   Testing happens as follows: The program forks, creating one image
 *   running in "Master" mode, and the other in "Slave" mode.  The
 *   master opens read and write pipes to the slave.  The master
 *   creates a number of threads to write messages and wait for
 *   replies.  The slave does the same thing to read messages and
 *   write replies.
 *
 *   Sending and receiving is repeated a number of times which can be
 *   specified on the command line, as can the number of threads.
 *******************************************************************/


#include <simdist/messages.h>
#include <simdist/feedback.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <cstring>
#include <cstdlib>

const int default_info_level = 2;
const size_t max_bulk_size = 2;

// FeedbackError E_MASTER_SEND("Master failed in sending message");
DEFINE_FEEDBACK_ERROR(E_MASTER_SEND, "Master failed in sending message")
// FeedbackError E_MASTER_RECEIVE("Master failed in receiving message");
DEFINE_FEEDBACK_ERROR(E_MASTER_RECEIVE, "Master failed in receiving message")
// FeedbackError E_MASTER_VERIFY("Master received incorrect message");
DEFINE_FEEDBACK_ERROR(E_MASTER_VERIFY, "Master received incorrect message")

// FeedbackError E_SLAVE_SEND("Slave failed in sending message");
DEFINE_FEEDBACK_ERROR(E_SLAVE_SEND, "Slave failed in sending message")
// FeedbackError E_SLAVE_RECEIVE("Slave failed in receiving message");
DEFINE_FEEDBACK_ERROR(E_SLAVE_RECEIVE, "Slave failed in receiving message")
// FeedbackError E_SLAVE_VERIFY("Slave received incorrect message");
DEFINE_FEEDBACK_ERROR(E_SLAVE_VERIFY, "Slave received incorrect message")

// FeedbackError E_TEST_MODE("Invalid mode. Should be \"master\" or \"slave\"");
DEFINE_FEEDBACK_ERROR(E_TEST_MODE, "Invalid mode. Should be \"master\" or \"slave\"")
// FeedbackError E_TEST_MESSAGES("Different number of send and receive messages");
DEFINE_FEEDBACK_ERROR(E_TEST_MESSAGES, "Different number of send and receive messages")

using namespace std;


int test_msg_sequence(Feedback &fb, const string &sMode, const string &sServer, const vector<string> &masterMsgs, const vector<string> &slaveMsgs)
{
  if (masterMsgs.size() != slaveMsgs.size())
    return fb.Error(E_TEST_MESSAGES) << " (" << masterMsgs.size() << " master messages, and " 
                                     << slaveMsgs.size() << " slave messages).";

  if (sMode != "Master" && sMode != "Slave")
    return fb.Error(E_TEST_MODE) << ", was \"" << sMode << "\"";

  MessagePasser mp;
  for (size_t nRep = 0; nRep < masterMsgs.size(); nRep++)
  {
    if (sMode == "Master")
    {
      fb.Info(2, "Sending message..");
      if (mp.Send(sServer, masterMsgs[nRep]))
        return fb.Error(E_MASTER_SEND) 
          << " while sending message number " << nRep;
      fb.Info(2, "Message sent, now receiving..");
      string sMsg;
      if (mp.Receive(sServer, sMsg))
        return fb.Error(E_MASTER_RECEIVE) 
          << " while receiving message number " << nRep;
      fb.Info(2, "Message received");
      if (sMsg != slaveMsgs[nRep])
        return fb.Error(E_MASTER_VERIFY) 
          << ", message received, but contents wrong. Was:----\n" << sMsg 
          << "\n---- Should be: ----\n" << slaveMsgs[nRep] << "\n----\n";
      fb.Info(2) << "Successfully sent/received message number " << nRep << " out of " << masterMsgs.size();
    }
    else
    {
      string sMsg;
      fb.Info(2, "Receiving message..");
      if (mp.Receive(sServer, sMsg))
        return fb.Error(E_SLAVE_SEND)
          << " while receiving message number " << nRep;
      if (sMsg != masterMsgs[nRep])
        return fb.Error(E_SLAVE_RECEIVE) 
          << ", message received, but contents wrong. Was:----\n" << sMsg 
          << "\n---- Should be: ----\n" << masterMsgs[nRep] << "\n----\n";
      fb.Info(2, "Message received, now sending..");
      if (mp.Send(sServer, slaveMsgs[nRep]))
        return fb.Error(E_SLAVE_VERIFY) 
          << " while sending message number " << nRep;
      fb.Info(2) << "Successfully sent/received message number " << nRep << " out of " << masterMsgs.size();
    }
  }
  fb.Info(2) << "Done! Successfully sent/received " << masterMsgs.size() << " messages";
  return 0;
}


int test_msg_bulk(Feedback &fb, const string &sMode, const string &sServer, const vector<string> &masterMsgs, const vector<string> &slaveMsgs)
{
  if (masterMsgs.size() != slaveMsgs.size())
    return fb.Error(E_TEST_MESSAGES) << " (" << masterMsgs.size() << " master messages, and " 
                                     << slaveMsgs.size() << " slave messages).";

  if (sMode != "Master" && sMode != "Slave")
    return fb.Error(E_TEST_MODE) << ", was \"" << sMode << "\"";

  MessagePasser mp;
  size_t nMsgCtr = 0;//, nBulkCtr = 0;
  while (nMsgCtr < masterMsgs.size())
  {
    const size_t nBulkMax = min(max_bulk_size, masterMsgs.size() - nMsgCtr);
    for (size_t nBulkMsgCtr = 0; nBulkMsgCtr < nBulkMax; nBulkMsgCtr++, nMsgCtr++)
    {
      string sMsg = (sMode == "Master") ? masterMsgs[nBulkMsgCtr] : slaveMsgs[nBulkMsgCtr];
      FeedbackError err = (sMode == "Master") ? E_MASTER_SEND() : E_SLAVE_SEND();
      fb.Info(2, "Sending message..");
      if (mp.Send(sServer, sMsg))
        return fb.Error(err) 
          << " while sending message number " << nMsgCtr;
      fb.Info(2, "Message sent");
    }

    for (size_t nBulkMsgCtr = 0; nBulkMsgCtr < nBulkMax; nBulkMsgCtr++)
    {
      string sMsgVerify = (sMode == "Master") ? slaveMsgs[nBulkMsgCtr] : masterMsgs[nBulkMsgCtr];
      FeedbackError errReceive = (sMode == "Master") ? E_MASTER_RECEIVE() : E_SLAVE_RECEIVE();
      FeedbackError errVerify = (sMode == "Master") ? E_MASTER_VERIFY() : E_SLAVE_VERIFY();

      string sMsg;
      fb.Info(2, "Receiving message..");
          // Error here not entirely correct (nMsgCtr wrong), but close enough.
      if (mp.Receive(sServer, sMsg))
        return fb.Error(errReceive) 
          << " while receiving message number " << nMsgCtr;
      fb.Info(2, "Message received");
      if (sMsg != sMsgVerify)
        return fb.Error(errVerify) 
          << ", message received, but contents wrong. Was:----\n" << sMsg 
          << "\n---- Should be: ----\n" << sMsgVerify << "\n----\n";
    }
    fb.Info(2) << "Successfully sent/received " << nMsgCtr << " out of " << masterMsgs.size() << " messages.";
  }
  fb.Info(1) << "Done! Successfully sent/received " << masterMsgs.size() << " messages";
  return 0;
}


typedef struct TThreadDataVar
{
  string sMode;
  string sServer;
  int nNumRepetitions;
  enum { bulk, sequence } order;
  int nResult;
} TThreadData;



void* thread_func(void *arg)
{
  TThreadData &td = *static_cast<TThreadData*>(arg);

  Feedback fb(td.sServer + ":" + td.sMode + (td.order == TThreadData::bulk ? ":bulk" : ":sequence"));
  fb.Info(2, "Loaded thread function, about to create messages");

  vector<string> masterMsgs, slaveMsgs;
  while (static_cast<int>(masterMsgs.size()) < td.nNumRepetitions)
  {
    int nCtr = static_cast<int>(masterMsgs.size());
    stringstream ssMaster, ssSlave;
    switch(nCtr % 6)
    {
        case 0:
          ssMaster << "CONNECT\n" << td.sServer << "\n1\nMasterProg" << nCtr;
          ssSlave  << "CONNECT\n" << td.sServer << "\n2\nSlaveProg" << nCtr;
          break;
        case 1:
          ssMaster << "READY\n" << td.sServer;
          ssSlave  << "FAIL\n" << td.sServer << "\n2\nThe damn thing failed for the " << nCtr << " time!";
          break;
        case 2:
          ssMaster << "JOB\n" << td.sServer << "\nDaJobID" << nCtr << "\nThis is data. Get to work.";
          ssSlave  << "RESULTS\n" << td.sServer << "\nDaJobID" << nCtr << "\nThese are my results.\nIt took ages.";
          break;
        case 3:
          ssMaster << "ABORT\n" << td.sServer << "\nDaJobID" << nCtr;
          ssSlave  << "ABORTED_READY\n" << td.sServer << "\nDaJobID" << nCtr;
          break;
        case 4:
          ssMaster << "TERMINATE\n" << td.sServer;
          ssSlave  << "TERMINATED\n" << td.sServer;
          break;
        case 5:
          ssMaster << "EOF-Test" << td.sServer << "\nEOF\n";
          ssMaster << "EOFReply" << td.sServer << "\nEOFEOF";
          break;
        default:
          cerr << "Error in thread function!\n";
          td.nResult = 1;
          return 0;
    }
    masterMsgs.push_back(ssMaster.str());
    slaveMsgs.push_back(ssSlave.str());
  }    


  if (td.order == TThreadData::bulk)
    td.nResult = test_msg_bulk(fb, td.sMode, td.sServer, masterMsgs, slaveMsgs);
  else 
    td.nResult = test_msg_sequence(fb, td.sMode, td.sServer, masterMsgs, slaveMsgs);

  if (!td.nResult)
    fb.Info(1) << "Successfully sent and received " << td.nNumRepetitions
               << " messages in " << ((td.order == TThreadData::bulk) ? "bulk" : "sequence")
               << " order.";
  return 0;

}


#define CHECK(a) if(a) { cerr << "The operation " #a " failed.\n"; return 1; }
#define CHECKNEQ(a, b) if(a == b) { cerr << "The operation " #a " failed and returned " #b ".\n"; return 1; }

int main(int argc, char *argv[])
{
      // TODO: Get number of threads from command line.

  if (argc != 1 && argc < 3)
  { 
    cerr << "Usage: " << argv[0] << " [num_threads num_reps [info-level] ]\n"
         << "Spawn num_threads threads in parent and child, each sending and "
         << "receiving num_reps messages.  Default 1 for both.  "
         << "Optionally specify info level as third argument.  Default is " 
         << default_info_level << ".\n";
    return 1;
  }

  int nNumThreads = argc > 2 ? atoi(argv[1]) : 1;
  int nNumReps    = argc > 2 ? atoi(argv[2]) : 1;
  int nInfoLevel  = argc > 3 ? atoi(argv[3]) : default_info_level;

  FeedbackCentral::Instance().SetInfoLevel(nInfoLevel);
  
  int nPipeOut[2], nPipeIn[2];
  CHECK(pipe(nPipeOut));
  CHECK(pipe(nPipeIn));

  pid_t pidChild;
  CHECKNEQ((pidChild = fork()), -1);

  string sMode;

  if (pidChild)
  {
        // This is the parent/master
    CHECKNEQ(dup2(nPipeIn[0], 0), -1);
    CHECKNEQ(dup2(nPipeOut[1], 1), -1);
    sMode = "Master";
  }
  else
  {
        // This is the child/slave
    CHECKNEQ(dup2(1, 2), -1);
    CHECKNEQ(dup2(nPipeOut[0], 0), -1);
    CHECKNEQ(dup2(nPipeIn[1], 1), -1);
    sMode = "Slave";
  }

      // Check parent-child communication
  cout << "test_iocomm_msg_from_" << sMode << "\n" << flush;
  string sCheck;
  cin >> sCheck;
  cerr << sMode << " read " << sCheck << " from stdin...\n";
  cout << sCheck << endl << flush;
  cin >> sCheck;
  if (sCheck != "test_iocomm_msg_from_"+sMode)
  {
    cerr << "Stdio check failed in " << sMode << ".\n";
    return 1;
  }

  Feedback fb("Messages-tester");
  fb.RegisterThreadDescription("main");

  fb.Info(0) << sMode << " passed forking stage and I/O check, ready to begin testing!\n";

  fb.Info(0) << "About to create " << nNumThreads << " thread(s), each running " 
             << nNumReps << " repetition(s) of the test.\n\n";

  if (MessageRouter::Instance().StartReceiver())
    return 1;
  
      // Create threads and run tests
  vector<TThreadData> tds(nNumThreads);
  vector<pthread_t> tids(nNumThreads);
  for (int nThr = 0; nThr < nNumThreads; nThr++)
  {
    TThreadData &td = tds[nThr];
    td.sMode = sMode;
    stringstream ssServer;
    ssServer << "server_" << nThr;
    td.sServer = ssServer.str();
    td.nNumRepetitions = nNumReps;
    td.order = (nThr % 2) ? TThreadData::sequence : TThreadData::bulk;
    if (pthread_create(&(tids[nThr]), 0, thread_func, &(tds[nThr])))
    {
      fb.Info(0) << sMode << " failed to create thread number " << nThr << "!\n";
      return 1;
    }
    fb.Info(0) << sMode << " created thread number " << nThr << " (id " << tids[nThr] << ").\n";
  }

      // Join threads and summarize results.
  int nNumFailures = 0; 
  for  (int nThr = 0; nThr < nNumThreads; nThr++)
  {
    if (pthread_join(tids[nThr], 0))
    {
      fb.Info(0) << sMode << "failed to join thread number " << nThr << "!\n";
      return 1;
    }
    if (tds[nThr].nResult)
      nNumFailures++;
  }

  fb.Info(0) << sMode << " completed testing with " << nNumFailures << " failures.\n";
  return 0;
}
