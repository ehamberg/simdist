/********************************************************************
 *   		errorcodes_common.cpp
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

#include <simdist/errorcodes_common.h>

// FeedbackError E_INTERNAL_LOGIC("Internal logical error");
DEFINE_FEEDBACK_ERROR(E_INTERNAL_LOGIC, "Internal logical error")
// FeedbackError E_INIT_OPTIONS("Failed to initialize correctly from user-supplied options");
DEFINE_FEEDBACK_ERROR(E_INIT_OPTIONS, "Failed to initialize correctly from user-supplied options")

DEFINE_FEEDBACK_ERROR(E_READ_ERROR, "An error occurred while reading data")
DEFINE_FEEDBACK_ERROR(E_WRITE_ERROR, "An error occurred while writing data")
