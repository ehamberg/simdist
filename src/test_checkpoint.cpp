/********************************************************************
 *   		test_checkpoint.cpp
 *   Created on Mon Apr 28 2008 by Boye A. Hoeverstad.
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
 *   Test checkpointing.
 *******************************************************************/

#include <simdist/io_utils.h>

#include <simdist/options.h>
#include <simdist/feedback.h>

#include <iostream>

#include <time.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

class CheckTester
{
  Feedback m_fb;
  bool m_bVerbose, m_bBinaryIO;
  Checkpointer m_check;
 public:
  CheckTester();

  int Run();
  void DeclareUserOptions() const;
  int GetUserOptions();
  bool Verbose() const;
};

CheckTester::CheckTester()
    : m_fb("CheckTester")
{
}


void
CheckTester::DeclareUserOptions() const
{
  Options::Instance().AppendStandard(Options::seed);
  Options::Instance().AppendStandard(Options::optionfile);
  Options::Instance().AppendStandard(Options::verbose);
  Options::Instance().AppendStandard(Options::help);
  Options::Instance().AppendStandard(Options::info_level);

  Options::Instance().Append("check-file", new OptionString("Name of checkpoint file, if any", true, ""));
  Options::Instance().Append("binary-io", new OptionBool("Use binary I/O?", false, false));

      // Options normally take arguments OptionType("Description", mandatory (bool)?, default_value, short_option_char)
  // Options::Instance().Append("bool-option", new OptionBool("", false, false, 'b'));
  // Options::Instance().Append("string-option", new OptionString("", false, "", 's'));
  // Options::Instance().Append("float-option", new OptionFloat("", false, 0.0, 'f'));
  // Options::Instance().Append("int-option", new OptionInt("", false, 0, 'i'));
      // List opts take args OptionType("Desc", mandatory?, defvals_vector, min_elems, max_elems (0=infty), short_opt)
  // Options::Instance().Append("int-list-option", new OptionInts("", false, std::vector<int>(), 0, 0, 'l'));
}


int
CheckTester::GetUserOptions()
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
  if (Options::Instance().Option("binary-io", m_bBinaryIO))
    return m_fb.Error(E_INIT_OPTIONS) << ": failed to get binary-io option.";

  string sFilename;
  if (Options::Instance().Option("check-file", sFilename))
    return m_fb.Error(E_INIT_OPTIONS) << ": failed to get check-file option.";
  m_check.SetFilename(sFilename);

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
CheckTester::Verbose() const
{
  return m_bVerbose;
}

int
CheckTester::Run()
{
  int nCtr1 = 0, nCtr2 = 0;
  string sCheck;
  if (!m_check.Load(sCheck))
  {
    cout << "Loading from checkpoint.\n";
    stringstream ss(sCheck);
    if (m_bBinaryIO)
    {
      ss.read(reinterpret_cast<char*>(&nCtr1), sizeof(nCtr1));
      ss.read(reinterpret_cast<char*>(&nCtr2), sizeof(nCtr2));
    }
    else
      ss >> nCtr1 >> nCtr2;
  }
  else
    cout << "NOT loading from checkpoint.\n";

  struct timespec sleepTime;
  sleepTime.tv_sec = 0;
  sleepTime.tv_nsec = static_cast<long>(1E9 / 10);
  
  bool bContinue = true;
  while (bContinue)
  {
    nCtr1 += 1;
    nCtr2 += 2;
    cout << "Currently at " << nCtr1 << ", " << nCtr2
         << ". Please kill me at any time.  Sleeping..." << flush;
    nanosleep(&sleepTime, 0);
    cout << "slept.\n" << flush;
    if (!(nCtr1 % 10))
    {
      cout << "Storing checkpoint data..." << flush;
      stringstream ss;
      if (m_bBinaryIO)
      {
        ss.write(reinterpret_cast<char*>(&nCtr1), sizeof(nCtr1));
        ss.write(reinterpret_cast<char*>(&nCtr2), sizeof(nCtr2));
      }
      else
        ss << nCtr1 << " "<< nCtr2;
      cout << "done. Checkpointing..." << flush;
      if (int nRet = m_check.Store(ss.str()))
      {
        cerr << "Failed to store checkpoint!\n";
        return nRet;
      }
      cout << "checkpoint stored.\n" << flush;
    }

    if (!(nCtr1 % 200))
    {
      cout << nCtr1 << " steps have passed. Continue (or clear checkpoint and exit)?";
      char c;
      cin >> c;
      if (c == 'y')
        continue;
      m_check.Clear();
      break;
    }
  }
  return 0;
}


int
main(int argc, char *argv[])
{
  CheckTester tester;

  tester.DeclareUserOptions();
  if (int nRet = Options::Instance().ReadWithCommandLine(&argc, &argv))
    return nRet;
  if (int nRet = tester.GetUserOptions())
    return nRet;

  std::cerr << "Current settings are as follows:\n"
            << Options::Instance().PrintValues()
            << "\nPid is " << getpid() << ".\n";

  return tester.Run();
}
