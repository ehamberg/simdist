/********************************************************************
 *   		timer.cpp
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
 *   See header file for description.
 *******************************************************************/

#include <simdist/timer.h>
#include <iostream>
#include <sstream>

Timer::Timer(bool bReportOnDestroy /*=true*/, std::ostream *pReportStream /*=&std::cerr*/)
    : m_bReportOnDestroy(bReportOnDestroy)
    , m_pReportStream(pReportStream)
{
  Reset();
}

  
Timer::~Timer()
{
  if (m_bReportOnDestroy && m_pReportStream)
    PrintReport(*m_pReportStream);
}

  
void
Timer::ReportOnDestroy(bool bReport, std::ostream *pReportStream /*=&std::cerr*/)
{
  m_bReportOnDestroy = bReport;
  m_pReportStream = pReportStream;
}


void
Timer::Reset()
{
  time(&m_timeStart);
}


double
Timer::Elapsed() const
{
  time_t timeNow;
  time(&timeNow);
  return difftime(timeNow, m_timeStart);
}


std::string
Timer::CreateReport() const
{
  double dTimeElapsed = Elapsed(), dTimeElapsedKeep = dTimeElapsed;
  std::stringstream s;
  s << "Total simulation time: ";
  if (dTimeElapsed > 3600)
  {
    int nHrs = static_cast<int>(dTimeElapsed) / 3600;
    s << nHrs << "h";
    dTimeElapsed -= 3600 * nHrs;
  }
  if (dTimeElapsed > 60)
  {
    int nMins = static_cast<int>(dTimeElapsed) / 60;
    s << nMins << "m";
    dTimeElapsed -= 60 * nMins;
  }
  s << dTimeElapsed << "s";
  if (dTimeElapsedKeep > 60)
    s << " (=" << dTimeElapsedKeep << "s)";

  return s.str();
}
  

std::ostream&
Timer::PrintReport(std::ostream &s) const
{
  s << CreateReport() << "\n";
  return s;
}
