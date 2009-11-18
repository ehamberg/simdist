/********************************************************************
 *   		test_mathutils.cpp
 *   Created on Thu Jan 11 2007 by Boye A. Hoeverstad.
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
 *   Test mathutils.
 *******************************************************************/

#include <simdist/mathutils.h>
#include <simdist/misc_utils.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <functional>
#include <numeric>
#include <algorithm>

using namespace std;

int
main(int argc, char *argv[])
{

      // Check that the two versions of Cauchy distrib give the
      // same result.
  const double dRange = 3.5, dCenter = -7;
  const size_t nNumSamples = 100000;
  vector<double> r1(nNumSamples, dCenter), r2(nNumSamples, 0), 
    r3(nNumSamples, 0), r4(nNumSamples, 0), rDiff(nNumSamples);

  int nSeed = time(0);
  
  srand48(nSeed);
  std::transform(r1.begin(), r1.end(), r1.begin(), make_NoiseAdder(rndCauchy, dRange));
//   cout << "Voila " << nNumSamples << " cauchy doubles (function) with center " 
//        << dCenter << " and range " << dRange << ":\n";
//   copy(r1.begin(), r1.end(), ostream_iterator<double>(cout, "\n"));

  srand48(nSeed);
  generate(r2.begin(), r2.end(), RandomCauchy(dRange, dCenter));
//   cout << "Voila " << nNumSamples << " cauchy doubles (struct) with center " 
//        << dCenter << " and range " << dRange << ":\n";
//   copy(r2.begin(), r2.end(), ostream_iterator<double>(cout, "\n"));

//   RandomNormalZiggurat::Seed(nSeed);
//   generate(r3.begin(), r3.end(), RandomNormalZiggurat(dRange, dCenter));
//   cout << "Voila " << nNumSamples << " normal doubles (struct) with center " 
//        << dCenter << " and range " << dRange << ":\n";
//   copy(r3.begin(), r3.end(), ostream_iterator<double>(cout, "\n"));

  generate(r4.begin(), r4.end(), RandomUniform(dRange, dCenter));
//   cout << "Voila " << nNumSamples << " uniform doubles (struct) with center " 
//        << dCenter << " and range " << dRange << ":\n";
//   copy(r4.begin(), r4.end(), ostream_iterator<double>(cout, "\n"));

      // Calc diff
  std::transform(r1.begin(), r1.end(), r2.begin(), rDiff.begin(), std::minus<double>());
      // Abs the differences
  
      // Need to be explicit about which type of fabs to use.
  double (*myfabs)(double) = fabs;
  std::transform(rDiff.begin(), rDiff.end(), rDiff.begin(), myfabs);

      // Sum the abs of the diffs
//   cout << "r1 contents: ";
//   copy(r1.begin(), r1.end(), ostream_iterator<double>(cout, " "));
//   cout << "\nr2 contents: ";
//   copy(r1.begin(), r1.end(), ostream_iterator<double>(cout, " "));
//   cout << "\n\n";

  double dTotalDiff = std::accumulate(rDiff.begin(), rDiff.end(), 0.0);
  if (dTotalDiff != 0)
  {
    cerr << "The difference between the two is: " << dTotalDiff << ". Should be 0.\n";
    return 1;
  }

//   copy(r3.begin(), r3.end(), ostream_iterator<double>(cout, "\n"));
  
//   double (*mypow)(double, double) = pow;
  double dAvg = Average(r3.begin(), r3.end());
  double dStdDev = StdDev(r3.begin(), r3.end(), dAvg);
  float fAvg = Average_t<float>()(r3.begin(), r3.end());
  float fStdDev = StdDev(r3.begin(), r3.end(), fAvg);

  cout << "Average in r3 is double " << dAvg << ", float " << fAvg << " (should be " << dCenter 
       << "), stddev is double " << dStdDev << ", float " << fStdDev << " (should be " << dRange << ").\n";


//   double (* noise)(double);
// // NoiseAdder<RandomUniform> noise;
//   NoiseAdder<RandomUniform> na = make_NoiseAdder(RandomUniform(1));
//   noise = na.operator();
//   noise = make_NoiseAdder(RandomCauchy(1));

  return 0;
}
