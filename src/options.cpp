/********************************************************************
 *   		options.cpp
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
 *   See header file for description.
 *******************************************************************/

#if HAVE_CONFIG_H
#include "../config.h"
#endif

#include <simdist/options.h>

#include <simdist/misc_utils.h>

#include <sys/timeb.h>

#include <sstream>
#include <fstream>
#include <cstring>
#include <iomanip>
#include <cctype>
#include <iostream>
#include <locale>
#include <cassert>
#if HAVE_GETOPT_H
#include <getopt.h>
#endif

/********************************************************************
 *   Dummy C function for autoconf to find.
 *******************************************************************/
extern "C" void autotools_dummy () {}

// FeedbackError E_OPTIONS_TYPE("Option cannot be converted to the requested type");
DEFINE_FEEDBACK_ERROR(E_OPTIONS_TYPE, "Option cannot be converted to the requested type");
// FeedbackError E_OPTIONS_VALUE("A mandatory option has not been given a value");
DEFINE_FEEDBACK_ERROR(E_OPTIONS_VALUE, "A mandatory option has not been given a value");
// FeedbackError E_OPTIONS_OPTION("Request for non-existing option");
DEFINE_FEEDBACK_ERROR(E_OPTIONS_OPTION, "Request for non-existing option");
DEFINE_FEEDBACK_ERROR(E_OPTIONS_INVALID, "Invalid option or error in options file");
DEFINE_FEEDBACK_ERROR(E_OPTIONS_NOARGUMENT, "Missing argument to option");
DEFINE_FEEDBACK_ERROR(E_OPTIONS_FILE, "Could not open or read options file");
// DEFINE_FEEDBACK_ERROR(E_OPTIONDATA_CONVERT, "Failed to convert string option to requested type")

using namespace std;

const char Options::comment_char    = '#';
const char Options::separator_char  = ':';


OptionData::OptionData(const std::string &sDesc, bool bM, char cShortOpt /*=0*/)
    : m_fb("OptionData"), m_sDescription(sDesc), m_bMandatory(bM)
    , m_bUserSet(false), m_cShortOption(cShortOpt)
{
}

template<class TOpt, class TValue> 
static TValue
GetOptionValue(const TOpt *pOpt)
{
  if (pOpt->m_bUserSet)
    return pOpt->m_value;
  else
    return pOpt->m_defaultValue;
}

template<class TOpt, class TValue> 
static std::string
StringifyOptionValue(const TOpt *pOpt)
{
  std::stringstream ss;
  ss << GetOptionValue<TOpt, TValue>(pOpt);
  return ss.str();
}  

template<class TOpt, class TValue>
static std::string
StringifyMultiOptionValues(const TOpt *pOpt, std::string sDelim = " ")
{
  std::stringstream ss;
  TValue vals = GetOptionValue<TOpt, TValue>(pOpt);
  for (typename TValue::const_iterator it = vals.begin(); it != vals.end(); it++)
    ss << *it << sDelim;
  return ss.str();
}


OptionString::OptionString(const std::string &sDesc, bool bM, const std::string &sDefVal, char cShortOpt /*=0*/)
    : OptionData(sDesc, bM, cShortOpt), m_defaultValue(sDefVal)
{
}

int
OptionString::Parse(const std::string &sVal)
{
  m_value = sVal;
  m_bUserSet = true;
  return 0;
}

std::string 
OptionString::ValueAsString() const
{
  return StringifyOptionValue<OptionString, std::string>(this);
}

OptionInt::OptionInt(const std::string &sDesc, bool bM, int nDefVal, char cShortOpt /*=0*/)
    : OptionData(sDesc, bM, cShortOpt), m_defaultValue(nDefVal)
{
}


OptionStrings::OptionStrings(const std::string &sDesc, bool bM, const std::vector<std::string> &sDefVal, 
                             size_t nMinElems /*=1*/, size_t nMaxElems /*=0*/, char cShortOpt /*=0*/)
    : OptionData(sDesc, bM, cShortOpt), m_defaultValue(sDefVal), m_nMinElems(nMinElems), m_nMaxElems(nMaxElems)
{
}

int
OptionStrings::Parse(const std::string &sVal)
{
  std::stringstream ss(sVal);
  while (ss)
  {
    string s;
    ss >> s;
    if (!s.empty())
      m_value.push_back(s);
  }
  if (m_value.size() < m_nMinElems || (m_nMaxElems > 0 && m_value.size() > m_nMaxElems))
    return m_fb.Error(E_OPTIONS_TYPE) << ": Failed to convert " << sVal << " to a list of strings."
                                      << " Should have got between " << m_nMinElems << " and " << m_nMaxElems
                                      << " elements (0 = no limit). Found " << m_value.size() << ".";
  m_bUserSet = true;
  return 0;
}

std::string 
OptionStrings::ValueAsString() const
{
  return StringifyMultiOptionValues<OptionStrings, std::vector<std::string> >(this);
}


int
OptionInt::Parse(const std::string &sVal)
{
  if (sscanf(sVal.c_str(), "%d", &m_value) != 1)
    return m_fb.Error(E_OPTIONS_TYPE) << ": Failed to convert " << sVal << " to integer.";
  m_bUserSet = true;
  return 0;
}

std::string 
OptionInt::ValueAsString() const
{
  return StringifyOptionValue<OptionInt, int>(this);
}


OptionInts::OptionInts(const std::string &sDesc, bool bM, std::vector<int> defVal, 
                       size_t nMinElems /*=1*/, size_t nMaxElems /*=0*/, char cShortOpt /*=0*/)
    : OptionData(sDesc, bM, cShortOpt), m_defaultValue(defVal), m_nMinElems(nMinElems), m_nMaxElems(nMaxElems)
{
}

int
OptionInts::Parse(const std::string &sVal)
{
  std::string sFxVal = " " + sVal, sWhiteSpace = " \t\n";
  std::string::size_type it = 0;
  while ((it = sFxVal.find_first_of(sWhiteSpace, it)) != std::string::npos)
  {
    int nValue;
    if (sscanf(&(sVal.c_str()[it]), "%d", &nValue) == 1)
      m_value.push_back(nValue);
    it = sFxVal.find_first_not_of(sWhiteSpace, it);
  }
  if (m_value.size() < m_nMinElems || (m_nMaxElems > 0 && m_value.size() > m_nMaxElems))
    return m_fb.Error(E_OPTIONS_TYPE) << ": Failed to convert " << sVal << " to a list of integers."
                                      << " Should have got between " << m_nMinElems << " and " << m_nMaxElems
                                      << " elements (0 = no limit). Found " << m_value.size() << ".";
  m_bUserSet = true;
  return 0;
}

std::string 
OptionInts::ValueAsString() const
{
  return StringifyMultiOptionValues<OptionInts, std::vector<int> >(this);
}




OptionFloat::OptionFloat(const std::string &sDesc, bool bM, float fDefVal, char cShortOpt /*=0*/)
    : OptionData(sDesc, bM, cShortOpt), m_defaultValue(fDefVal)
{
}

int
OptionFloat::Parse(const std::string &sVal)
{
  if (sscanf(sVal.c_str(), "%f", &m_value) != 1)
    return m_fb.Error(E_OPTIONS_TYPE) << ": Failed to convert " << sVal << " to float.";
  m_bUserSet = true;
  return 0;
}

std::string 
OptionFloat::ValueAsString() const
{
  return StringifyOptionValue<OptionFloat, float>(this);
}

OptionBool::OptionBool(const std::string &sDesc, bool bM, bool bDefVal, char cShortOpt /*=0*/)
    : OptionData(sDesc, bM, cShortOpt), m_defaultValue(bDefVal)
{
}

int
OptionBool::Parse(const std::string &sVal)
{
  if (strcasecmp(sVal.c_str(), "true") 
      && strcasecmp(sVal.c_str(), "yes")
      && strcasecmp(sVal.c_str(), "on"))
  {
    if (strcasecmp(sVal.c_str(), "false")
        && strcasecmp(sVal.c_str(), "no")
        && strcasecmp(sVal.c_str(), "off"))
      return m_fb.Error(E_OPTIONS_TYPE) << ": Failed to interpret " << sVal << " as a boolean.  "
                                        << "Allowed toggles are \"true/false\", \"yes/no\" and \"on/off\"";
    else
      m_value = false;
  }
  else
    m_value = true;
  m_bUserSet = true;
  return 0;
}

std::string 
OptionBool::ValueAsString() const
{
  return StringifyOptionValue<OptionBool, bool>(this);
}


Options::Options()
    : m_fb("Options"), m_bIgnoreUnrecognized(false), m_nOpterrWas(opterr)
{
}

Options::~Options()
{
}

Options&
Options::Instance()
{
      // Static function variables are always initialized.
      // Static global c++ object variables in shared objects are
      // not correctly constructed on some platforms.
  static Options instance; 

  return instance;
}



/********************************************************************
 *   Ignoring an unrecognized option involves not printing an
 *   error message if such an option is found.
 *******************************************************************/
void
Options::IgnoreUnrecognized(bool bIgnore)
{
  if (m_bIgnoreUnrecognized == bIgnore)
    return;
  if (bIgnore)
  {
    m_nOpterrWas = opterr;
    opterr = 0;
  }
  else
    opterr = m_nOpterrWas;
  m_bIgnoreUnrecognized = bIgnore;
}


bool
Options::IgnoreUnrecognized() const
{
  return m_bIgnoreUnrecognized;
}


void
Options::Assign(const std::list<std::pair<std::string, OptionData*> > &options)
{
  m_opts.clear();
  for (std::list<std::pair<std::string, OptionData*> >::const_iterator opt_it = options.begin();
       opt_it != options.end(); opt_it++)
    Append(opt_it->first, opt_it->second);
}

void
Options::Append(const std::string &sKey, OptionData *pOption)
{
  m_opts[sKey] = ref_ptr<OptionData>(pOption);
}


void
Options::AppendStandard(eStandardOption option) throw(std::range_error)
{
  switch(option)
  {
      case seed:
        {
          struct timeb tp;
          ftime(&tp);
          int nSeed = tp.time + tp.millitm;
          Append("seed", new OptionInt("Random seed (use system time if not user set)", false, nSeed));
          break;
        }
      case verbose:
        Append("verbose", new OptionBool("Print verbose output (true/false)", false, false, 'v'));
        break;
      case optionfile:
        Options::Instance().Append("options", new OptionString("File where options are stored", false, ""));
        break;
      case help:
        Options::Instance().Append("help", new OptionBool("Print a help message", false, false, 'h'));
        break;
      case info_level:
        Options::Instance().Append("info-level", new OptionInt("Level of verbosity for the feedback system", false, 0));
        break;
      default:
        throw std::range_error("Unhandled standard option!");
  }

}


void
Options::AppendStandard(const eStandardOption *pOptions, size_t nNumOptions) throw(std::range_error)
{
  for (size_t nOpt = 0; nOpt < nNumOptions; nOpt++)
    AppendStandard(pOptions[nOpt]);
}


/********************************************************************
 *   Preprocess a file: Remove trailing blanks, comments, comment
 *   lines and empty lines.  Originally (before 2008), each line
 *   that passed preprocessing was prepended with a number
 *   indicating the line number in the original file, so that the
 *   parsing routine could give better feedback to the user in
 *   case of an error.  This was removed since it doesn't make
 *   any sense for command line parsing.
 *******************************************************************/
void
Options::Preprocess(istream &source, ostream &target)
{
  std::stringstream ss;
  if (UncommentStream(source, ss))
    return;
  
  std::string sLine;
  while (std::getline(ss, sLine))
  {
        // Replace all spaces, tabs etc with normal space
    for (size_t i = 0; i < sLine.size(); i++)
      if (isspace(sLine[i])) sLine[i] = ' ';

    sLine = Trim(sLine);
    if (!sLine.empty())
      target << sLine << endl;
  }
}


int
Options::Read(istream &options)
{
  stringstream stripped;
  Preprocess(options, stripped);

  std::string sLine;
  while(std::getline(stripped, sLine))
  {
    string::size_type opt_pos, val_pos;
    if (((opt_pos = sLine.find_first_of(separator_char)) != string::npos)
        && ((val_pos = sLine.find_first_not_of(" ", opt_pos+1)) != string::npos))
    {
      string key = sLine.substr(0, opt_pos);
      TOptionSet::iterator key_it = m_opts.find(key);
      if (key_it != m_opts.end())
      {
        if (key_it->second->Parse(sLine.substr(val_pos, sLine.size() - val_pos)))
          return m_fb.Error(E_OPTIONS_TYPE) << ": " << key_it->first;
      }
      else 
        return m_fb.Error(E_OPTIONS_INVALID) << ": " << sLine;
    } 
    else 
      return m_fb.Error(E_OPTIONS_INVALID) << ": " << sLine;
  }
  return 0;
}



/********************************************************************
 *   Parse options, put them into stream.  Send sFileOpt file
 *   first to read, then the newly created stream.  This will
 *   give command line options precedence over file options.
 *******************************************************************/
int
Options::ReadWithCommandLine(int *pargc, char ***pargv, std::string sFileOpt /*="options"*/)
{
#if HAVE_GETOPT_H
      // Add '+' to avoid reordering options and stop on first
      // non-option.  Necessary if reading options before sending
      // argc/argv to another parser, i.e. the Options lib
      // doesn't recognize all the options in the string.
  std::string sShortOptions = "+"; 
  std::map<char, int> optRemapper; // Map from short to long option.

  struct option *longopts = new option[m_opts.size() + 2];
  memset(longopts, 0, sizeof(option)*(m_opts.size() + 2));

      // Add option for config file 
  char *c = new char[sFileOpt.size() + 1];
  strcpy(c, sFileOpt.c_str());
  longopts[0].name = c; // I am for some weird reason unable to assign directly...
  longopts[0].has_arg = required_argument;

      // Build option array for getopt_long.  The fields "flag"
      // and "val" are set to 0 by memset above.
  int nOptIdx = 1;
  for (TOptionSet::const_iterator opit = m_opts.begin(); opit != m_opts.end();
       opit++, nOptIdx++)
  {
    struct option &opt = longopts[nOptIdx];
        // Create long option from description
    char *c = new char[opit->first.size() + 1];;
    strcpy(c, opit->first.c_str());
    opt.name = c;

        // Add optional short option.  Short options do not
        // support optional arguments!  
    if (opit->second->m_cShortOption != 0)
    {
      sShortOptions += opit->second->m_cShortOption;
          // Assume all other than OptionBool to take a mandatory
          // argument, so add a colon to the argument list.
      if (!dynamic_cast<OptionBool*>(opit->second.GetPtr()))
        sShortOptions += ":";
      optRemapper[opit->second->m_cShortOption] = nOptIdx;
    }
      
        // Optional argument for toggles, required argument
        // otherwise. 
    if (dynamic_cast<OptionBool*>(opit->second.GetPtr()))
      opt.has_arg = optional_argument;
    else
      opt.has_arg = required_argument;
  }

  std::stringstream ssCmdLineOptions;

      // Parse command line options
  int nRet, nLongOptIdx = 0, nNumIgnored = 0;
  while((nRet = getopt_long_only(*pargc, *pargv, sShortOptions.c_str(), longopts, &nLongOptIdx)) != -1)
  {
    switch (nRet)
    {
          // Fall through on 0, this means that a long option has
          // successfully been processed.  nLongOptIdx has been
          // set to the corresponding index in longopts.
        case 0:
          break;
        case ':':
          return m_fb.Error(E_OPTIONS_NOARGUMENT);
        case '?':
          if (m_bIgnoreUnrecognized)
          {
            nNumIgnored++;
            continue;
          }
          return m_fb.Error(E_OPTIONS_INVALID);
        default:
              // We get here either due to a bug, or because a
              // short option was recognized and returned (i.e. a
              // char cast as an int).  In this case we set
              // nLongOptIdx to what getopt_long would have
              // returned, given the corresponding long option.
          if (optRemapper.find(nRet) == optRemapper.end())
          {
            m_fb.Warning("Possible bug: getopt_long returned short option character not found in option string.  Ignored option: ")
              << static_cast<char>(nRet) << " (integer value: " << nRet << ")";
            continue;
          }
          nLongOptIdx = optRemapper[nRet];
    }
      
        // First option is config file option
    if (nLongOptIdx == 0)
    {
      assert(!strcmp(longopts[nLongOptIdx].name, sFileOpt.c_str()));
      std::ifstream optsFile(optarg);
      if (!optsFile)
        return m_fb.Error(E_OPTIONS_FILE) << optarg;
      if (int nReadRet = Read(optsFile))
        return nReadRet;
      continue;
    }

        // Only toggles come without arguments.
    ssCmdLineOptions << longopts[nLongOptIdx].name << ":\t";
    if (optarg)
      ssCmdLineOptions << optarg << "\n";
    else
      ssCmdLineOptions << "true\n";
  }
  
      // Update argc and argv.  optind starts at 1, and some
      // routines may expect program name to be in argv[0].
      // Consider resetting optind as well.
  int nNumRem = optind - nNumIgnored - 1;
  for (int nArg = nNumRem + 1; nArg < *pargc; nArg++)
    (*pargv)[nArg - nNumRem] = (*pargv)[nArg];
  *pargc -= nNumRem;
          
  for (size_t nOpt = 0; nOpt < m_opts.size(); nOpt++)    
    delete [] longopts[nOpt].name;
  delete [] longopts;

  return Read(ssCmdLineOptions);

#else
      // Alternative if getopt_long is not supported: Search for sFileOpt.
  sFileOpt = "--" + sFileOpt;
      // Shift left to remove argv[0], for consistency with getopt.
  copy(*pargv + 1, *pargv + *pargc, *pargv);
  *pargc -= 1;

  bool bOptsFileFound = false;
  for (int nArg = 0; nArg < *pargc - 1; nArg++)
  {
    const char *opt = (*pargv)[nArg], *arg = (*pargv)[nArg + 1];
    if (!strcasecmp(opt, sFileOpt.c_str()))
    {
      std::ifstream optsFile(arg);
      if (!optsFile)
        return m_fb.Error(E_OPTIONS_FILE) << arg;
//       m_fb.Info(0) << "Reading options from options file " << arg;
      copy(*pargv + nArg + 2, *pargv + *pargc, *pargv + nArg);
      *pargc -= 2;


      if (int nReadRet = Read(optsFile))
        return nReadRet;
      bOptsFileFound = true;
    }
  }

  if (!bOptsFileFound || *pargc > 0)
    m_fb.Warning() << "getopt.h not found during installation; "
                   << "command line options other than '" << sFileOpt.c_str() 
                   << " <filename>' will not be supported.  "
                   << "Consider updating libc and recompiling.";

  if (!bOptsFileFound)
    return m_fb.Error(E_OPTIONS_FILE) << ": No " << sFileOpt.c_str() 
                                      << " <filename> option found on command line."; 
  return 0;
#endif
}


int
Options::FindOption(const std::string &sKey, OptionData **ppValue) const
{
  TOptionSet::const_iterator key_it = m_opts.find(sKey);
  if (key_it == m_opts.end())
    return m_fb.Error(E_OPTIONS_OPTION) << " \"" << sKey << "\".";

# if defined(DEBUG)
  const_cast<set<string>* >(&used_opts)->insert(key_it->first);
# endif

  *ppValue = key_it->second.GetPtr();
  if ((*ppValue)->m_bMandatory && !(*ppValue)->m_bUserSet)
    return m_fb.Error(E_OPTIONS_VALUE) << ": " << sKey << ".";
  return 0;
}  



template<class TOpt, class TVal> int
Options::GetTypedOption(const std::string &sTypeDesc, const std::string &sKey, TVal *pValue) const
{
  OptionData *pOptData;
  if (int nRet = FindOption(sKey, &pOptData))
    return nRet;
  
  TOpt *pTypedOptData = dynamic_cast<TOpt*>(pOptData);
  if (!pTypedOptData)
    return m_fb.Error(E_OPTIONS_TYPE) << ": " << sKey
                                      << " is not registered as an option of type " 
                                      << sTypeDesc << ".";
  *pValue = GetOptionValue<TOpt, TVal>(pTypedOptData);
  return 0;
}


int
Options::Option(const string &sKey, string &sValue) const
{
  return GetTypedOption<OptionString, string>("string", sKey, &sValue);
}

int
Options::Option(const string &sKey, int &nValue) const 
{
  return GetTypedOption<OptionInt, int>("integer", sKey, &nValue);
}


int
Options::Option(const string &sKey, float &fValue) const 
{
  return GetTypedOption<OptionFloat, float>("float", sKey, &fValue);
}


int
Options::Option(const std::string &sKey, bool &bValue) const
{
  return GetTypedOption<OptionBool, bool>("boolean", sKey, &bValue);
}


int
Options::Option(const std::string &sKey, std::vector<int> &value) const
{
  return GetTypedOption<OptionInts, std::vector<int> >("integer list", sKey, &value);
}

int
Options::Option(const std::string &sKey, std::vector<std::string> &value) const
{
  return GetTypedOption<OptionStrings, std::vector<string> >("string list", sKey, &value);
}

bool
Options::IsOption(const std::string &sKey) const
{
  return m_opts.find(sKey) != m_opts.end();
}

bool
Options::IsUserSet(const std::string &sKey) const
{
  return m_opts.find(sKey)->second->m_bUserSet;
}

string
Options::PrintValues() const 
{
  stringstream s;
  ios::fmtflags old_flags = s.flags();
  s.setf(ios::left, ios::adjustfield);
  for (TOptionSet::const_iterator op_it = m_opts.begin(); op_it != m_opts.end(); op_it++)
  {
    s << setw(35) << op_it->first + ":" << "\t"
      << setw(15) << op_it->second->ValueAsString() 
      << (op_it->second->m_bUserSet ? "\t(user specified)\n" : "\t(default)\n");
  }
  s.flags(old_flags);
  return s.str();
}

std::string
Options::PrettyPrint() const
{
  stringstream s;
  ios::fmtflags old_flags = s.flags();
  s.setf(ios::left, ios::adjustfield);
//   s << setw(35) << "Key" << setw(15) << "Value (default)" << setw(10) "Mandatory" << "Description\n";
  for (TOptionSet::const_iterator op_it = m_opts.begin(); op_it != m_opts.end(); op_it++)
  {
    string sOption = (op_it->second->m_bMandatory ? "" : "[") + op_it->first
      + (op_it->second->m_cShortOption ? string(" | ") + op_it->second->m_cShortOption : "")
      + (op_it->second->m_bMandatory ? "" : "]");
    string sDesc = op_it->second->m_sDescription;
    if (!sDesc.empty() && sDesc[sDesc.size()-1] != '.')
      sDesc += ".";
    s << setw(35) << sOption << "\t" << sDesc << " Value: \"" << op_it->second->ValueAsString() << "\" "
      << (op_it->second->m_bUserSet ? "(user specified)\n" : "(default)\n");
  }
  s.flags(old_flags);
  return s.str();
}


#if defined(DEBUG)
string 
Options::PrintUnused() const 
{
  stringstream unused_ss;
  unused_ss.setf(ios::left, ios::adjustfield);
  for (TOptionSet::const_iterator op_it = m_opts.begin(); op_it != m_opts.end(); op_it++)
    if (used_opts.find(op_it->first) == used_opts.end())
      unused_ss << setw(35) << op_it->first.c_str() << "\t" << op_it->second->ValueAsString() << "\n";
  return unused_ss.str();
}
#endif
