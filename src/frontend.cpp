/********************************************************************
 *   		frontend.cpp
 *   Created on Mon May 22 2006 by Boye A. Hoeverstad.
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
 *   Frontend to the distribution module, for use with MPI distributed
 *   processing.  Initializes MPI, and then loads either the master or
 *   the slave module, depending on the process rank.
 *******************************************************************/

#if HAVE_CONFIG_H
#include "../config.h"
#endif

// slave_mpi.h must be included before stdio. See comment in slave_mpi.h
#include <simdist/slave_mpi.h>
#include <simdist/master.h>
#include <simdist/slave.h>
#include <simdist/options.h>

#include <simdist/master_stdio.h>
#include <simdist/slave_stdio.h>

#include <mpi.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iterator>

using namespace std;

class MPILibWrapper
{
public:
  MPILibWrapper(int *argc, char ***argv)
  {
    MPI_Init(argc, argv);
  }

  ~MPILibWrapper()
  {
    MPI_Finalize();
  }
};


int
InitOptions(int argc, char *argv[])
{
  static const string process_name = argv[0]; //"masterstub";

  Options::Instance().AppendStandard(Options::optionfile);

  Options::Instance().Append("verbosity", new OptionInt("Level of verbosity for informational output from the master", false, 0, 'v'));
  Options::Instance().Append("slave-verbosity", new OptionInt("Level of verbosity for informational output from the slaves", false, 0));
  Options::Instance().Append("master-program", new OptionString("The name of the process to be loaded on the master side", false, ""));
  Options::Instance().Append("master-arguments", new OptionString("Arguments sent to master process", false, ""));
  Options::Instance().Append("master", new OptionString("The name of and arguments to the process to be loaded on the master side, i.e. a concatenation of master-program and master-arguments", false, "", 'm'));
  Options::Instance().Append("slave-program", new OptionString("The name of the process to be loaded on the slave side", false, ""));
  Options::Instance().Append("slave-arguments", new OptionString("Arguments sent to the slave process", false, "", 'b'));
  Options::Instance().Append("slave", new OptionString("The name of and arguments to the process to be loaded on the slave side, i.e. a concatenation of slave-program and slave-arguments", false, "", 's'));
  Options::Instance().Append("slave-run-once", new OptionBool("The slave process must be killed and reloaded for each new evaluation (true/false).", false, false));
  Options::Instance().Append("master-input-mode", new OptionString("How the master expects its input formatted.  Available values are SIMPLE [lines], EOF, BIN-EOF [bytes] and BYTES", false, "SIMPLE"));
  Options::Instance().Append("master-output-mode", new OptionString("Similar to master-input-mode", false, "SIMPLE"));
//   Options::Instance().Append("slave-count", new OptionInt("The number of slaves to spawn", false, 1));
  Options::Instance().Append("slave-wait-factor", new OptionFloat("How long a slave waits before taking a job already taken by another slave", false, 10));
  Options::Instance().Append("jobs-per-send", new OptionInt("How many free jobs each slave will take from the queue at once (0 = auto)", false, 0));
  Options::Instance().Append("verbosity-showonly", new OptionString("If not empty, only verbose output from modules in this comma-separated list will be printed", false, ""));
  Options::Instance().Append("verbosity-dontshow", new OptionString("If not empty, verbose output from modules in this comma-separated list will never be printed", false, ""));
  Options::Instance().Append("slave-id-tag", new OptionString("When the argument to this option is found in the list of slave process arguments, its value will be replaced with a unique identifier on each slave server.", false, "SLAVEID"));
  Options::Instance().Append("report-total-simulation-time", new OptionBool("Report total simulation time (in real time) when the distribution system shuts down (true/false).", false, false));

//   Options::Instance().Append("slave-expects-id", new OptionBool("", false, "false"));

  if (argc == 1 || (argc == 2 && (!strcasecmp(argv[1], "-?") || !strcasecmp(argv[1], "--help") || !strcasecmp(argv[1], "-h"))))
  {
    cerr << "Usage: \n\t" << process_name << " config-file\n"
         << "--- or ---\n\t"
         << process_name << " [options ] [--options options-file]\n"
         << "Where options can be given on the command line or in \"options-file\", written as lines of key:value pairs.  "
         << "The available options are as follows:\n"
         << Options::Instance().PrettyPrint() << "\n";
    return 1;
  }

      // Only one arguments: Assume it is name of config file.
  if (argc == 2)
  {
    ifstream ifOptions(argv[1]);
    if (!ifOptions)
    {
      cerr << "Failed to open options-file " << argv[1] << " for reading.  Run " 
           << process_name << " --help for further options.\n";
      return 1;
    }
    return Options::Instance().Read(ifOptions);
  }
  else
  {
    if (int nRet = Options::Instance().ReadWithCommandLine(&argc, &argv))
      return nRet;
    if (argc > 1)
    {
      cerr << "Warning: " << argc - 1 << " unrecognized option(s) on the command line "
           << "(not counting program name (argv[0]): ";
      copy(argv, argv + argc, ostream_iterator<char*>(cerr, " "));
      cerr << "\n";
    }
    return 0;
  }
}


int
main(int argc, char *argv[])
{
#if HAVE_STD__IOS__SYNC_WITH_STDIO
      // Disable c++ stl synchronization with C standard i/o
      // routines.  This presumably improves I/O performance (ref
      // libstdc++-v3 HOWTO, chapter 27: Input/Output;
      // http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#8;
      // and configure.ac).
  std::ios::sync_with_stdio(false);
#endif

  MPILibWrapper w(&argc, &argv);
  int nRank, nRet = MPI_Comm_rank(MPI_COMM_WORLD, &nRank);
  if (nRet != MPI_SUCCESS)
  {
    std::cerr << "Failed to get MPI rank (MPI_Comm_rank did not return MPI_SUCCESS).  Abort.\n";
    return 1;
  }

  if ((nRet = InitOptions(argc, argv)))
    return nRet;

  if (nRank == 0)
    return master_main(argc, argv);
  else
    return slave_main(argc, argv);
}
