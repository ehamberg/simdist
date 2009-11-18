/********************************************************************
 *   		options.h
 *   Created on Fri Jul 05 2002 by Boye A. Hoeverstad.
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
 *   Parses options and settings for the program.  Takes as input two
 *   streams containing key-value lines, separated by a colon (:).
 *   Comments start with a pound sign (#).  The first input stream
 *   should contain all available options, with default values.  The
 *   second strem may contain user-specified option values which will
 *   overwrite the default values.
 *
 *******************************************************************/

#if !defined(__CONTROL_H__)
#define __CONTROL_H__

#include "feedback.h"
#include "ref_ptr.h"

#include <stdexcept>
#include <iosfwd>
#include <map>
#include <string>
#include <list>
#include <vector>

#if defined(DEBUG)
#include <set>
#endif

// extern FeedbackError E_OPTIONS_TYPE;
// extern FeedbackError E_OPTIONS_VALUE;
// extern FeedbackError E_OPTIONS_OPTION;

/********************************************************************
 *   Dummy C function for autoconf to find.
 *******************************************************************/
extern "C" void autotools_dummy ();

class OptionData
{
public:
//   typedef enum { optional, mandatory } ERequired;
  Feedback m_fb;
  const std::string m_sDescription; // A user-friendly description of what the option means
  const bool m_bMandatory; // True if the user must supply a value for this option.  False if the default value is enough.
  bool m_bUserSet; // True if set by user.  Should be set to true by Parse in subclasses.
  char m_cShortOption; // Optional short option, for use on the command line.
  OptionData(const std::string &sDesc, bool bM, char cShortOpt = 0);
  virtual ~OptionData() {};
  virtual int Parse(const std::string &sVal) = 0;
  virtual std::string ValueAsString() const = 0;
};


class OptionString : public OptionData
{
public:
  const std::string m_defaultValue;
  std::string m_value;
  OptionString(const std::string &sDesc, bool bM, const std::string &sDefVal, char cShortOpt = 0);
//   OptionString();
  virtual ~OptionString() {};
  virtual int Parse(const std::string &sVal);
  virtual std::string ValueAsString() const;
};



class OptionStrings : public OptionData
{
public:
  const std::vector<std::string> m_defaultValue;
  const size_t m_nMinElems, m_nMaxElems;
  std::vector<std::string> m_value;
  OptionStrings(const std::string &sDesc, bool bM, const std::vector<std::string> &sDefVal,
               size_t nMinElems = 1, size_t nMaxElems = 0, char cShortOpt = 0);
  virtual ~OptionStrings() {};
  virtual int Parse(const std::string &sVal);
  virtual std::string ValueAsString() const;
};


class OptionInt : public OptionData
{
public:
  const int m_defaultValue;
  int m_value;
  OptionInt(const std::string &sDesc, bool bM, int nDefVal, char cShortOpt = 0);
//   OptionInt();
  virtual ~OptionInt() {};
  virtual int Parse(const std::string &sVal);
  virtual std::string ValueAsString() const;
};


class OptionInts : public OptionData
{
public:
  const std::vector<int> m_defaultValue;
  const size_t m_nMinElems, m_nMaxElems;
  std::vector<int> m_value;
  OptionInts(const std::string &sDesc, bool bM, std::vector<int> defVal, 
             size_t nMinElems = 1, size_t nMaxElems = 0, char cShortOpt = 0);
  virtual ~OptionInts() {};
  virtual int Parse(const std::string &sVal);
  virtual std::string ValueAsString() const;
};


class OptionFloat : public OptionData
{
public:
  const float m_defaultValue;
  float m_value;
  OptionFloat(const std::string &sDesc, bool bM, float fDefVal, char cShortOpt = 0);
//   OptionFloat();
  virtual ~OptionFloat() {};
  virtual int Parse(const std::string &sVal);
  virtual std::string ValueAsString() const;
};


class OptionBool : public OptionData
{
public:
      // Deleted m_bOptionEnables, I can't see when giving the
      // option-key on its own (on the command line) will toggle
      // the option to false, it doesn't make sense.
  const bool m_defaultValue;
  bool m_value;
  OptionBool(const std::string &sDesc, bool bM, bool bDefVal, char cShortOpt = 0);
  virtual ~OptionBool() {};
  virtual int Parse(const std::string &sVal);
  virtual std::string ValueAsString() const;
};


class Options
{
  typedef std::map<std::string, ref_ptr<OptionData> > TOptionSet;

# if defined(DEBUG)
  mutable std::set<std::string> used_opts;
# endif

  Feedback m_fb;
  TOptionSet m_opts;
  bool m_bIgnoreUnrecognized;
  int m_nOpterrWas;
  void Preprocess(std::istream &source, std::ostream &target);
  void ReadPreprocessed(std::istream &stripped, bool bAdd);

  Options();

  template<class TOpt, class TVal> 
  int GetTypedOption(const std::string &sTypeDesc, const std::string &sKey, TVal *pValue) const;
  int FindOption(const std::string &sKey, OptionData **ppValue) const;
public:
  typedef enum { seed, verbose, optionfile, help, info_level } eStandardOption;
  static const char comment_char;
  static const char separator_char;

//   Options(std::istream &default_opts, std::istream &current_opts);
  static Options &Instance(); 
  ~Options();

  void IgnoreUnrecognized(bool bIgnore);
  bool IgnoreUnrecognized() const;

      // Assign or append options: Define the available options
      // (keys), together with description and default value.
  void Assign(const std::list<std::pair<std::string, OptionData*> > &options);
  void Append(const std::string &sKey, OptionData *pOption);
  void AppendStandard(eStandardOption option) throw(std::range_error);
  void AppendStandard(const eStandardOption *pOptions, size_t nNumOptions) throw(std::range_error);

      // Read options from stream.  New keys are not created, only
      // existing keys will be assigned new values.
  int Read(std::istream &opts);

      // Parse command line options.  If sFileOpt is found, use
      // the argument file name as input to Read function above.
      // Command line options take precedence over file options.
      // pargc and pargv are pointers to argc and argv,
      // respectively.  These will be changed to reflect the
      // options parsed successfully.
  int ReadWithCommandLine(int *pargc, char ***pargv, std::string sFileOpt = "options");


  int Option(const std::string &sKey, std::string &sValue) const;
  int Option(const std::string &sKey, int &nValue) const;
  int Option(const std::string &sKey, float &fValue) const;
  int Option(const std::string &sKey, bool &bValue) const;
  int Option(const std::string &sKey, std::vector<int> &value) const;
  int Option(const std::string &sKey, std::vector<std::string> &value) const;

  std::string PrintValues() const;
  std::string PrettyPrint() const;

  bool IsOption(const std::string &sKey) const;
  bool IsUserSet(const std::string &sKey) const;

# if defined(DEBUG)
  std::string PrintUnused() const;
# endif
};

#endif



