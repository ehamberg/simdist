/********************************************************************
 *   		slave_stdio.h
 *   Created on Mon May 22 2006 by Boye A. Hoeverstad.
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
 *   Stub to launch the slave server.
 *
 *******************************************************************/

#if !defined(__SLAVE_STDIO_H__)
#define __SLAVE_STDIO_H__

#include "feedback.h"

// extern FeedbackError E_SLAVEMAIN_RECV;
DECLARE_FEEDBACK_ERROR(E_SLAVEMAIN_RECV)
// extern FeedbackError E_SLAVEMAIN_SEND;
DECLARE_FEEDBACK_ERROR(E_SLAVEMAIN_SEND)
// extern FeedbackError E_SLAVEMAIN_MSG;
DECLARE_FEEDBACK_ERROR(E_SLAVEMAIN_MSG)
// extern FeedbackError E_SLAVEMAIN_OPEN;
DECLARE_FEEDBACK_ERROR(E_SLAVEMAIN_OPEN)
// extern FeedbackError E_SLAVEMAIN_LAUNCH;
DECLARE_FEEDBACK_ERROR(E_SLAVEMAIN_LAUNCH)
// extern FeedbackError E_SLAVEMAIN_JOBSEND;
DECLARE_FEEDBACK_ERROR(E_SLAVEMAIN_JOBSEND)
// extern FeedbackError E_SLAVEMAIN_SETUP;
DECLARE_FEEDBACK_ERROR(E_SLAVEMAIN_SETUP)

int 
slave_main(int argc, char *argv[]);


#endif
