/********************************************************************
 *   		producer_consumer.cpp
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
 *   See header file for description.
 *******************************************************************/

#include <simdist/producer_consumer.h>

#include <simdist/options.h>

#include <iostream>

#include <sys/types.h>
#include <unistd.h>

using namespace std;

Producer::Producer()
    : m_fb("Producer")
{
}


void
Producer::DeclareUserOptions() const
{
  Options::Instance().AppendStandard(Options::seed);
  Options::Instance().AppendStandard(Options::optionfile);
  Options::Instance().AppendStandard(Options::verbose);
  Options::Instance().AppendStandard(Options::help);
  Options::Instance().AppendStandard(Options::info_level);

      // Options normally take arguments OptionType("Description", mandatory (bool)?, default_value, short_option_char)
  // Options::Instance().Append("bool-option", new OptionBool("", false, false, 'b'));
  // Options::Instance().Append("string-option", new OptionString("", false, "", 's'));
  // Options::Instance().Append("float-option", new OptionFloat("", false, 0.0, 'f'));
  // Options::Instance().Append("int-option", new OptionInt("", false, 0, 'i'));
      // List opts take args OptionType("Desc", mandatory?, defvals_vector, min_elems, max_elems (0=infty), short_opt)
  // Options::Instance().Append("int-list-option", new OptionInts("", false, std::vector<int>(), 0, 0, 'l'));
}


int
Producer::GetUserOptions()
{
  bool bHelp;
  Options::Instance().Option("help", bHelp);
  if (bHelp)
  {
    cout << Options::Instance().PrettyPrint() << "\n";
    return 1;
  }

  if (Options::Instance().Option("verbose", m_bVerbose))
    return m_fb.Error(E_INIT_OPTIONS) << ": failed to get verbose option.";

  int nInfoLevel;
  if (Options::Instance().Option("info-level", nInfoLevel))
    return m_fb.Error(E_INIT_OPTIONS) << ": failed to get info-level option.";
  FeedbackCentral::Instance().SetInfoLevel(nInfoLevel);

  int nSeed = 0;
  if (Options::Instance().Option("seed", nSeed))
    return m_fb.Error(E_INIT_OPTIONS) << ": failed to get random seed.";
  srand(nSeed);
  srand48(nSeed);
  std::cerr << "Using random seed " << nSeed << ".\n";

  return 0;
}


bool
Producer::Verbose() const
{
  return m_bVerbose;
}


int
main(int argc, char *argv[])
{
  Producer ProducerVar;

  ProducerVar.DeclareUserOptions();
  if (int nRet = Options::Instance().ReadWithCommandLine(&argc, &argv))
    return nRet;
  if (int nRet = ProducerVar.GetUserOptions())
    return nRet;

  std::cerr << "Current settings are as follows:\n"
            << Options::Instance().PrintValues()
            << "\nPid is " << getpid() << ".\n";

  return 0;
}
