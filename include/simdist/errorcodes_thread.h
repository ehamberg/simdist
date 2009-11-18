/********************************************************************
 *   		errorcodes_thread.h
 *   Created on Wed Mar 22 2006 by Boye A. Hoeverstad.
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
 *   Error codes relevant for multithreaded and synchronized
 *   development.
 *******************************************************************/

#if !defined(__ERRORCODES_THREAD_H__)
#define __ERRORCODES_THREAD_H__

#include "feedback.h"

// extern FeedbackError E_COND_BROADCAST;
DECLARE_FEEDBACK_ERROR(E_COND_BROADCAST)
// extern FeedbackError E_COND_DESTROY;
DECLARE_FEEDBACK_ERROR(E_COND_DESTROY)
// extern FeedbackError E_COND_INIT;
DECLARE_FEEDBACK_ERROR(E_COND_INIT)
// extern FeedbackError E_COND_SIGNAL;
DECLARE_FEEDBACK_ERROR(E_COND_SIGNAL)
// extern FeedbackError E_COND_WAIT;
DECLARE_FEEDBACK_ERROR(E_COND_WAIT)
// extern FeedbackError E_COND_TIMEDWAIT;
DECLARE_FEEDBACK_ERROR(E_COND_TIMEDWAIT)

// extern FeedbackError E_MUTEX_DESTROY;
DECLARE_FEEDBACK_ERROR(E_MUTEX_DESTROY)
// extern FeedbackError E_MUTEX_INIT;
DECLARE_FEEDBACK_ERROR(E_MUTEX_INIT)
// extern FeedbackError E_MUTEX_LOCK;
DECLARE_FEEDBACK_ERROR(E_MUTEX_LOCK)
// extern FeedbackError E_MUTEX_UNLOCK;
DECLARE_FEEDBACK_ERROR(E_MUTEX_UNLOCK)
// extern FeedbackError E_MUTEX_UNLOCK_TWICE;
DECLARE_FEEDBACK_ERROR(E_MUTEX_UNLOCK_TWICE)

// extern FeedbackError E_THREAD_CREATE;
DECLARE_FEEDBACK_ERROR(E_THREAD_CREATE)

// extern FeedbackError E_THREAD_CREATE_EAGAIN;
DECLARE_FEEDBACK_ERROR(E_THREAD_CREATE_EAGAIN)
// extern FeedbackError E_THREAD_CREATE_EINVAL;
DECLARE_FEEDBACK_ERROR(E_THREAD_CREATE_EINVAL)
// extern FeedbackError E_THREAD_CREATE_EPERM;
DECLARE_FEEDBACK_ERROR(E_THREAD_CREATE_EPERM)

// extern FeedbackError E_CLIENT_SEND, "Failed to ");
// extern FeedbackError E_CLIENT_TAKE, "Failed to ");
// extern FeedbackError E_PVMCONTROL, "Failed to ");
// extern FeedbackError E_PVM_BUFINFO, "Failed to ");
// extern FeedbackError E_PVM_INIT, "Failed to ");
// extern FeedbackError E_PVM_INITSEND, "Failed to ");
// extern FeedbackError E_PVM_PACK, "Failed to ");
// extern FeedbackError E_PVM_RECV, "Failed to ");
// extern FeedbackError E_PVM_SEND, "Failed to ");
// extern FeedbackError E_PVM_SLAVEEXISTS, "Failed to ");
// extern FeedbackError E_PVM_UNPACK, "Failed to ");
// extern FeedbackError E_SET_ABORTED, "Failed to ");
// extern FeedbackError E_SET_RESULTS, "Failed to ");
// extern FeedbackError E_SLAVE_ABORT, "Failed to ");
// extern FeedbackError E_SLAVE_JOBNOTFOUND, "Failed to ");
// extern FeedbackError E_SLAVE_NORESULTS, "Failed to ");
// extern FeedbackError E_SLAVE_RECEIVE, "Failed to ");
// extern FeedbackError E_STATE, "Failed to ");
// extern FeedbackError E_STATE_INVALID, "Failed to ");
// extern FeedbackError E_SLAVETYPE_INVALID, "Failed to ");
// extern FeedbackError E_PVM_MCAST, "Failed to ");
// extern FeedbackError E_SERVER_ABORT, "Failed to ");
// extern FeedbackError E_SERVER_CANCEL, "Failed to ");
// extern FeedbackError E_SERVER_CREATE, "Failed to ");
// extern FeedbackError E_SERVER_EXISTS, "Failed to ");
// extern FeedbackError E_SERVER_JOIN, "Failed to ");
// extern FeedbackError E_SERVER_KILL, "Failed to ");
// extern FeedbackError E_SERVER_LOCK, "Failed to ");
// extern FeedbackError E_SERVER_RECV, "Failed to ");
// extern FeedbackError E_SERVER_SETJOB, "Failed to ");
// extern FeedbackError E_QUEUE_SIGNAL, "Failed to ");
// extern FeedbackError E_QUEUE_WAIT, "Failed to ");
// extern FeedbackError E_MASTER_DUPLICATERESULTS, "Failed to ");
// extern FeedbackError E_MASTER_EVALAGAIN, "Failed to ");
// extern FeedbackError E_PVM_SPAWN, "Failed to ");
// extern FeedbackError E_SLAVE_WAIT, "Failed to ");
// extern FeedbackError E_SLAVES_CREATE, "Failed to ");
// extern FeedbackError E_PVMCONTROL_START, "Failed to ");
// extern FeedbackError E_SLAVE_INITTWICE, "Failed to ");
// extern FeedbackError E_SERVER_NOTREADY, "Failed to ");
// extern FeedbackError E_SET_READY, "Failed to ");
// extern FeedbackError E_EVALPROG_OPEN, "Failed to ");
// extern FeedbackError E_EVALPROG_WRITE, "Failed to ");
// extern FeedbackError E_EVALPROG_READ, "Failed to ");

#endif
