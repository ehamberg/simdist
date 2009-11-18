/********************************************************************
 *   		errorcodes_common.h
 *   Created on Wed Apr 05 2006 by Boye A. Hoeverstad.
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
 *   Common error codes
 *******************************************************************/

#if !defined(__ERRORCODES_COMMON_H__)
#define __ERRORCODES_COMMON_H__

#include "feedback.h"

// extern FeedbackError E_INTERNAL_LOGIC;
DECLARE_FEEDBACK_ERROR(E_INTERNAL_LOGIC)
// extern FeedbackError E_INIT_OPTIONS;
DECLARE_FEEDBACK_ERROR(E_INIT_OPTIONS)
DECLARE_FEEDBACK_ERROR(E_READ_ERROR)
DECLARE_FEEDBACK_ERROR(E_WRITE_ERROR)
#endif
