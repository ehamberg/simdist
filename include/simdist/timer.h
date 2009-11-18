/********************************************************************
 *   		timer.h
 *   Created on Fri May 09 2008 by Boye A. Hoeverstad.
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
 *   A simple class for reporting time spent during simulation.
 *******************************************************************/

#if !defined(__TIMER_H__)
#define __TIMER_H__

#include <iostream>

#include <sys/types.h>
#include <time.h>

class Timer
{
  bool m_bReportOnDestroy;
  time_t m_timeStart;
  std::ostream *m_pReportStream;
public:
  Timer(bool bReportOnDestroy = true, std::ostream *pReportStream = &std::cerr);
  ~Timer();
  
  void ReportOnDestroy(bool bReport, std::ostream *pReportStream = &std::cerr);
  void Reset();
  double Elapsed() const;
  std::string CreateReport() const;
  std::ostream& PrintReport(std::ostream &s) const;
};

#endif
