# All work is done in subdirectories.  Note that build order
# corresponds to the order in which directories are listed here.

include $(top_srcdir)/config/Makefile.am.include

MAINTAINERCLEANFILES += aclocal.m4 configure.scan autoscan.log config.h.in~

# NB! Remember to update AC_CONFIG_FILES in configure.ac as well.
SUBDIRS = src include 
EXTRA_DIST = demos/lisp/cppslave.cpp  demos/lisp/create_config.sh  demos/lisp/Makefile  demos/lisp/master.lisp  demos/lisp/slave.lisp
EXTRA_DIST += demos/matlab/filter.awk
EXTRA_DIST += demos/python/master.py  demos/python/slave.py
