#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# Run as follows for release build:
# ./configure --prefix=$HOME/local [--disable-static --enable-shared] --enable-release
# Run as follows for debug build:
# ./configure --prefix=$HOME/local 

# If compiling with an old Intel compiler (< ver 9), and the
# utilities library is built with g++: Build utilities
# statically, and run configure with the additional option
# CXXFLAGS=-cxxlib-gcc.  Possibly also include
# -gcc-version=<something> in CXXFLAGS.  Consider compiling the
# utilities library with the same compiler as simdist, for
# instance by 'module unload mpi', 'module load Mpi.mpich-gnu'.
# Note: some of this text is printed as a warning below when the
# test for utilities fails.

dnl Autoconf requirements
AC_PREREQ(2.50)
AC_INIT([simdist], [1.1], [hoversta at idi.ntnu.no])

# AC_CONFIG_AUX_DIR must be called before AM_INIT_AUTOMAKE
AC_CONFIG_AUX_DIR([config])

AM_INIT_AUTOMAKE([1.9 foreign])

dnl information on the package
AC_CONFIG_SRCDIR([src/master.cpp])
AC_CONFIG_HEADER([config.h])

dnl Check whether to enable distribution system.  Override MPI
dnl selection here for now.
AC_MSG_CHECKING([whether to enable parallel distribution system]) 
AC_ARG_ENABLE([parallel], [AC_HELP_STRING([--enable-parallel=[no/yes]],
		      [build the parallel distribution system [default=yes]])],, 
		      [enable_parallel=yes])
if test "x$enable_parallel" = "xyes"; then
   AC_MSG_RESULT(yes)
   enable_mpi=yes
else
   AC_MSG_RESULT(no)
   enable_mpi=no
fi
AM_CONDITIONAL([enable_parallel], [test x$enable_parallel = xyes])

dnl Check for enable MPI.  Prefer mpicxx over mpiCC because
dnl mpicxx is the only one installed by default by mpiCH.
AC_MSG_CHECKING([whether to enable MPI])
dnl DISABLE THIS FOR NOW.  Override with enable-parallel above until Pvm is also a viable choice.
dnl AC_ARG_ENABLE([mpi], [AC_HELP_STRING([--enable-mpi=[no/yes]],
dnl   		     [build using MPI for distribution [default=yes]])],
dnl 		     , [enable_mpi=yes])

if test "x$enable_mpi" = "xyes"; then
   AC_MSG_RESULT(yes)
   CC_preferred="mpicc"
   CXX_preferred="mpicxx mpic++ mpiCC"
else
   AC_MSG_RESULT(no)
   CC_preferred="gcc cc cl egcs"
   CXX_preferred="g++ c++ CC cxx cc++ cl KCC xlC aCC egcs gcc"
fi
AM_CONDITIONAL([enable_mpi], [test x$enable_mpi = xtrue])


dnl Check for enable Pvm.  Not supported yet.
dnl AC_MSG_CHECKING([whether to enable Pvm])
dnl dnl AC_ARG_ENABLE([mpi], [AC_HELP_STRING([--enable-mpi=[no/yes]],
dnl dnl   		     [build using MPI for distribution [default=yes]])],
dnl dnl 		     , [enable_mpi=yes])
enable_pvm=no
dnl if test "x$enable_pvm" = "xyes"; then
dnl    AC_MSG_RESULT(yes)
dnl else
dnl    AC_MSG_RESULT(no)
dnl fi
dnl AM_CONDITIONAL([enable_pvm], [test x$enable_pvm = xtrue])


dnl Checking to see if we should enable debugging.  Default: no.
AC_MSG_CHECKING([whether to enable debugging])
AC_ARG_ENABLE([debug], [AC_HELP_STRING([--enable-debug=[no/yes]],
		       [turn on debugging [default=no]])],, [enable_debug=no])
if test "x$enable_debug" = "xyes"; then
   AC_MSG_RESULT(yes)
else
   AC_MSG_RESULT(no)
fi


dnl Checking to see if we should enable efence.  Default: no.
AC_MSG_CHECKING([whether to enable efence])
AC_ARG_ENABLE([efence], [AC_HELP_STRING([--enable-efence=[no/yes]],
		       [link with memory-leak detection library efence [default=no]])],, [enable_efence=no])
if test "x$enable_efence" = "xyes"; then
   AC_MSG_RESULT(yes)
else
   AC_MSG_RESULT(no)
fi


dnl Checking to see if we should build optimized.  Default: no.
AC_MSG_CHECKING([whether to optimize compilation])
AC_ARG_ENABLE([optimized], [AC_HELP_STRING([--enable-optimized=[no/yes]],
		       [turn on optimized compilation [default=no]])],, [enable_optimized=no])
if test "x$enable_optimized" = "xyes"; then
   AC_MSG_RESULT(yes)
else
   AC_MSG_RESULT(no)
fi


dnl Checking to see if we should build for release.  Default: yes.
AC_MSG_CHECKING([whether to create a release build])
AC_ARG_ENABLE([release], [AC_HELP_STRING([--enable-release=[no/yes]],
		       [create a release build (identical to --disable-debug --enable-optimized) [default=yes]])],, [enable_release=yes])
if test "x$enable_release" = "xyes"; then
   AC_MSG_RESULT(yes)
   enable_debug="no"
   enable_optimized="yes"
else
   AC_MSG_RESULT(no)
fi


dnl Checking to see if we should enable profiling.  Default: no.
AC_MSG_CHECKING([whether to enable the profiler])
AC_ARG_ENABLE([profiler], [AC_HELP_STRING([--enable-profiler=[no/yes]],
		       [turn on the profiler [default=no]])],
		       , [enable_profiler=no])

if test "x$enable_profiler" = "xyes"; then
   AC_MSG_RESULT(yes)
else
   AC_MSG_RESULT(no)
fi

dnl checks for programs 

dnl Its not clear from the manual (IMHO), but AC_PROG_CXX and
dnl AC_PROG_CC must be run unconditionally.
AC_PROG_CC([$CC_preferred])
AC_PROG_CXX([$CXX_preferred])
if test "x$enable_mpi" = "xyes"; then
   if test -z "`echo x$CXX | grep mpi`"; then
      AC_MSG_WARN([Parallel evaluation using MPI Enabled, but unable to find the MPI C++ compiler, or another compiler is set by user (CXX="$CXX").])
   fi
fi
dnl AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_CPP

dnl This macro must come before AC_PROG_LIBTOOL
if test "x$enable_debug" = "xyes"; then
   AC_DISABLE_SHARED
fi
#b Substitute AC_PROG_RANLIB with AC_PROB_LIBTOOL if using libtool
# AC_PROG_RANLIB
AC_PROG_LIBTOOL

for tool in CC CXX INSTALL CPP LIBTOOL; do
   if test -z "${!tool}"; then
      AC_MSG_ERROR([Failed to set $tool. A tool required for compilation is missing.])
   fi
done

dnl Add prefix path to include and library search paths.
if test x$prefix != xNONE; then
   AC_MSG_NOTICE([Adding include and lib subdirectories of prefix to header and linker search path, respectively.])
   CPPFLAGS="$CPPFLAGS -I$prefix/include"
   LDFLAGS="$LDFLAGS -L$prefix/lib"
fi   
 
dnl Set language to C++ for all subsequent tests
AC_LANG([C++])

dnl checks for libraries

# Check for pthread and enable multithreading.  Remember never to
# put the first AC_CHECK_LIB call in a script inside a condition.
AC_CHECK_LIB([rt], [sched_get_priority_max])
if ( test "x$ac_cv_lib_rt_sched_get_priority_max" != xyes ); then
  AC_MSG_WARN([Failed to detect rt (realtime) library!])
fi

AC_CHECK_LIB([pthread], [pthread_create])
if ( test "x$ac_cv_lib_pthread_pthread_create" != xyes); then
  AC_MSG_ERROR([Failed to detect pthread library!])
fi

dnl Check for debugging settings 
if test "x$enable_debug" = "xyes"; then
   CFLAGS="$CFLAGS -g -DDEBUG -O0"
   CXXFLAGS="$CXXFLAGS -g -DDEBUG -O0"
   CPPFLAGS="$CPPFLAGS -g -DDEBUG -O0"
   if [[[ -z "`$CXX --version 2>/dev/null | grep Intel`" && -z "`$CXX -V 2>/dev/null | grep Intel`" ]]]; then
      CFLAGS="$CFLAGS -Wall"
      CXXFLAGS="$CXXFLAGS -Wall"
   else
      AC_MSG_NOTICE([Not adding -Wall to Intel compiler options due to numerous remarks.])
   fi
fi

dnl Check for efence
if test "x$enable_efence" = "xyes"; then
   AC_CHECK_LIB([efence], [malloc])
   if ( test "x$ac_cv_lib_efence_malloc" != xyes ); then
      AC_MSG_ERROR([Failed to detect efence library.  Locate manually or configure without --enable-efence])
   fi
fi

dnl Check for optimization settings.
if test "x$enable_optimized" = "xyes"; then
    CFLAGS="$CFLAGS -O3"
    CPPFLAGS="$CPPFLAGS -O3"
    CXXFLAGS="$CXXFLAGS -O3"
fi

if test "x$enable_profiler" = "xyes"; then
   CFLAGS="$CFLAGS -pg"
   CPPFLAGS="$CPPFLAGS -pg"
   CXXFLAGS="$CXXFLAGS -pg"
fi

if test "x$enable_pvm" = "xyes"; then
dnl Check for PVM_ROOT.  This may not be defined, it isn't on OS X, but the
dnl headers and the library may still be available.
  if test "$PVM_ROOT"; then
    dnl Pvm root found, now look for the pvmtmparch program, which
    dnl will return architecture specific library directory
    if test -f "$PVM_ROOT/lib/pvmtmparch"; then
      PVM_ARCH=`$PVM_ROOT/lib/pvmtmparch`
    fi
    CPPFLAGS="$CPPFLAGS -I$PVM_ROOT/include"
    LDFLAGS="$LDFLAGS -L$PVM_ROOT/lib/$PVM_ARCH -R$PVM_ROOT/lib/$PVM_ARCH"
  else
    AC_MSG_WARN([PVM_ROOT not set, trying to find Pvm headers and libraries in normal search paths])
  fi
	
  dnl Now check headers and library.  Pvm uses socket and xdr functions on
  dnl certain platforms.  TODO: Improve the tests here, and especially
  dnl feedback if it fails.  In the future, Pvm slaves should be separated
  dnl out as an optional package.
  dnl
  dnl Other notes: Compile pvm with the right settings, as found in a
  dnl custom settings file in the 'conf' subdirectory of pvm named after
  dnl the operating system.  Remember that the libraries must come after
  dnl the source file being compiled.
  AC_CHECK_HEADERS([pvm3.h])
  AC_CHECK_LIB([socket], [getsockname])
  AC_CHECK_LIB([nsl], [xdr_double])
  AC_CHECK_LIB([pvm3], [pvm_mytid])
  if ( test "x$ac_cv_lib_pvm3_pvm_mytid" != xyes \
     || test "x$ac_cv_header_pvm3_h" != xyes); then
     AC_MSG_ERROR([Cannot find pvm3 headers and/or libraries (PVM_ROOT is '$PVM_ROOT')])
  fi
  
fi
  
dnl Export CPPFLAGS variable (so Makefile substitutions can be made).
dnl b What does this actually mean?  Ref. Automake manual 26.5 "Flag
dnl Variables Ordering", especially 26.5.1 "Compile Flag Variables".
dnl AC_SUBST(CFLAGS)

dnl checks for header files

dnl Check for mpi headers.  Check especially mpi++, this needs to be
dnl explicitly included in order for mpich c++ bindings to work.
if test "x$enable_mpi" = "xyes"; then
   AC_CHECK_HEADERS([mpi.h mpi2c++/mpi++.h])
fi

AC_CHECK_HEADERS([ios])

dnl checks for types

dnl Check if getopt is declared outside of getopt.h...
AC_CHECK_DECLS([getopt])
dnl ...then check for getopt.h.  Do this after the check for
dnl getopt, otherwise getopt.h will be included by configure in
dnl conftest when checking for the declaration of getopt.  Only
dnl getopt.h has getopt_long.
AC_CHECK_HEADERS([getopt.h])

dnl checks for structures
dnl checks for compiler characteristics
dnl checks for library functions

dnl Check for the non-portable relative time extension to
dnl timedwait.  Make sure this happens after the generic pthreads
dnl check, so -lpthread is used when running this test.
AC_CHECK_FUNCS([pthread_cond_timedwait_relative_np clock_gettime])

#  Check for the existence of std::ios::sync_with_stdio.
#  Disabling this sync increases stl i/o speed (ref libstdc++-v3
#  HOWTO, chapter 27: Input/Output;
#  http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#8)
#  Note that the #-lines below ARE NOT COMMENTS, THEY ARE PART OF
#  THE TEST PROGRAM!.
AC_LANG_PUSH([C++])
AC_MSG_CHECKING([for std::ios::sync_with_stdio])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if defined(HAVE_IOS) 
#include <ios>
#endif]],
				   [[std::ios::sync_with_stdio(false);]])], 
                  [AC_MSG_RESULT(yes)
		   AC_DEFINE([HAVE_STD__IOS__SYNC_WITH_STDIO], [1],
                             [Define if std::ios::sync_with_stdio exists.])], 
                  [AC_MSG_RESULT(no)])
AC_LANG_POP([C++])

dnl checks for system services

dnl NB! Remember to update list of subdirs in Makefile.am as well!
dnl if test -d utilities; then
dnl    AC_CONFIG_SUBDIRS([utilities])
dnl    AM_CONDITIONAL([include_utilities], true)
dnl fi

AC_CONFIG_FILES([Makefile
		 src/Makefile
                 include/Makefile
		])

AC_OUTPUT

