/********************************************************************
 *   		data_sampler.cpp
 *   Created on Sat Nov 03 2007 by Boye A. Hoeverstad.
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
 *   Create points sampled (normally) around given center points.
 *   Read input from stdin as <fX fY stddev class>.  For
 *   instance, the following file will yield XOR data points:
 *
        0       0       0
        1       0       1
        0       1       1
        1       1       0
 * 
 *
 *   If the above data is saved to data_points_xor.txt, usage may
 *   be as follows:
 *
 *   ./data-sampler -n 1000 --num-features=2 --num-classes=1 --stddev=0.3 <data_points_xor.txt |./plot-data-samples.sh -c 4 -k
 *
 *   Alternatively, a local representation of xor may be created as follows:
 *
        0       0       0       1
        1       0       1       0
        0       1       1       0
        1       1       0       1
 
 *   with the corresponding command (plotting doesn't work, though):
 *
 *   ./data-sampler -n 1000 --num-features=2 --num-classes=2 --stddev=0.3 <data_points_xor_local.txt
 *
 *******************************************************************/

#include <simdist/options.h>

#include <simdist/mathutils.h>
#include <simdist/misc_utils.h>

#include <iostream>
#include <cassert>


using namespace std;


typedef struct TPointVar
{
  float fX, fY, fStdDevX, fStdDevY, fClass;
} TPoint;


typedef std::pair<vector<float>, vector<float> > TDataline; 
typedef vector<TDataline> TDataset; 


int
ReadBasePoints(istream &s, TDataset &data, int nNumInputs, int nNumOutputs)
{
  while (s.good())
  {
    TDataline line;
    line.first.resize(nNumInputs);
    std::for_each(line.first.begin(), line.first.end(), stream_in<float>(s));
    line.second.resize(nNumOutputs);
    std::for_each(line.second.begin(), line.second.end(), stream_in<float>(s));
    if (s.good())
      data.push_back(line);
  }
  return 0;
}


int Sample(const TDataset &data, int nNumSamples, float fStdDev, ostream &s)
{
  RandomNormalZiggurat sampler(fStdDev);

  for (int nS = 0; nS < nNumSamples; nS++)
  {
    for (TDataset::const_iterator dit = data.begin(); dit != data.end(); dit++)
    {
      for (vector<float>::const_iterator lit = dit->first.begin(); lit != dit->first.end(); lit++)
        s << *lit + sampler() << "\t";
      copy(dit->second.begin(), dit->second.end(), ostream_iterator<float>(s, "\t"));
    }
    s << "\n";
  }
  s.flush();

  return 0;
}


int
main(int argc, char *argv[])
{
  Options::Instance().Append("num-samples", new OptionInt("The number of samples to create", false, 10, 'n'));
  Options::Instance().Append("num-features", new OptionInt("The number of features (= #elements in input vector = #input nodes in ANN)", false, 2, 'i'));
  Options::Instance().Append("num-classes", new OptionInt("The number of output classes (= #elements in output vector = #output nodes in ANN)", false, 1, 'o'));
  Options::Instance().Append("stddev", new OptionFloat("Standard deviation", false, 0.3, 'd'));
  Options::Instance().AppendStandard(Options::seed);
  Options::Instance().AppendStandard(Options::help);

  if (Options::Instance().ReadWithCommandLine(&argc, &argv))
  {
    cerr << "Failed to get user options.\n";
    return 1;
  }

  bool bHelp;
  Options::Instance().Option("help", bHelp);
  if (bHelp)
  {
    cout << Options::Instance().PrettyPrint() << "\n";;
    return 0;
  }

  int nSeed, nNumSamples, nNumInputs, nNumOutputs;
  float fStdDev;
  if (Options::Instance().Option("seed", nSeed)
      || Options::Instance().Option("num-samples", nNumSamples)
      || Options::Instance().Option("num-features", nNumInputs)
      || Options::Instance().Option("num-classes", nNumOutputs)
      || Options::Instance().Option("stddev", fStdDev))
  {
    cerr << "Failed to get user options.\n";
    return 1;
  }

  RandomNormalZiggurat::Seed(nSeed);
  cerr << "Using random seed " << nSeed << "\n";

  TDataset data;
  if (int nRes = ReadBasePoints(cin, data, nNumInputs, nNumOutputs))
  {
    cerr << "Failed to read base points.\n";
    return nRes;
  }

  if (int nRes = Sample(data, nNumSamples, fStdDev, cout))
  {
    cerr << "Failed to create data points.\n";
    return nRes;
  }

  cerr << nNumSamples * data.size() << " data points created in " 
       << data.size() << " columns by " << nNumSamples << " rows.\n";
  return 0;
}
