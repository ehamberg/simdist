/********************************************************************
 *   		producer_consumer.h
 *   Created on Fri Apr 17 2009 by Boye A. Hoeverstad.
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
 *   Test program for create
 *******************************************************************/

#if !defined(__PRODUCER_CONSUMER_H__)
#define __PRODUCER_CONSUMER_H__

#include "feedback.h"

class Producer
{
  Feedback m_fb;
  bool m_bVerbose;
 public:
  Producer();
  void DeclareUserOptions() const;
  int GetUserOptions();
  bool Verbose() const;
};

#endif
