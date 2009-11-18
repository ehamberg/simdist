/********************************************************************
 *   		errorcodes_thread.cpp
 *   Created on Sun Aug 06 2006 by Boye A. Hoeverstad.
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

#include <simdist/errorcodes_thread.h>

// FeedbackError E_COND_BROADCAST("Failed to broadcast a condition (invalid condition variable?)");
DEFINE_FEEDBACK_ERROR(E_COND_BROADCAST, "Failed to broadcast a condition (invalid condition variable?)")
// FeedbackError E_COND_DESTROY("Failed to distroy a condition");
DEFINE_FEEDBACK_ERROR(E_COND_DESTROY, "Failed to distroy a condition")
// FeedbackError E_COND_INIT("Failed to initialize a condition");
DEFINE_FEEDBACK_ERROR(E_COND_INIT, "Failed to initialize a condition")
// FeedbackError E_COND_SIGNAL("Failed to signal a condition");
DEFINE_FEEDBACK_ERROR(E_COND_SIGNAL, "Failed to signal a condition")
// FeedbackError E_COND_WAIT("Failed to wait on a condition");
DEFINE_FEEDBACK_ERROR(E_COND_WAIT, "Failed to wait on a condition")
// FeedbackError E_COND_TIMEDWAIT("A timed wait on a condition failed");
DEFINE_FEEDBACK_ERROR(E_COND_TIMEDWAIT, "A timed wait on a condition failed")

// FeedbackError E_MUTEX_DESTROY("Failed to destroy a mutex");
DEFINE_FEEDBACK_ERROR(E_MUTEX_DESTROY, "Failed to destroy a mutex")
// FeedbackError E_MUTEX_INIT("Failed to initialize a mutex");
DEFINE_FEEDBACK_ERROR(E_MUTEX_INIT, "Failed to initialize a mutex")
// FeedbackError E_MUTEX_LOCK("Failed to lock a mutex");
DEFINE_FEEDBACK_ERROR(E_MUTEX_LOCK, "Failed to lock a mutex")
// FeedbackError E_MUTEX_UNLOCK("Failed to unlock a mutex");
DEFINE_FEEDBACK_ERROR(E_MUTEX_UNLOCK, "Failed to unlock a mutex")
// FeedbackError E_MUTEX_UNLOCK_TWICE("Attempted to unlock the same mutex several times");
DEFINE_FEEDBACK_ERROR(E_MUTEX_UNLOCK_TWICE, "Attempted to unlock the same mutex several times")

// FeedbackError E_THREAD_CREATE("Failed to create a thread");
DEFINE_FEEDBACK_ERROR(E_THREAD_CREATE, "Failed to create a thread")

// FeedbackError E_THREAD_CREATE_EAGAIN(E_THREAD_CREATE.Description() + ": Out of resources (EAGAIN)");
DEFINE_FEEDBACK_ERROR(E_THREAD_CREATE_EAGAIN, E_THREAD_CREATE().Description() + ": Out of resources (EAGAIN)")
// FeedbackError E_THREAD_CREATE_EINVAL(E_THREAD_CREATE.Description() + ": Invalid attributes (EINVAL)");
DEFINE_FEEDBACK_ERROR(E_THREAD_CREATE_EINVAL, E_THREAD_CREATE().Description() + ": Invalid attributes (EINVAL)")
// FeedbackError E_THREAD_CREATE_EPERM(E_THREAD_CREATE.Description() + ": Inappropriate permission (EPERM)");
DEFINE_FEEDBACK_ERROR(E_THREAD_CREATE_EPERM, E_THREAD_CREATE().Description() + ": Inappropriate permission (EPERM)")
