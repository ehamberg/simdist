# Makefile.am in directory 'dist' 

include $(top_srcdir)/config/Makefile.am.include

bin_PROGRAMS = logio pipeio

logio_SOURCES = logio.cpp
logio_LDADD = libsimdistutils.la

pipeio_SOURCES = pipeio.cpp pipeio_common.cpp 
pipeio_LDADD = libsimdistutils.la

noinst_PROGRAMS = test-feedback test-options test-mathutils test-misc-utils test-syncutils test-ref-ptr test-checkpoint

# noinst_LTLIBRARIES = libutilities_dbg.la

# Automake makefile complains if we try to use feedback.cpp both
# directly and in the utilities library, so we must include
# libsimdistutils instead of just linking directly to feedback.cpp.
test_checkpoint_SOURCES = test_checkpoint.cpp
test_checkpoint_LDADD = libsimdistutils.la

test_feedback_SOURCES = test_feedback.cpp
test_feedback_LDADD = libsimdistutils.la

test_options_SOURCES = test_options.cpp
test_options_LDADD = libsimdistutils.la

test_ref_ptr_SOURCES = test_ref_ptr.cpp
test_ref_ptr_LDADD = libsimdistutils.la

test_syncutils_SOURCES = test_syncutils.cpp
test_syncutils_LDADD = libsimdistutils.la

test_misc_utils_SOURCES = test_misc_utils.cpp
test_misc_utils_LDADD = libsimdistutils.la
# test_misc_utils_CPPFLAGS = $(AM_CPPFLAGS) -D_GLIBCXX_DEBUG

test_mathutils_LDADD = libsimdistutils.la
test_mathutils_SOURCES = test_mathutils.cpp

lib_LTLIBRARIES = libsimdistutils.la
libsimdistutils_la_SOURCES = feedback.cpp syncutils.cpp options.cpp errorcodes_common.cpp \
			  errorcodes_thread.cpp misc_utils.cpp io_utils.cpp

# libsimdistutils_dbg_la_SOURCES = $(libsimdistutils_la_SOURCES)
# libsimdistutils_dbg_la_CPPFLAGS = $(AM_CPPFLAGS) -D_GLIBCXX_DEBUG


EXTRA_DIST = simdist.in

if enable_parallel
  bin_PROGRAMS += simdist-mpi

  # noinst_PROGRAMS = pvmslavetester
  noinst_PROGRAMS += test-messages test-slave test-master test-testers multi-pipeio

  # Rules for creating simdist. Much more below.
  bin_SCRIPTS = simdist
  CLEANFILES = $(bin_SCRIPTS)

  multi_pipeio_SOURCES = multi_pipeio.cpp
  multi_pipeio_LDADD = pipeio_common.o libsimdistutils.la

  test_messages_SOURCES = test_messages.cpp 
  test_messages_LDADD = libsimdist.la libsimdistutils.la

  test_master_SOURCES = test_master.cpp 
  test_master_LDADD = libsimdistutils.la

  test_slave_SOURCES = test_slave.cpp
  test_slave_LDADD = libsimdistutils.la

  test_testers_SOURCES = test_testers.cpp
  test_testers_LDADD = libsimdist.la libsimdistutils.la 

  # pvmslavetester_SOURCES = pvm_slave_tester.cpp

  # slavestub_SOURCES = slave_stdio.cpp
  # slavestub_LDADD = libsimdist.la 

  # masterstub_SOURCES = master_stdio.cpp
  # masterstub_LDADD = libsimdist.la 

  simdist_mpi_SOURCES = frontend.cpp slave_stdio.cpp master_stdio.cpp
  simdist_mpi_LDADD = libsimdist.la libsimdistutils.la 
  # simdist_mpi_CPPFLAGS = $(AM_CPPFLAGS) -D_GLIBCXX_DEBUG

  lib_LTLIBRARIES += libsimdist.la
  libsimdist_la_SOURCES = slave.cpp jobqueue.cpp \
			master.cpp messages.cpp \
			slave_channel.cpp \
			slave_mpi.cpp timer.cpp

  # libsimdist_la_CPPFLAGS = $(AM_CPPFLAGS) -D_GLIBCXX_DEBUG

# Create the simdist script.  "Recipe" taken from a mix of
# Automake manual, "9.1 Executable Scripts", and Autoconf manual,
# "14.7.3 Transformation Rules".  We would optimally test for the
# existence of mpiexec, and alternatively use mpirun, but mpiexec
# is often not available on the front-end, only on compute nodes
# while running batch scripts.
  simstid_transform = @program_transform_name@
  simdist_program_base = $(shell echo simdist-mpi | sed -e '$(simdist_transform)')
  simdist_program = $(bindir)/$(simdist_program_base)
  simdist: Makefile simdist.in
	sed -e's,@SIMDISTMPI@,$(simdist_program),g; s,@SIMDISTMPIBASE@,$(simdist_program_base),g' simdist.in >simdist
	chmod +x simdist
endif

# if enable_pvm
#   libsimdist_la_SOURCES += slave_pvm.cpp 
# endif

