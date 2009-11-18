/********************************************************************
 *   		master_stdio.h
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
 *   TODO: Insert description here.
 *******************************************************************/

#if !defined(__MASTER_STDIO_H__)
#define __MASTER_STDIO_H__

#include "feedback.h"

// extern FeedbackError E_MASTERMAIN_LOOPSETUP;
DECLARE_FEEDBACK_ERROR(E_MASTERMAIN_LOOPSETUP)
// extern FeedbackError E_MASTERMAIN_LOOP;
DECLARE_FEEDBACK_ERROR(E_MASTERMAIN_LOOP)
// extern FeedbackError E_MASTERMAIN_SETUP;
DECLARE_FEEDBACK_ERROR(E_MASTERMAIN_SETUP)

 
int 
master_main(int argc, char *argv[]);

#endif
