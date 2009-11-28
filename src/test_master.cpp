/********************************************************************
 *   		test_master.cpp
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
 *   See header file for description.
 *******************************************************************/

#include "../config.h"
#include <simdist/test_master.h>

#include <simdist/misc_utils.h>

#include <cassert>
#include <vector>
#include <iostream>
#include <list>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <getopt.h>

using namespace std;

static list<int> signals;
static int nNumLines = 10;
static int nFactor = 7;
static int nNumIterations = 50;

typedef enum { eof, simple } eIOMode;
static eIOMode inputMode = simple;
static eIOMode outputMode = simple;

static int
parse_arguments(int argc, char *argv[])
{
  struct option opts[] = {
    {"lines"           , required_argument, 0, 'l'},
    {"factor"          , required_argument, 0, 'f'},
    {"iterations"      , required_argument, 0, 'i'},
    {"job-input-mode"  , required_argument, 0, 'j'},
    {"job-output-mode" , required_argument, 0, 'o'},
    {"signal"          , required_argument, 0, 's'},
    { 0 }};

      // Adding a + to the opstring to avoid permuting the option string.
  int ch;
  while ((ch = getopt_long(argc, argv, "+l:f:j:o:i:s:", opts, 0)) != -1)
  {
    switch (ch)
    {
        case 'l':
          nNumLines = atoi(optarg);
          break;
        case 'f':
          nFactor = atoi(optarg);
          break;
        case 'i':
          nNumIterations = atoi(optarg);
          break;
        case 'j':
          if (!strcasecmp(optarg, "eof"))
            inputMode = eof;
          else if (!strcasecmp(optarg, "simple"))
            inputMode = simple;
          else
            cerr << "Unrecognized input mode: " << optarg << ". Ignored.\n";
          break;
        case 'o':
          if (!strcasecmp(optarg, "eof"))
            outputMode = eof;
          else if (!strcasecmp(optarg, "simple"))
            outputMode = simple;
          else
            cerr << "Unrecognized output mode: " << optarg << ". Ignored.\n";
          break;
        case 's':
            signals.push_back(atoi(optarg));
          break;
//         case 'v':
//           bVerbose = true;
//           break;
        default:
          std::cerr << "Unregonized option: " << static_cast<char>(optopt) << ".\n";
          return 1;
    }
  }
  return 0;
}

static void
SignalIgnorer(int nSig)
{
  cerr << "Master: Signal " << nSig << " received and ignored.\n";
}

int main(int argc, char *argv[])
{
  if (parse_arguments(argc, argv))
  {
    cerr << "Usage: " << argv[0] << " [options]\n"
         << "Write lines of numbers on separate lines to stdout.  "
         << "Expect them returned multiplied by factor on stdin.  "
         << "Repeat the process for a given number of times.  \n\nOptions:\n"
         << "--lines=n | -l n   Write n lines of numbers at each iteration. Default " << nNumLines << ".\n"
         << "--factor=k | -f k  Expect each line to be multiplied by k on return. Default " << nFactor << ".\n"
         << "--iterations=n | -i n \n"
         << "                   Repeat the write/read process n times. Default " << nNumIterations << ".\n"
         << "--job-input-mode=mode | -j mode \n"
         << "                   Expect input of type mode, either 'eof' or 'simple'.\n"
         << "--job-output-mode=mode | -o mode \n"
         << "                   Write output of type mode, either 'eof' or 'simple'.\n"
         << "--signal=n | -s n  Report and ignore signal n.\n";
    return 1;
  }

  for (list<int>::const_iterator itsig = signals.begin(); itsig != signals.end(); itsig++)
  {
    cerr << "Master: Will ignore signal " << *itsig << " if received...\n";
    Signal(*itsig, SignalIgnorer);
  }

  cerr << "Master now writing " << nNumLines << " lines to std out. Expecting them returned multiplied by " 
       << nFactor << ".  Will repeat the process " << nNumIterations << " times...\n";
  
  srand(time(0));
  for (int nIter = 0; nIter < nNumIterations; nIter++)
  {
    std::vector<int> numbers(nNumLines);
    for (int nLine = 0; nLine < nNumLines && cout; nLine++)
    {
      if (outputMode == eof)
        cout << "EOF\n";
      else 
        assert(outputMode == simple);

      //numbers[nLine] = rand();
      numbers[nLine] = nIter*nNumLines + nLine;
      cout << numbers[nLine] << "\n" << flush;
//       cerr << "Wrote " << numbers[nLine] << ".\n" << flush;
//       sleep(1);

      if (outputMode == eof)
        cout << "EOF\n" << flush;
      else 
        assert(outputMode == simple);
    }
    cout << flush;
    if (!cout)
    {
      cerr << "Standard output failed.  Aborting.\n";
      return 3;
    }

    for (int nLine = 0; nLine < nNumLines && cin; nLine++)
    {
      int nRes, nExpected = numbers[nLine] * nFactor;

      string sEOF1, sEOF2, sChomp;
      if (inputMode == eof)
        getline(cin, sEOF1);
      else 
        assert(inputMode == simple);

      cin >> nRes;
      getline(cin, sChomp);

      if (inputMode == eof)
      {
        getline(cin, sEOF2);
        if (sEOF1 != sEOF2)
        {
          cerr << "Read non-matching EOF marks. Mark 1: " << sEOF1 << ", mark 2: " << sEOF2 << ".\n";
          exit(1);
        }
      }
      else 
        assert(inputMode == simple);

      if (nRes != nExpected)
      {
        cerr << "Error on line " << nLine + 1 << " of iteration " << nIter + 1 << "!  "
             << "Expected " << nExpected << " (" << numbers[nLine] << "*" 
             << nFactor << "), got " << nRes << "!  Continuing.\n";
        //return 2;
      }
    }
    if (!cin)
    {
      cerr << "Standard input failed.  Aborting.\n";
      return 3;
    }
    cerr << "Iteration " << nIter + 1 << ": " << nNumLines << " calculations completed successfully.\n";
  }
  cerr << "All " << nNumIterations << " iterations completed. " << nNumLines * nNumIterations << " results successfully returned.\n";
  return 0;
}
