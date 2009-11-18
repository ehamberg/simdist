/********************************************************************
 *   		pipeio_common.h
 *   Created on Tue Nov 04 2008 by Boye A. Hoeverstad.
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
 *   Routines common to pipeio and pipeio4.
 *******************************************************************/

#if !defined(__PIPEIO_COMMON_H__)
#define __PIPEIO_COMMON_H__

#include <simdist/misc_utils.h>

bool CheckArgs(int argc, char *argv[]);
pid_t Connect(int nArg, char *argv[], 
              fdistream *pfReadOut, fdostream *pfWriteIn);


#endif
