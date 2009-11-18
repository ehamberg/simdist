/********************************************************************
 *   		mathutils.h
 *   Created on Fri Sep 08 2006 by Boye A. Hoeverstad.
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
 *   Mathematics utilities
 *******************************************************************/

#if !defined(__MATHUTILS_H__)
#define __MATHUTILS_H__

#include <cstdlib>
#include <cmath>
#include <functional>
#include <numeric>
#include <iterator>
#include <algorithm>


/********************************************************************
 *  Return a random value in [0, nRange), using a method
 *  suggested by Numerical Recipes in C: The Art of Scientific
 *  Computing (William H. Press, Brian P. Flannery, Saul
 *  A. Teukolsky, William T. Vetterling; New York: Cambridge
 *  University Press, 1992 (2nd ed., p. 277)), via the Linux
 *  rand() man page:
 *
 *    "If you want to generate a random integer between 1 and 10,
 *    you should always do it by using high-order bits, as in
 *      j = 1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0)));
 *    and never by anything resembling
 *      j = 1 + (rand() % 10);
 *    (which uses lower-order bits)."
 *
 *  Interestingly, the SGI STL has a comment in stl_algo.h saying
 *  that lrand48 is superior to rand().  However, the linux
 *  lrand48 man page claims rand() to supercede lrand48.
 *******************************************************************/
inline int my_rand(int nRange)
{
  return static_cast<int>(nRange * (rand() / (RAND_MAX + 1.0)));
}


inline double rndCauchy(double wtrange) 
{
  const double Cauchy_cut = 10.0;
  double u = drand48();
 
  while (u == 0.5) 
    u = drand48();
  u = wtrange * tan(u * M_PI);
  if(fabs(u) > Cauchy_cut)
    return rndCauchy(wtrange);
  else
    return u;
}


/********************************************************************
 *   Return a random number from a uniform distribution over
 *   [dCenter - dRange/2, dCenter + dRange / 2).
 *******************************************************************/
struct RandomUniform
{
  double m_dCenter, m_dRange;
  RandomUniform(double dRange, double dCenter = 0)
      : m_dCenter(dCenter), m_dRange(dRange)
  {
  }

  double operator() () const
  {
    return (drand48() - 0.5) * m_dRange + m_dCenter;
  }
};


/********************************************************************
 *   Return a random integer from a uniform distribution over
 *   [nOffset, nOffset + nRange);
 *******************************************************************/
struct RandomUniformInt
{
  int m_nOffset, m_nRange;
  RandomUniformInt(int nRange, int nOffset = 0)
      : m_nOffset(nOffset), m_nRange(nRange)
  {
  }

  int operator() () const
  {
    return m_nOffset + my_rand(m_nRange);;
  }
};


/********************************************************************
 *   Return a random number from a Cauchy distribution with range
 *   dRange and center dCenter, such that 50% of the drawn values
 *   fall withing +/-(dRange + dCenter).
 *******************************************************************/
struct RandomCauchy : public RandomUniform
{
  const double m_dCauchyCut;
  RandomCauchy(double dRange, double dCenter = 0)
      : RandomUniform(dRange, dCenter), m_dCauchyCut(10.0)
  {
  }

  double operator() () const
  {
    double u = drand48();
 
    while (u == 0.5) 
      u = drand48();
    u = m_dRange * tan(u * M_PI);
    if(fabs(u) > m_dCauchyCut)
      return operator()();
    else
      return u + m_dCenter;
  }
};


/********************************************************************
 *   Return a random number from a Gaussian distribution with stddev
 *   fStdDev and center fCenter.  This method is supposedly fast, BUT
 *   THE ALGORITHM _MUST_ BE SEEDED ONCE PER PROGRAM, by a call so the
 *   Seed member function.  Implemented in ziggurat.cpp.
 *******************************************************************/
struct RandomNormalZiggurat : public RandomUniform
{
  RandomNormalZiggurat(double dStdDev, double dCenter = 0)
      : RandomUniform(dStdDev, dCenter)
  {
  }

  double operator() () const throw(std::string);
  static void Seed(unsigned long nSeed);
};




template<class DistributionFunc>
struct NoiseAdder1 : public std::unary_function<double, double >
{
  DistributionFunc m_f;
  double m_dRange;
  NoiseAdder1(DistributionFunc f, double dRange)
      : m_f(f), m_dRange(dRange)
  {
  }

  double operator()(double dVal) const
  {
    return dVal + m_f(m_dRange);
  }
};


template<class DistributionFunc>
struct NoiseAdder : public std::unary_function<double, double >
{
  DistributionFunc m_f;
  NoiseAdder()
      : m_f(RandomUniform(1))
  {
  }

  NoiseAdder(DistributionFunc f)
      : m_f(f)
  {
  }

  double operator()(double dVal) const
  {
    return dVal + m_f();
  }
};


template<class DistributionFunc>
inline NoiseAdder1<DistributionFunc> 
make_NoiseAdder(DistributionFunc f, double dRange)
{
  return NoiseAdder1<DistributionFunc>(f, dRange);
}

template<class DistributionFunc>
inline NoiseAdder<DistributionFunc> 
make_NoiseAdder(DistributionFunc f)
{
  return NoiseAdder<DistributionFunc>(f);
}



/********************************************************************
 *   Utility functor which returns a continuously increasing
 *   value.  To be used for instance when initializing index
 *   vectors.
 *******************************************************************/
template<class T = size_t>
struct sequence
{
  T m_nCur;
  const T m_nStep;
  sequence(T nInit = 0, T nStep = 1)
      : m_nCur(nInit), m_nStep(nStep)
  {
  }

  T operator() ()
  {
    T nRet = m_nCur;
    m_nCur += m_nStep;
    return nRet;
  }
};




/********************************************************************
 *   Random sample function based on SGI extension to STL.  
 *
 *   Starts by filling the target sequence with the first
 *   elements from the source sequence.  Then, if the source
 *   sequence contains more elements than the target sequence, a
 *   random position is chosen for each element in the remainder
 *   of the source.  If this position is inside the target
 *   sequence, the current element is overwritten.
 *******************************************************************/
template <class InputIter, class RandomAccessIter, class Distance>
RandomAccessIter
my_random_sample(InputIter first, InputIter last,
                 RandomAccessIter out_first, Distance n)
{
  Distance m = 0;
  Distance t = n;
  for ( ; first != last && m < n; ++m, ++first) 
    out_first[m] = *first;

  while (first != last) {
    ++t;
    Distance M = my_rand(t);
    if (M < n)
      out_first[M] = *first;
    ++first;
  }

  return out_first + m;
}

template <class InputIter, class RandomAccessIter>
RandomAccessIter
my_random_sample(InputIter first, InputIter last,
                 RandomAccessIter out_first, RandomAccessIter out_last)
{
  return my_random_sample(first, last, out_first, out_last - out_first);
}


/********************************************************************
 *   random_sample_n function based on SGI extension to STL.  
 *
 *   The main difference from random_sample is that this routine
 *   maintains the relative order of the sampled elements.
 *******************************************************************/
template <class ForwardIter, class OutputIter, class Distance>
OutputIter my_random_sample_n(ForwardIter first, ForwardIter last,
                              OutputIter out_first, Distance n)
{
  Distance remaining = std::distance(first, last);
  Distance m = min(n, remaining);

  while (m > 0) {
    if (my_rand(remaining) < m) {
      *out_first = *first;
      ++out_first;
      --m;
    }

    --remaining;
    ++first;
  }
  return out_first;
}

template <class ForwardIter, class OutputIter>
OutputIter my_random_sample_n(ForwardIter first, ForwardIter last,
                              OutputIter out_first, OutputIter out_last)
{
  return my_random_sample_n(first, last, out_first, out_last - out_first);
}



/********************************************************************
 ********************************************************************
 *   Compose functions based on Josuttis, "The C++ Standard Library",
 *   1999 (p 314 and onwards).
 ********************************************************************
 *******************************************************************/

/********************************************************************
 *   Basic compose function.
 *******************************************************************/
template <class OP1, class OP2>
class compose_f_gx_t : public std::unary_function<typename OP2::argument_type,
                                                  typename OP1::result_type>
{
private:
  OP1 m_op1;
  OP2 m_op2;
public:
  explicit compose_f_gx_t(const OP1& op1, const OP2 &op2)
      : m_op1(op1), m_op2(op2)
  {
  }

  typename OP1::result_type
  operator() (const typename OP2::argument_type& x) const
  {
    return m_op1(m_op2(x));
  }
};


/********************************************************************
 *   Convenience function for compose_f_gx_t.
 *******************************************************************/
template <class OP1, class OP2>
inline compose_f_gx_t<OP1, OP2>
compose_f_gx(const OP1& op1, const OP2 &op2)
{
  return compose_f_gx_t<OP1, OP2>(op1, op2);
}

/********************************************************************
 *   Compose f(g(h(x)))
 *******************************************************************/
template <class OP1, class OP2, class OP3>
class compose_f_g_hx_t : public std::unary_function<typename OP3::argument_type,
                                                  typename OP1::result_type>
{
private:
  OP1 m_op1;
  OP2 m_op2;
  OP3 m_op3;
public:
  explicit compose_f_g_hx_t(const OP1& op1, const OP2 &op2, const OP3 &op3)
      : m_op1(op1), m_op2(op2), m_op3(op3)
  {
  }

  typename OP1::result_type
  operator() (const typename OP3::argument_type& x) const
  {
    return m_op1(m_op2(m_op3(x)));
  }
};


/********************************************************************
 *   Convenience function for compose_f_g_hx_t.
 *******************************************************************/
template <class OP1, class OP2, class OP3>
inline compose_f_g_hx_t<OP1, OP2, OP3>
compose_f_g_hx(const OP1& op1, const OP2 &op2, const OP3 &op3)
{
  return compose_f_g_hx_t<OP1, OP2, OP3>(op1, op2, op3);
}

/********************************************************************
 *   Compose where f takes g(x, y).
 *******************************************************************/

template <class OP1, class OP2>
class compose_f_gxy_t : public std::binary_function<typename OP2::first_argument_type,
                                                   typename OP2::second_argument_type,
                                                  typename OP1::result_type>
{
private:
  OP1 m_op1;
  OP2 m_op2;
public:
  explicit compose_f_gxy_t(const OP1& op1, const OP2 &op2)
      : m_op1(op1), m_op2(op2)
  {
  }

  typename OP1::result_type
  operator() (const typename OP2::first_argument_type& x, const typename OP2::second_argument_type& y) const
  {
    return m_op1(m_op2(x, y));
  }
};


/********************************************************************
 *   Convenience function for compose_f_gx_t.
 *******************************************************************/
template <class OP1, class OP2>
inline compose_f_gxy_t<OP1, OP2>
compose_f_gxy(const OP1& op1, const OP2 &op2)
{
  return compose_f_gxy_t<OP1, OP2>(op1, op2);
}


/********************************************************************
 *   Compose where f takes g(x) and h(x) as arguments.  Use to
 *   formulate stuff like "x < 5 or x > 10".
 *******************************************************************/
template <class OP1, class OP2, class OP3>
class compose_f_gx_hx_t : public std::unary_function<typename OP2::argument_type, 
                                                     typename OP1::result_type>
{
private:
  OP1 m_op1;
  OP2 m_op2;
  OP3 m_op3;
public:
  explicit compose_f_gx_hx_t(const OP1& op1, const OP2 &op2, const OP3 &op3)
      : m_op1(op1), m_op2(op2), m_op3(op3)
  {
  }

  typename OP1::result_type
  operator() (const typename OP2::argument_type& x) const
  {
    return m_op1(m_op2(x), m_op3(x));
  }
};


/********************************************************************
 *   Convenience function.
 *******************************************************************/
template <class OP1, class OP2, class OP3>
inline compose_f_gx_hx_t<OP1, OP2, OP3>
compose_f_gx_hx(const OP1& op1, const OP2 &op2, const OP3 &op3)
{
  return compose_f_gx_hx_t<OP1, OP2, OP3>(op1, op2, op3);
}



/********************************************************************
 *   Compose where f takes g(x) and h(y) as arguments.
 *******************************************************************/
template <class OP1, class OP2, class OP3>
class compose_f_gx_hy_t : public std::binary_function<typename OP2::argument_type, 
                                                      typename OP3::argument_type, 
                                                      typename OP1::result_type>
{
private:
  OP1 m_op1;
  OP2 m_op2;
  OP3 m_op3;
public:
  explicit compose_f_gx_hy_t(const OP1& op1, const OP2 &op2, const OP3 &op3)
      : m_op1(op1), m_op2(op2), m_op3(op3)
  {
  }

  typename OP1::result_type
  operator() (const typename OP2::argument_type& x,
              const typename OP3::argument_type& y) const
  {
    return m_op1(m_op2(x), m_op3(y));
  }
};


/********************************************************************
 *   Convenience function.
 *******************************************************************/
template <class OP1, class OP2, class OP3>
inline compose_f_gx_hy_t<OP1, OP2, OP3>
compose_f_gx_hy(const OP1& op1, const OP2 &op2, const OP3 &op3)
{
  return compose_f_gx_hy_t<OP1, OP2, OP3>(op1, op2, op3);
}

/********************************************************************
 *   End of compose functions from Josuttis.
 *******************************************************************/


/********************************************************************
 *   Identity function adaptor.  Part of SGI STL, but not in the
 *   standard.
 *******************************************************************/
template <class T>
struct identity : public std::unary_function<T, T>
{
  const T& operator() (const T& t) const
  {
    return t;
  }

  T& operator() (T& t) const
  {
    return t;
  }
};


/********************************************************************
 *   Normalize a sequence
 *******************************************************************/
template<class Iterator>
void 
inline Normalize(Iterator itBegin, Iterator itEnd)
{
  typedef typename Iterator::value_type T;
  if (itBegin == itEnd)
    return;

  T tMin = *std::min_element(itBegin, itEnd);
  T tScale = *std::max_element(itBegin, itEnd) - tMin;
  if (tScale != 0)
    std::transform(itBegin, itEnd, itBegin, compose_f_gx(std::bind2nd(std::divides<T>(), tScale),
                                                         std::bind2nd(std::minus<T>(), tMin)));
  else
    std::fill(itBegin, itEnd, T(0));
}


/********************************************************************
 *   Similar to for_each, but iterates through two sequences
 *   simultaneously.  Also similar to transform, but usable when
 *   there is no assignment to be performed, but rather for
 *   instance sequence modification or data gathering.
 *******************************************************************/
template <class Iterator1, class Iterator2, class BinaryFunction>
BinaryFunction
inline for_both(Iterator1 first1, Iterator1 last1, 
                        Iterator2 first2, BinaryFunction f)
{
  while (first1 != last1)
    f(*first1++, *first2++);
  return f;
}

/********************************************************************
 *   Struct and convenience function to calculate the average of
 *   a sequence.  Use the convenience function for normal
 *   average.  Use this struct directly if you want to calculate
 *   average with another type than the value type of the
 *   sequence (typically calc'ing the avg of an integer
 *   sequence).
 *
 *   The struct is not public std::binary_function because it's
 *   nice to only have to specify the desired return type, not
 *   both return type and argument types.
 *******************************************************************/
template<class T = float>
struct Average_t 
{
  template<class Iterator>
  T operator() (Iterator itBegin, Iterator itEnd)
  {
    return std::accumulate(itBegin, itEnd, static_cast<T>(0)) / std::distance(itBegin, itEnd);
  }
};


/********************************************************************
 *   Default convenience function; previously, we used the value
 *   type of the sequence when calculating average, but this only
 *   makes sense for floating point sequences.
 *******************************************************************/
template<class Iterator>
float
inline Average(Iterator itBegin, Iterator itEnd)
{
  return Average_t<float>()(itBegin, itEnd);
}


/********************************************************************
 *   Calculate standard deviation of a sequence, given the
 *   average.  Here there is apparently no reason to use the
 *   struct directly, as there was for the average, since the
 *   value type is determined by the type of the average.
 *******************************************************************/
template<class Iterator, class T>
struct StdDev_t
{
  T operator() (Iterator itBegin, Iterator itEnd, T avg) const
  {
        // Make choice of pow overload explicit, otherwise some
        // compilers will complain no matching function for call
        // to `ptr_fun(<unknown type>)'
    double (*mypow)(double, double) = pow;    
    return static_cast<T>(sqrt(std::accumulate(itBegin, itEnd, static_cast<T>(0.0), 
                                               compose_f_gx_hy(std::plus<T>(), 
                                                               identity<T>(),
                                                               compose_f_gx(std::bind2nd(std::ptr_fun(mypow), static_cast<T>(2.0)), 
                                                                            std::bind2nd(std::minus<T>(), avg))))
                               / std::distance(itBegin, itEnd)));
                          }
  };


  template<class Iterator, class T>
  T
  inline StdDev(Iterator itBegin, Iterator itEnd, T avg)
  {
    return StdDev_t<Iterator, T>()(itBegin, itEnd, avg);
  }

#endif
