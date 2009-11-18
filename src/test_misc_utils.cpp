/********************************************************************
 *   		test_misc_utils.cpp
 *   Created on Tue Jan 23 2007 by Boye A. Hoeverstad.
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
 *   Various tests for the utilities library.
 *******************************************************************/

#include <simdist/misc_utils.h>
#include <simdist/mathutils.h>
#include <simdist/ref_ptr.h>
#include <getopt.h>

#include <list>

#include <cmath>
#include <iostream>
#include <functional>
#include <iterator>
#include <algorithm>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/timeb.h>
#include <cstring>

using namespace std;

bool bVerbose = false;
bool bChild = false;
int nSeed;

int
parse_arguments(int argc, char *argv[])
{
  struct option opts[] = {
//     {"arg"      , required_argument, 0, 'a'},
//     {"noarg"    , no_argument,       0, 'n'},
//     {"opt"      , optional_argument, 0, 'o'},
    {"child"    , no_argument,       0, 'c'},
    {"verbose"  , no_argument,       0, 'v'},
    {"seed"    , required_argument,       0, 's'},
    { 0 }};

      // Adding a + to the opstring to avoid permuting the option string.
  int ch;
  while ((ch = getopt_long(argc, argv, "+cs:v", opts, 0)) != -1)
  {
    switch (ch)
    {
        case 'c':
          bChild = true;
          break;
        case 's':
          nSeed = atoi(optarg);
          break;
        case 'v':
          bVerbose = true;
          break;
        default:
          std::cerr << "Unregonized option: " << static_cast<char>(optopt) << ".\n";
          return 1;
    }
  }
  return 0;
}


/********************************************************************
 *   Support class for functional tests below.
 *******************************************************************/
class MyInt
{
  int m_n;
public:
  explicit MyInt(int n)
      : m_n(n)
  {
  }

  operator int () const
  {
    return m_n;
  }
  
  float Divide(float fDiv) const
  {
    return m_n / fDiv;
  }

  float RefDivide(const float &fDiv) const
  {
    return m_n / fDiv;
  }

};



/********************************************************************
 *   Test "functional" utils: ref_ptr::GetRef(), compose, etc.
 *******************************************************************/
int
TestFunctional()
{
  vector<ref_ptr<MyInt> > v;
  for (int i = 0; i < 10; i++)
    v.push_back(ref_ptr<MyInt>(new MyInt(i)));
 
      // Test GetRef.
  cout << "The values in v: ";
  transform(v.begin(), v.end(), ostream_iterator<int>(cout, " "), mem_fun_ref(&ref_ptr<MyInt>::GetRef));
  cout << ".\nThe sum is " << accumulate(v.begin(), v.end(), 0, compose_f_gx_hy(plus<int>(), identity<int>(), mem_fun_ref(&ref_ptr<MyInt>::GetRef))) << ".\n";

      // Now we want to bind after GetRef, and then compose.
  cout << "The squares of the values in v: ";
  float (*mypow)(float, int) = pow;
  transform(v.begin(), v.end(), ostream_iterator<float>(cout, " "), compose_f_gx(bind2nd(ptr_fun(mypow), 2), mem_fun_ref(&ref_ptr<MyInt>::GetRef)));
  cout << ".\nThe sum of the squares is " << accumulate(v.begin(), v.end(), 0.0f, compose_f_gx_hy(plus<float>(), identity<float>(), 
                                                                                                  compose_f_gx(bind2nd(ptr_fun(mypow), 2), 
                                                                                                               mem_fun_ref(&ref_ptr<MyInt>::GetRef)))) << ".\n";

      // Finally, call a member function on the object contained in
      // the ref_ptr.  When accumulating, call the function taking a
      // reference as argument, using mem_fun_unref to take away the
      // reference.
  cout << "The squares of a third of the values in v: ";
  transform(v.begin(), v.end(), ostream_iterator<float>(cout, " "), compose_f_g_hx(bind2nd(ptr_fun(mypow), 2), 
                                                                                   bind2nd(mem_fun_ref(&MyInt::Divide), 3.0f), 
                                                                                   mem_fun_ref(&ref_ptr<MyInt>::GetRef)));
  cout << ".\nThe sum of the squares of the third is " 
       << accumulate(v.begin(), v.end(), 0.0f, compose_f_gx_hy(plus<float>(), identity<float>(), 
                                                               compose_f_g_hx(bind2nd(ptr_fun(mypow), 2), 
                                                                              bind2nd(mem_fun_ref_unref(&MyInt::RefDivide), 3.0f), 
                                                                              mem_fun_ref(&ref_ptr<MyInt>::GetRef)))) << ".\n";
  return 0;
}



/********************************************************************
 *   Test CreateArgumentVector().  
 *
 *   Comment out when compiling with -D_GLIBCXX_DEBUG,
 *   CreateArgumentVector won't link correctly (see Makefile.am)
 *******************************************************************/
int
TestArgVec()
{
  typedef vector<string> TAV;
  TAV argLines;
  
  argLines.push_back("exec -o opt abc");
  argLines.push_back("exec -o \"opt 1 2\" abc");
  argLines.push_back("exec -o \"opt '1 3 5' 2\" abc");
  argLines.push_back("exec -o \"opt ''1 3 5' 2\" \"abc 123\"");
  argLines.push_back("exec -o \"opt ''1 3 5' 2\"\"'abc 123\"");
  argLines.push_back("exec -o \"\" \"opt ''1 3 5' 2\"\"'abc 123\"");
  argLines.push_back("exec -o \"\" \"opt ''\"1 3 5' 2\"\"'abc 123\"");

  for (size_t nArg = 0; nArg < argLines.size(); nArg++)
  {
    string sArg = argLines[nArg];
    vector<string> sArgs;
    if (!CreateArgumentVector(sArg, sArgs))
    {
      cout << "Arguments successfully parsed:\n\t" << sArg << "\t=>\t";
      copy(sArgs.begin(), sArgs.end(), ostream_iterator<string>(cout, " || "));
      cout << "\n\n";
    }
    else
    {
      cerr << "Failed to parse argument line number " << nArg << " (0-based)!\n";
      return 1;
    }
  }
//   cerr << "TestArgVec() commented out...\n";
  return 0;
}


int
TestFdStreams(int argc, char *argv[])
{
  const int nNumGens = 100;
  if (bChild)
  {
    for (int nGen = 0; nGen < nNumGens; nGen++)
    {
      stringstream ssBatchEOF; 
      ssBatchEOF << "BATCH-EOF-" << rand();
      int nPop = my_rand(5000);
      vector<int> v(nPop), vin(nPop);
      generate(v.begin(), v.end(), rand);
      v[my_rand(v.size())] = 0;
      v[my_rand(v.size())] = EOF;
      cout << ssBatchEOF.str() << "\n";
      if (nGen % 2)
        copy(v.begin(), v.end(), ostream_iterator<int>(cout, "\n"));
      else
      {
        size_t nSize = v.size();
        cout.write(reinterpret_cast<char*>(&nSize), sizeof(size_t));
        for (size_t nIdx = 0; nIdx < v.size(); nIdx++)
        {
          cout.write(reinterpret_cast<char*>(&v[nIdx]), sizeof(int));
        }
        cout << "\n";
      }
      cout << ssBatchEOF.str() << "\n" << flush;

      for_each(vin.begin(), vin.end(), stream_in<int>(cin));

      for (int nInd = 0; nInd < nPop; nInd++)
        if (v[nInd] * 2 != vin[nInd])
        {
          cerr << "Child: Error in generation " << nGen << ", individual " << nInd
               << "! Expected " << v[nInd] * 2 << ", got " << vin[nInd] << ".\n";
          return 1;
        }
    }
    cerr << "Child read and wrote all generations successfully.\n";
  }
  else
  {
    fdostream myssErr(2);
    myssErr << "This is a write to stderr through fdostream.\n";
//           << "It may screw up the internal workings of stderr, "
//           << "so it might be a good idea to call this late in the program.\n"; 


    fdostream childWrite;
    fdistream childRead;
    stringstream ssArg;
    ssArg << "--child --seed " << nSeed;
    if (ConnectProcess(argv[0], ssArg.str(), &childWrite, &childRead, 0, 0, 0))
    {
      cerr << "Failed to connect to " << argv[0] << " child.\n";
      return 1;
    }
    for (int nGen = 0; nGen < nNumGens; nGen++)
    {
      string sEOF, sLine, sData;
      vector<int> v;
      getline(childRead, sEOF);
      while(getline(childRead, sLine) && sLine != sEOF)
        sData += sLine + "\n";

      stringstream ss(sData);
      if (nGen % 2)
      {
        int n;
        while (ss >> n)
          v.push_back(n*2);
      }
      else
      {
        size_t nSize;
        ss.read(reinterpret_cast<char*>(&nSize), sizeof(nSize));
        v.resize(nSize);
        ss.read(reinterpret_cast<char*>(&v[0]), v.size()*sizeof(int));
        transform(v.begin(), v.end(), v.begin(), bind2nd(multiplies<int>(), 2));
      }

      if (sLine != sEOF)
      {
        cerr << "Parent: Failed to read trailing EOF mark in generation " 
             << nGen << ". (searched for " << sEOF << ").\n";
        return 1;
      }

      copy(v.begin(), v.end(), ostream_iterator<int>(childWrite, "\n"));
      childWrite.flush();
    }
    cerr << "Parent read and wrote all generations successfully.\n";

//     for (int nL = 0; nL < 10; nL+=2)
//     {
//       childWrite << "Line " << nL << "\nLine " << nL+1 << "\n" << flush;
//       int nCtr;
//       if (nL % 4)
//       {
//         string sL;
//         childRead >> nCtr;
//         getline(childRead, sL); // Chomp newline 
//         cerr << "Parent (op>>): Child has confirmed " << nCtr << " lines.\n";
//       }
//       else if (nL % 3)
//       {
//         string sL;
//         getline(childRead, sL);
//         cerr << "Parent (getline): Child has confirmed " << sL << " lines.\n";
//       }
//       else
//       {
//         const int nSize = 2;
//         char b[nSize];
//         childRead.read(b, nSize);
//         b[childRead.gcount() - 1] = 0;
//         cerr << "Parent (read): Child has confirmed " << b << " lines.\n";
//       }
//     }
    childWrite.close();
    childRead.close();
  }
  return 0;
}


struct unlinker
{
  int m_nFd;
  string m_sFname;
  unlinker(int nFd, const string &sFname)
      : m_nFd(nFd), m_sFname(sFname)
  {}
  ~unlinker()
  {
    if (m_nFd > 0)
      close(m_nFd);
    if (!m_sFname.empty())
    {
      if (unlink(m_sFname.c_str()))
        cerr << "Failed to unlink " << m_sFname << ". System error message: "
             << strerror(errno) << "\n";
      else
        cerr << "Unlinked " << m_sFname << ".\n";
    }
  }
};

int
TestFdStreams2()
{
  char ofname[] = "./__misc_utils_testfile.XXXXXX";
  int nFdOut = mkstemp(ofname);
  if (nFdOut < 0)
  {
    cerr << "Failed to create temporary file!\nSystem error message: " 
         << strerror(errno) << "\n";
    return 1;
  }

  fdostream fdsOut(nFdOut);
  unlinker unl(nFdOut, ofname);

  const int nCount = my_rand(25000), nCharCount = nCount * sizeof(int);
  cerr << "Testing fdstreams with a file of " << nCharCount << " bytes...\n";
  vector<int> v(nCount);
  generate(v.begin(), v.end(), rand);
  fdsOut.write(reinterpret_cast<char*>(&(v[0])), nCharCount);

  fdsOut.close();
  
  int nFdIn = open(ofname, O_RDONLY);
  if (nFdIn < 0)
  {
    cerr << "Failed to open temporary file for reading!\nSystem error message: " 
         << strerror(errno) << "\n";
    return 1;
  }

  fdistream fdsIn(nFdIn);
  unl.m_nFd = nFdIn;

  vector<char> v2(nCharCount);
  int nPutbackTotal = 0, nPutbackDebt = 0;
  for (int nIdx = 0; nIdx < static_cast<int>(v2.size()); )
  {
    int nRead = min(my_rand(50), nCharCount - nIdx);
    fdsIn.read(&(v2[0]) + nIdx, nRead);
    for (int nCheck = nIdx; nCheck < nIdx + nRead; nCheck++)
      if (v2[nCheck] != reinterpret_cast<char*>(&v[0])[nCheck])
      {
        cerr << "Error in input after reading " << nCheck << " bytes!\n";
        return 1;
      }

    nIdx += nRead;
    nPutbackDebt = max(0, nPutbackDebt - nRead);
    int nPutback = min(my_rand(fdiobuf::putbacksize + 1 - nPutbackDebt), nIdx);
    nPutbackDebt += nPutback;
    nPutbackTotal += nPutback;
    for (int nPb = 0; nPb < nPutback; nPb++, nIdx--)
    {
      if (nPutback % 2)
        fdsIn.unget();
      else
        fdsIn.putback(v2[nIdx-1]);
      if (!fdsIn.good())
      {
        cerr << "Error after putting back character number " << nIdx-1 << " (number " 
             << nPb << " in this round of putback, total put back is " 
             << nPutbackTotal << "). Put back with " 
             << (nPutback % 2 ? "unget()" : "putback()") << ".\n";
        return 1;
      }
    }
  }

  cerr << "No errors detected during reading and putback.  Post checking the arrays...\n";
  for (int nIdx = 0; nIdx < static_cast<int>(v2.size()); nIdx++)
  {
    if (v2[nIdx] != reinterpret_cast<char*>(&v[0])[nIdx])
    {
      cerr << "Post read check error in position " << nIdx << "!\n";
      return 1;
    }
  }
 
  cerr << "Post read check passed!  All tests in TestFdStreams2() passed successfully.\n";
  return 0;
}


typedef struct TMatchTestVar
{
  string sW, sS;
  bool bM;
  TMatchTestVar(string w, string s, bool m)
      : sW(w), sS(s), bM(m) {}
} TMatchTest;

int TestWildcardMatch()
{
  list<TMatchTest> mlist;
  mlist.push_back(TMatchTest("SlaveClient*", "SlaveClient-server-1", true));
  mlist.push_back(TMatchTest("*SlaveClient*", "SlaveClient-server-1", true));
  mlist.push_back(TMatchTest("*Client*", "SlaveClient-server-1", true));
  mlist.push_back(TMatchTest("*server-1", "SlaveClient-server-1", true));

  for (list<TMatchTest>::const_iterator it = mlist.begin(); it != mlist.end(); it++)
  {
    bool bMatch = WildcardMatch(it->sW.c_str(), it->sS.c_str());
    cerr << "Does '" << it->sW << "' wildcard-match '" << it->sS << "'? "
         << (bMatch ? "yes" : "no") << ". Should it? " 
         << (bMatch == it->bM ? "yes" : "no") << "\n";
    if (bMatch != it->bM)
      return 1;
  }
  return 0;
}


int
TestTrim()
{
  const int num_strings = 7;
  string strings[num_strings];
  strings[0] = " hei";
  strings[1] = " hei ";
  strings[2] = "   \t hei alle sammen";
  strings[3] = "\t\t\t  hei alle sammen igjen    \t ";
  strings[4] = "";
  strings[5] = "      \t    ";
  strings[6] = "hei ";

  for (int s = 0; s < num_strings; s++)
    cerr << "'" << strings[s] << "' trims to '" << Trim(strings[s]) << "'.\n";
  cerr << "Trim test complete.\n\n";
  return 0;
}


int
main(int argc, char *argv[])
{
  struct timeb tp;
  ftime(&tp);
  nSeed = tp.time + tp.millitm;

  if (parse_arguments(argc, argv))
  {
    cerr << "Usage: " << argv[0] << " [--seed n] [--verbose | -v]\n";
    return 1;
  }
  
  srand(nSeed);
  cerr << "Random seed is " << nSeed << " .\n";

  if (bChild)
    return TestFdStreams(argc, argv);

  if (TestFunctional() ||
      TestArgVec() ||
      TestFdStreams(argc, argv) ||
      TestWildcardMatch() || 
      TestFdStreams2() ||
      TestTrim())
  {
    cerr << "One or more tests FAILED!\n";
    return 1;
  }

  cerr << "All tests completed successfully.\n";
  return 0;
}
