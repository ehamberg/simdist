/********************************************************************
 *   		test_options.cpp
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
 *   Test program for the options parsing class.
 *******************************************************************/

#include <simdist/options.h>

#include <iostream>
#include <iterator>
#include <fstream>
#include <cstring>

DEFINE_FEEDBACK_ERROR(E_TESTOPTIONS_FILE, "Reading options file failed");
DEFINE_FEEDBACK_ERROR(E_TESTOPTIONS_CMDLINE, "Reading command line or options file failed");



using namespace std;

int main(int argc, char *argv[])
{
  static const string process_name = "masterstub";

  Feedback fb("Options test program");

  Options::Instance().AppendStandard(Options::verbose);

  Options::Instance().Append("info-level", new OptionInt("Level of verbosity for informational output", false, 2, 'i'));
  Options::Instance().Append("master-program", new OptionString("The name of the process to be loaded on the master side", true, "", 'm'));
  Options::Instance().Append("master-arguments", new OptionString("Arguments sent to master process", false, "", 'a'));
  Options::Instance().Append("slave-program", new OptionString("The name of the process to be loaded on the slave side", true, "", 's'));
  Options::Instance().Append("slave-arguments", new OptionString("Arguments sent to the slave process", false, ""));

  if (argc == 2 && (!strcasecmp(argv[1], "-?") || !strcasecmp(argv[1], "--help") || !strcasecmp(argv[1], "-h")))
  {
    cerr << "Usage: \n\t" << argv[0] << " config-file\n"
         << "--- or ---\n\t"
         << argv[0] << " [options ] [--file config-file]\n"
         << "Where options can be given on the command line or in \"config-file\", written as lines of key:value pairs.  "
         << "The available options are as follows:\n"
         << Options::Instance().PrettyPrint() << "\n";
    return 1;
  }

  cerr << "Before reading options file.  Current options are:\n"
       << Options::Instance().PrettyPrint() << "\n";

  cerr << "Ignoring (skipping) unrecognized options.\n";
  Options::Instance().IgnoreUnrecognized(true);

  if (argc == 2)
  {
    ifstream ifOptions(argv[1]);
    if (!ifOptions)
    {
      cerr << "Failed to open config-file " << argv[1] << " for reading.  Run " 
           << argv[0] << " --help for further options.";
      return 1;
    }
    if (Options::Instance().Read(ifOptions))
      return fb.Error(E_TESTOPTIONS_FILE);
  }
  else
  {
    if (Options::Instance().ReadWithCommandLine(&argc, &argv, "file"))
      return fb.Error(E_TESTOPTIONS_CMDLINE);
    stringstream ss;
    copy(argv, argv + argc, ostream_iterator<char*>(ss, " "));
    fb.Info(0) << argc << " words in remaining command line: "
               << ss.str();

  }

  cerr << "\nOptions read.  Current options are:\n"
       << Options::Instance().PrettyPrint() << "\n\n";

  int nInfoLevel;
  if (Options::Instance().Option("info-level", nInfoLevel))
    fb.Warning("Failed to get option info-level.\n");
  else
    fb.Info(1) << "info-level is \"" << nInfoLevel << "\".";

  string sMaster, sMasterArgs, sSlave, sSlaveArgs;
  if (Options::Instance().Option("master-program", sMaster))
    fb.Warning("Failed to get option master-program.");
  else
    fb.Info(1) << "master-program is \"" << sMaster << "\".";

  if (Options::Instance().Option("master-arguments", sMasterArgs))
    fb.Warning("Failed to get option master-arguments.");
  else
    fb.Info(1) << "master-arguments is \"" << sMasterArgs << "\".";

  if (Options::Instance().Option("slave-program", sSlave))
    fb.Warning("Failed to get option slave-program.");
  else
    fb.Info(1) << "slave-program is \"" << sSlave << "\".";

  if (Options::Instance().Option("slave-arguments", sSlaveArgs))
    fb.Warning("Failed to get option slave-arguments.");
  else
    fb.Info(1) << "slave-arguments is \"" << sSlaveArgs << "\".";

  return 0;
}
