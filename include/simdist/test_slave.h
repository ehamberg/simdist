/********************************************************************
 *   		test_slave.h
 *   Created on Tue May 23 2006 by Boye A. Hoeverstad.
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
 *   Test code for the slave process.  At the moment it simply returns
 *   the input multiplied with the startup argument after a random
 *   amount of time.  Normally the process will wait a few seconds
 *   before returning, but with with a certain probability it will wait considerably longer.  The possibly long wait is supposed to imitate a crash,
 *   an infinite loop or something similar.
 *******************************************************************/

#if !defined(__TEST_SLAVE_H__)
#define __TEST_SLAVE_H__


#endif
