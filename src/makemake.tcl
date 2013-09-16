#!/usr/bin/tclsh
#
# Run this TCL script to generate the various makefiles for a variety
# of platforms.  Files generated include:
#
#     src/main.mk           # makefile for all unix systems
#     win/Makefile.mingw    # makefile for mingw on windows
#     win/Makefile.*        # makefiles for other windows compilers
#
# Run this script while in the "src" subdirectory.  Like this:
#
#      tclsh makemake.tcl
#
#############################################################################

# Basenames of all source files that get preprocessed using
# "translate" and "makeheaders".  To add new source files to the
# project, simply add the basename to this list and rerun this script.
#
set src {
  add
  allrepo
  attach
  bag
  bisect
  blob
  branch
  browse
  captcha
  cgi
  checkin
  checkout
  clearsign
  clone
  comformat
  configure
  content
  db
  delta
  deltacmd
  descendants
  diff
  diffcmd
  doc
  encode
  event
  export
  file
  finfo
  glob
  graph
  gzip
  http
  http_socket
  http_transport
  import
  info
  json
  json_artifact
  json_branch
  json_config
  json_diff
  json_dir
  json_finfo
  json_login
  json_query
  json_report
  json_status
  json_tag
  json_timeline
  json_user
  json_wiki
  leaf
  login
  lookslike
  main
  manifest
  markdown
  markdown_html
  md5
  merge
  merge3
  moderate
  name
  path
  pivot
  popen
  pqueue
  printf
  rebuild
  regexp
  report
  rss
  schema
  search
  setup
  sha1
  shun
  skins
  sqlcmd
  stash
  stat
  style
  sync
  tag
  tar
  th_main
  timeline
  tkt
  tktsetup
  undo
  unicode
  update
  url
  user
  utf8
  util
  verify
  vfile
  wiki
  wikiformat
  winhttp
  wysiwyg
  xfer
  xfersetup
  zip
  http_ssl
}

# Name of the final application
#
set name fossil

# The "writeln" command sends output to the target makefile.
#
proc writeln {args} {
  global output_file
  if {[lindex $args 0]=="-nonewline"} {
    puts -nonewline $output_file [lindex $args 1]
  } else {
    puts $output_file [lindex $args 0]
  }
}

# STOP HERE.
# Unless the build procedures changes, you should not have to edit anything
# below this line.

##############################################################################
##############################################################################
##############################################################################
# Start by generating the "main.mk" makefile used for all unix systems.
#
puts "building main.mk"
set output_file [open main.mk w]
fconfigure $output_file -translation binary

writeln {#
##############################################################################
# WARNING: DO NOT EDIT, AUTOMATICALLY GENERATED FILE (SEE "src/makemake.tcl")
##############################################################################
#
# This file is automatically generated.  Instead of editing this
# file, edit "makemake.tcl" then run "tclsh makemake.tcl"
# to regenerate this file.
#
# This file is included by primary Makefile.
#

XTCC = $(TCC) $(CFLAGS) -I. -I$(SRCDIR) -I$(OBJDIR)

}
writeln -nonewline "SRC ="
foreach s [lsort $src] {
  writeln -nonewline " \\\n  \$(SRCDIR)/$s.c"
}
writeln "\n"
writeln -nonewline "TRANS_SRC ="
foreach s [lsort $src] {
  writeln -nonewline " \\\n  \$(OBJDIR)/${s}_.c"
}
writeln "\n"
writeln -nonewline "OBJ ="
foreach s [lsort $src] {
  writeln -nonewline " \\\n \$(OBJDIR)/$s.o"
}
writeln "\n"
writeln "APPNAME = $name\$(E)"
writeln "\n"

writeln {
all:	$(OBJDIR) $(APPNAME)

install:	$(APPNAME)
	mkdir -p $(INSTALLDIR)
	mv $(APPNAME) $(INSTALLDIR)

$(OBJDIR):
	-mkdir $(OBJDIR)

$(OBJDIR)/translate:	$(SRCDIR)/translate.c
	$(BCC) -o $(OBJDIR)/translate $(SRCDIR)/translate.c

$(OBJDIR)/makeheaders:	$(SRCDIR)/makeheaders.c
	$(BCC) -o $(OBJDIR)/makeheaders $(SRCDIR)/makeheaders.c

$(OBJDIR)/mkindex:	$(SRCDIR)/mkindex.c
	$(BCC) -o $(OBJDIR)/mkindex $(SRCDIR)/mkindex.c

$(OBJDIR)/mkversion:	$(SRCDIR)/mkversion.c
	$(BCC) -o $(OBJDIR)/mkversion $(SRCDIR)/mkversion.c

# WARNING. DANGER. Running the test suite modifies the repository the
# build is done from, i.e. the checkout belongs to. Do not sync/push
# the repository after running the tests.
test:	$(OBJDIR) $(APPNAME)
	$(TCLSH) $(SRCDIR)/../test/tester.tcl $(APPNAME)

$(OBJDIR)/VERSION.h:	$(SRCDIR)/../manifest.uuid $(SRCDIR)/../manifest $(SRCDIR)/../VERSION $(OBJDIR)/mkversion
	$(OBJDIR)/mkversion $(SRCDIR)/../manifest.uuid \
		$(SRCDIR)/../manifest \
		$(SRCDIR)/../VERSION >$(OBJDIR)/VERSION.h

# The USE_SYSTEM_SQLITE variable may be undefined, set to 0, or set
# to 1. If it is set to 1, then there is no need to build or link
# the sqlite3.o object. Instead, the system sqlite will be linked
# using -lsqlite3.
SQLITE3_OBJ.1 = 
SQLITE3_OBJ.0 = $(OBJDIR)/sqlite3.o
SQLITE3_OBJ.  = $(SQLITE3_OBJ.0)

# The FOSSIL_ENABLE_TCL variable may be undefined, set to 0, or set to 1.
# If it is set to 1, then we need to build the Tcl integration code and
# link to the Tcl library.
TCL_OBJ.0 = 
TCL_OBJ.1 = $(OBJDIR)/th_tcl.o
TCL_OBJ. = $(TCL_OBJ.0)

EXTRAOBJ = \
  $(SQLITE3_OBJ.$(USE_SYSTEM_SQLITE)) \
  $(OBJDIR)/shell.o \
  $(OBJDIR)/th.o \
  $(OBJDIR)/th_lang.o \
  $(TCL_OBJ.$(FOSSIL_ENABLE_TCL)) \
  $(OBJDIR)/cson_amalgamation.o

$(APPNAME):	$(OBJDIR)/headers $(OBJ) $(EXTRAOBJ)
	$(TCC) -o $(APPNAME) $(OBJ) $(EXTRAOBJ) $(LIB)

# This rule prevents make from using its default rules to try build
# an executable named "manifest" out of the file named "manifest.c"
#
$(SRCDIR)/../manifest:	
	# noop

clean:	
	rm -rf $(OBJDIR)/* $(APPNAME)

}

set mhargs {}
foreach s [lsort $src] {
  append mhargs " \$(OBJDIR)/${s}_.c:\$(OBJDIR)/$s.h"
  set extra_h($s) {}
}
append mhargs " \$(SRCDIR)/sqlite3.h"
append mhargs " \$(SRCDIR)/th.h"
#append mhargs " \$(SRCDIR)/cson_amalgamation.h"
append mhargs " \$(OBJDIR)/VERSION.h"
writeln "\$(OBJDIR)/page_index.h: \$(TRANS_SRC) \$(OBJDIR)/mkindex"
writeln "\t\$(OBJDIR)/mkindex \$(TRANS_SRC) >$@"
writeln "\$(OBJDIR)/headers:\t\$(OBJDIR)/page_index.h \$(OBJDIR)/makeheaders \$(OBJDIR)/VERSION.h"
writeln "\t\$(OBJDIR)/makeheaders $mhargs"
writeln "\ttouch \$(OBJDIR)/headers"
writeln "\$(OBJDIR)/headers: Makefile"
writeln "\$(OBJDIR)/json.o \$(OBJDIR)/json_artifact.o \$(OBJDIR)/json_branch.o \$(OBJDIR)/json_config.o \$(OBJDIR)/json_diff.o \$(OBJDIR)/json_dir.o \$(OBJDIR)/json_finfo.o \$(OBJDIR)/json_login.o \$(OBJDIR)/json_query.o \$(OBJDIR)/json_report.o \$(OBJDIR)/json_status.o \$(OBJDIR)/json_tag.o \$(OBJDIR)/json_timeline.o \$(OBJDIR)/json_user.o \$(OBJDIR)/json_wiki.o : \$(SRCDIR)/json_detail.h"
writeln "Makefile:"
set extra_h(main) \$(OBJDIR)/page_index.h

foreach s [lsort $src] {
  writeln "\$(OBJDIR)/${s}_.c:\t\$(SRCDIR)/$s.c \$(OBJDIR)/translate"
  writeln "\t\$(OBJDIR)/translate \$(SRCDIR)/$s.c >\$(OBJDIR)/${s}_.c\n"
  writeln "\$(OBJDIR)/$s.o:\t\$(OBJDIR)/${s}_.c \$(OBJDIR)/$s.h $extra_h($s) \$(SRCDIR)/config.h"
  writeln "\t\$(XTCC) -o \$(OBJDIR)/$s.o -c \$(OBJDIR)/${s}_.c\n"
  writeln "\$(OBJDIR)/$s.h:\t\$(OBJDIR)/headers"
}


writeln "\$(OBJDIR)/sqlite3.o:\t\$(SRCDIR)/sqlite3.c"
set opt {-DSQLITE_OMIT_LOAD_EXTENSION=1}
append opt " -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_FILE_FORMAT=4"
#append opt " -DSQLITE_ENABLE_FTS3=1"
append opt " -DSQLITE_ENABLE_STAT3"
append opt " -Dlocaltime=fossil_localtime"
append opt " -DSQLITE_ENABLE_LOCKING_STYLE=0"
append opt " -DSQLITE_WIN32_NO_ANSI"
set SQLITE_OPTIONS $opt
writeln "\t\$(XTCC) $opt -c \$(SRCDIR)/sqlite3.c -o \$(OBJDIR)/sqlite3.o\n"

writeln "\$(OBJDIR)/shell.o:\t\$(SRCDIR)/shell.c \$(SRCDIR)/sqlite3.h"
set opt {-Dmain=sqlite3_shell}
append opt " -DSQLITE_OMIT_LOAD_EXTENSION=1"
append opt " -Dsqlite3_strglob=strglob"
writeln "\t\$(XTCC) $opt -c \$(SRCDIR)/shell.c -o \$(OBJDIR)/shell.o\n"

writeln "\$(OBJDIR)/th.o:\t\$(SRCDIR)/th.c"
writeln "\t\$(XTCC) -c \$(SRCDIR)/th.c -o \$(OBJDIR)/th.o\n"

writeln "\$(OBJDIR)/th_lang.o:\t\$(SRCDIR)/th_lang.c"
writeln "\t\$(XTCC) -c \$(SRCDIR)/th_lang.c -o \$(OBJDIR)/th_lang.o\n"

writeln "\$(OBJDIR)/th_tcl.o:\t\$(SRCDIR)/th_tcl.c"
writeln "\t\$(XTCC) -c \$(SRCDIR)/th_tcl.c -o \$(OBJDIR)/th_tcl.o\n"

set opt {}
writeln {
$(OBJDIR)/cson_amalgamation.o: $(SRCDIR)/cson_amalgamation.c
	$(XTCC) -c $(SRCDIR)/cson_amalgamation.c -o $(OBJDIR)/cson_amalgamation.o

#
# The list of all the targets that do not correspond to real files. This stops
# 'make' from getting confused when someone makes an error in a rule.
#

.PHONY: all install test clean
}

close $output_file
#
# End of the main.mk output
##############################################################################
##############################################################################
##############################################################################
# Begin win/Makefile.mingw output
#
puts "building ../win/Makefile.mingw"
set output_file [open ../win/Makefile.mingw w]
fconfigure $output_file -translation binary

writeln {#!/usr/bin/make
#
##############################################################################
# WARNING: DO NOT EDIT, AUTOMATICALLY GENERATED FILE (SEE "src/makemake.tcl")
##############################################################################
#
# This file is automatically generated.  Instead of editing this
# file, edit "makemake.tcl" then run "tclsh makemake.tcl"
# to regenerate this file.
#
# This is a makefile for use on Cygwin/Darwin/FreeBSD/Linux/Windows using
# MinGW or MinGW-w64.
#

#### Select one of MinGW, MinGW-w64 (32-bit) or MinGW-w64 (64-bit) compilers.
#    By default, this is an empty string (i.e. use the native compiler).
#
PREFIX =
# PREFIX = mingw32-
# PREFIX = i686-pc-mingw32-
# PREFIX = i686-w64-mingw32-
# PREFIX = x86_64-w64-mingw32-

#### The toplevel directory of the source tree.  Fossil can be built
#    in a directory that is separate from the source tree.  Just change
#    the following to point from the build directory to the src/ folder.
#
SRCDIR = src

#### The directory into which object code files should be written.
#
OBJDIR = wbld

#### C Compiler and options for use in building executables that
#    will run on the platform that is doing the build.  This is used
#    to compile code-generator programs as part of the build process.
#    See TCC below for the C compiler for building the finished binary.
#
BCC = gcc

#### Enable compiling with debug symbols (much larger binary)
#
# FOSSIL_ENABLE_SYMBOLS = 1

#### Enable JSON (http://www.json.org) support using "cson"
#
# FOSSIL_ENABLE_JSON = 1

#### Enable HTTPS support via OpenSSL (links to libssl and libcrypto)
#
# FOSSIL_ENABLE_SSL = 1

#### Enable scripting support via Tcl/Tk
#
# FOSSIL_ENABLE_TCL = 1

#### Load Tcl using the stubs mechanism
#
# FOSSIL_ENABLE_TCL_STUBS = 1

#### Use the Tcl source directory instead of the install directory?
#    This is useful when Tcl has been compiled statically with MinGW.
#
FOSSIL_TCL_SOURCE = 1

#### Check if the workaround for the MinGW command line handling needs to
#    be enabled by default.
#
ifndef BROKEN_MINGW_CMDLINE
ifeq (,$(findstring w64-mingw32,$(PREFIX)))
BROKEN_MINGW_CMDLINE = 1
endif
endif

#### The directories where the zlib include and library files are located.
#
ZINCDIR = $(SRCDIR)/../compat/zlib
ZLIBDIR = $(SRCDIR)/../compat/zlib

#### The directories where the OpenSSL include and library files are located.
#    The recommended usage here is to use the Sysinternals junction tool
#    to create a hard link between an "openssl-1.x" sub-directory of the
#    Fossil source code directory and the target OpenSSL source directory.
#
OPENSSLINCDIR = $(SRCDIR)/../compat/openssl-1.0.1e/include
OPENSSLLIBDIR = $(SRCDIR)/../compat/openssl-1.0.1e

#### Either the directory where the Tcl library is installed or the Tcl
#    source code directory resides (depending on the value of the macro
#    FOSSIL_TCL_SOURCE).  If this points to the Tcl install directory,
#    this directory must have "include" and "lib" sub-directories.  If
#    this points to the Tcl source code directory, this directory must
#    have "generic" and "win" sub-directories.  The recommended usage
#    here is to use the Sysinternals junction tool to create a hard
#    link between a "tcl-8.x" sub-directory of the Fossil source code
#    directory and the target Tcl directory.  This removes the need to
#    hard-code the necessary paths in this Makefile.
#
TCLDIR = $(SRCDIR)/../compat/tcl-8.6

#### The Tcl source code directory.  This defaults to the same value as
#    TCLDIR macro (above), which may not be correct.  This value will
#    only be used if the FOSSIL_TCL_SOURCE macro is defined.
#
TCLSRCDIR = $(TCLDIR)

#### The Tcl include and library directories.  These values will only be
#    used if the FOSSIL_TCL_SOURCE macro is not defined.
#
TCLINCDIR = $(TCLDIR)/include
TCLLIBDIR = $(TCLDIR)/lib

#### Tcl: Which Tcl library do we want to use (8.4, 8.5, 8.6, etc)?
#
ifdef FOSSIL_ENABLE_TCL_STUBS
LIBTCL = -ltclstub86
TCLTARGET = libtclstub86.a
else
LIBTCL = -ltcl86
TCLTARGET = binaries
endif

#### C Compile and options for use in building executables that
#    will run on the target platform.  This is usually the same
#    as BCC, unless you are cross-compiling.  This C compiler builds
#    the finished binary for fossil.  The BCC compiler above is used
#    for building intermediate code-generator tools.
#
TCC = $(PREFIX)gcc -Os -Wall -L$(ZLIBDIR) -I$(ZINCDIR)

#### Add the necessary command line options to build with debugging
#    symbols, if enabled.
#
ifdef FOSSIL_ENABLE_SYMBOLS
TCC += -g
endif

#### Compile resources for use in building executables that will run
#    on the target platform.
#
RCC = $(PREFIX)windres -I$(SRCDIR) -I$(ZINCDIR)

# With HTTPS support
ifdef FOSSIL_ENABLE_SSL
TCC += -L$(OPENSSLLIBDIR) -I$(OPENSSLINCDIR)
RCC += -I$(OPENSSLINCDIR)
endif

# With Tcl support
ifdef FOSSIL_ENABLE_TCL
ifdef FOSSIL_TCL_SOURCE
TCC += -L$(TCLSRCDIR)/win -I$(TCLSRCDIR)/generic -I$(TCLSRCDIR)/win
RCC += -I$(TCLSRCDIR)/generic -I$(TCLSRCDIR)/win
else
TCC += -L$(TCLLIBDIR) -I$(TCLINCDIR)
RCC += -I$(TCLINCDIR)
endif
endif

# With MinGW command line handling workaround
ifdef BROKEN_MINGW_CMDLINE
TCC += -DBROKEN_MINGW_CMDLINE=1
RCC += -DBROKEN_MINGW_CMDLINE=1
endif

# With HTTPS support
ifdef FOSSIL_ENABLE_SSL
TCC += -DFOSSIL_ENABLE_SSL=1
RCC += -DFOSSIL_ENABLE_SSL=1
endif

# With Tcl support
ifdef FOSSIL_ENABLE_TCL
TCC += -DFOSSIL_ENABLE_TCL=1
RCC += -DFOSSIL_ENABLE_TCL=1
# Either statically linked or via stubs
ifdef FOSSIL_ENABLE_TCL_STUBS
TCC += -DFOSSIL_ENABLE_TCL_STUBS=1 -DUSE_TCL_STUBS
RCC += -DFOSSIL_ENABLE_TCL_STUBS=1 -DUSE_TCL_STUBS
else
TCC += -DSTATIC_BUILD
RCC += -DSTATIC_BUILD
endif
endif

# With JSON support
ifdef FOSSIL_ENABLE_JSON
TCC += -DFOSSIL_ENABLE_JSON=1
RCC += -DFOSSIL_ENABLE_JSON=1
endif

#### We add the -static option here so that we can build a static
#    executable that will run in a chroot jail.
#
LIB = -static

# MinGW: If available, use the Unicode capable runtime startup code.
ifndef BROKEN_MINGW_CMDLINE
LIB += -municode
endif

# OpenSSL: Add the necessary libraries required, if enabled.
ifdef FOSSIL_ENABLE_SSL
LIB += -lssl -lcrypto -lgdi32
endif

# Tcl: Add the necessary libraries required, if enabled.
ifdef FOSSIL_ENABLE_TCL
LIB += $(LIBTCL)
endif

#### Extra arguments for linking the finished binary.  Fossil needs
#    to link against the Z-Lib compression library.  There are no
#    other mandatory dependencies.
#
LIB += -lmingwex -lz

#### These libraries MUST appear in the same order as they do for Tcl
#    or linking with it will not work (exact reason unknown).
#
ifdef FOSSIL_ENABLE_TCL
ifdef FOSSIL_ENABLE_TCL_STUBS
LIB += -lkernel32 -lws2_32
else
LIB += -lnetapi32 -lkernel32 -luser32 -ladvapi32 -lws2_32
endif
else
LIB += -lkernel32 -lws2_32
endif

#### Tcl shell for use in running the fossil test suite.  This is only
#    used for testing.
#
TCLSH = tclsh

#### Nullsoft installer MakeNSIS location
#
MAKENSIS = "$(ProgramFiles)\NSIS\MakeNSIS.exe"

#### Include a configuration file that can override any one of these settings.
#
-include config.w32

# STOP HERE
# You should not need to change anything below this line
#--------------------------------------------------------
XTCC = $(TCC) $(CFLAGS) -I. -I$(SRCDIR)
}
writeln -nonewline "SRC ="
foreach s [lsort $src] {
  writeln -nonewline " \\\n  \$(SRCDIR)/$s.c"
}
writeln "\n"
writeln -nonewline "TRANS_SRC ="
foreach s [lsort $src] {
  writeln -nonewline " \\\n  \$(OBJDIR)/${s}_.c"
}
writeln "\n"
writeln -nonewline "OBJ ="
foreach s [lsort $src] {
  writeln -nonewline " \\\n \$(OBJDIR)/$s.o"
}
writeln "\n"
writeln "APPNAME = ${name}.exe"
writeln {
#### If the USE_WINDOWS variable exists, it is assumed that we are building
#    inside of a Windows-style shell; otherwise, it is assumed that we are
#    building inside of a Unix-style shell.  Note that the "move" command is
#    broken when attempting to use it from the Windows shell via MinGW make
#    because the SHELL variable is only used for certain commands that are
#    recognized internally by make.
#
ifdef USE_WINDOWS
TRANSLATE   = $(subst /,\,$(OBJDIR)/translate)
MAKEHEADERS = $(subst /,\,$(OBJDIR)/makeheaders)
MKINDEX     = $(subst /,\,$(OBJDIR)/mkindex)
VERSION     = $(subst /,\,$(OBJDIR)/version)
CP          = copy
MV          = copy
RM          = del /Q
MKDIR       = -mkdir
RMDIR       = rmdir /S /Q
else
TRANSLATE   = $(OBJDIR)/translate
MAKEHEADERS = $(OBJDIR)/makeheaders
MKINDEX     = $(OBJDIR)/mkindex
VERSION     = $(OBJDIR)/version
CP          = cp
MV          = mv
RM          = rm -f
MKDIR       = -mkdir -p
RMDIR       = rm -rf
endif}

writeln {
all:	$(OBJDIR) $(APPNAME)

$(OBJDIR)/fossil.o:	$(SRCDIR)/../win/fossil.rc $(OBJDIR)/VERSION.h
ifdef USE_WINDOWS
	$(CP) $(subst /,\,$(SRCDIR)\..\win\fossil.rc) $(subst /,\,$(OBJDIR))
	$(CP) $(subst /,\,$(SRCDIR)\..\win\fossil.ico) $(subst /,\,$(OBJDIR))
else
	$(CP) $(SRCDIR)/../win/fossil.rc $(OBJDIR)
	$(CP) $(SRCDIR)/../win/fossil.ico $(OBJDIR)
endif
	$(RCC) $(OBJDIR)/fossil.rc -o $(OBJDIR)/fossil.o

install:	$(OBJDIR) $(APPNAME)
ifdef USE_WINDOWS
	$(MKDIR) $(subst /,\,$(INSTALLDIR))
	$(MV) $(subst /,\,$(APPNAME)) $(subst /,\,$(INSTALLDIR))
else
	$(MKDIR) $(INSTALLDIR)
	$(MV) $(APPNAME) $(INSTALLDIR)
endif

$(OBJDIR):
ifdef USE_WINDOWS
	$(MKDIR) $(subst /,\,$(OBJDIR))
else
	$(MKDIR) $(OBJDIR)
endif

$(OBJDIR)/translate:	$(SRCDIR)/translate.c
	$(BCC) -o $(OBJDIR)/translate $(SRCDIR)/translate.c

$(OBJDIR)/makeheaders:	$(SRCDIR)/makeheaders.c
	$(BCC) -o $(OBJDIR)/makeheaders $(SRCDIR)/makeheaders.c

$(OBJDIR)/mkindex:	$(SRCDIR)/mkindex.c
	$(BCC) -o $(OBJDIR)/mkindex $(SRCDIR)/mkindex.c

$(VERSION): $(SRCDIR)/mkversion.c
	$(BCC) -o $(OBJDIR)/version $(SRCDIR)/mkversion.c

# WARNING. DANGER. Running the test suite modifies the repository the
# build is done from, i.e. the checkout belongs to. Do not sync/push
# the repository after running the tests.
test:	$(OBJDIR) $(APPNAME)
	$(TCLSH) $(SRCDIR)/../test/tester.tcl $(APPNAME)

$(OBJDIR)/VERSION.h:	$(SRCDIR)/../manifest.uuid $(SRCDIR)/../manifest $(VERSION)
	$(VERSION) $(SRCDIR)/../manifest.uuid $(SRCDIR)/../manifest $(SRCDIR)/../VERSION >$(OBJDIR)/VERSION.h

EXTRAOBJ = \
  $(OBJDIR)/sqlite3.o \
  $(OBJDIR)/shell.o \
  $(OBJDIR)/th.o \
  $(OBJDIR)/th_lang.o \
  $(OBJDIR)/cson_amalgamation.o

ifdef FOSSIL_ENABLE_TCL
EXTRAOBJ +=  $(OBJDIR)/th_tcl.o
endif

zlib:
	$(MAKE) -C $(ZLIBDIR) PREFIX=$(PREFIX) -f win32/Makefile.gcc libz.a

openssl:	zlib
	cd $(OPENSSLLIBDIR);./Configure --cross-compile-prefix=$(PREFIX) --with-zlib-lib=$(PWD)/$(ZLIBDIR) --with-zlib-include=$(PWD)/$(ZLIBDIR) zlib mingw
	$(MAKE) -C $(OPENSSLLIBDIR) build_libs

tcl:
	cd $(TCLSRCDIR)/win;./configure
	$(MAKE) -C $(TCLSRCDIR)/win $(TCLTARGET)

$(APPNAME):	$(OBJDIR)/headers $(OBJ) $(EXTRAOBJ) $(OBJDIR)/fossil.o zlib
	$(TCC) -o $(APPNAME) $(OBJ) $(EXTRAOBJ) $(LIB) $(OBJDIR)/fossil.o

# This rule prevents make from using its default rules to try build
# an executable named "manifest" out of the file named "manifest.c"
#
$(SRCDIR)/../manifest:
	# noop

clean:
ifdef USE_WINDOWS
	$(RM) $(subst /,\,$(APPNAME))
	$(RMDIR) $(subst /,\,$(OBJDIR))
else
	$(RM) $(APPNAME)
	$(RMDIR) $(OBJDIR)
endif

setup: $(OBJDIR) $(APPNAME)
	$(MAKENSIS) ./fossil.nsi
}

set mhargs {}
foreach s [lsort $src] {
  if {[string length $mhargs] > 0} {append mhargs " \\\n\t\t"}
  append mhargs "\$(OBJDIR)/${s}_.c:\$(OBJDIR)/$s.h"
  set extra_h($s) {}
}
append mhargs " \\\n\t\t\$(SRCDIR)/sqlite3.h"
append mhargs " \\\n\t\t\$(SRCDIR)/th.h"
append mhargs " \\\n\t\t\$(OBJDIR)/VERSION.h"
writeln "\$(OBJDIR)/page_index.h: \$(TRANS_SRC) \$(OBJDIR)/mkindex"
writeln "\t\$(MKINDEX) \$(TRANS_SRC) >$@\n"
writeln "\$(OBJDIR)/headers:\t\$(OBJDIR)/page_index.h \$(OBJDIR)/makeheaders \$(OBJDIR)/VERSION.h"
writeln "\t\$(MAKEHEADERS) $mhargs"
writeln "\techo Done >\$(OBJDIR)/headers\n"
writeln "\$(OBJDIR)/headers: Makefile\n"
writeln "Makefile:\n"
set extra_h(main) \$(OBJDIR)/page_index.h

foreach s [lsort $src] {
  writeln "\$(OBJDIR)/${s}_.c:\t\$(SRCDIR)/$s.c \$(OBJDIR)/translate"
  writeln "\t\$(TRANSLATE) \$(SRCDIR)/$s.c >\$(OBJDIR)/${s}_.c\n"
  writeln "\$(OBJDIR)/$s.o:\t\$(OBJDIR)/${s}_.c \$(OBJDIR)/$s.h $extra_h($s) \$(SRCDIR)/config.h"
  writeln "\t\$(XTCC) -o \$(OBJDIR)/$s.o -c \$(OBJDIR)/${s}_.c\n"
  writeln "\$(OBJDIR)/${s}.h:\t\$(OBJDIR)/headers\n"
}


writeln "\$(OBJDIR)/sqlite3.o:\t\$(SRCDIR)/sqlite3.c"
set opt $SQLITE_OPTIONS
writeln "\t\$(XTCC) $opt -c \$(SRCDIR)/sqlite3.c -o \$(OBJDIR)/sqlite3.o\n"

set opt {}
writeln "\$(OBJDIR)/cson_amalgamation.o:\t\$(SRCDIR)/cson_amalgamation.c"
writeln "\t\$(XTCC) $opt -c \$(SRCDIR)/cson_amalgamation.c -o \$(OBJDIR)/cson_amalgamation.o\n"
writeln "\$(OBJDIR)/json.o \$(OBJDIR)/json_artifact.o \$(OBJDIR)/json_branch.o \$(OBJDIR)/json_config.o \$(OBJDIR)/json_diff.o \$(OBJDIR)/json_dir.o \$(OBJDIR)/jsos_finfo.o \$(OBJDIR)/json_login.o \$(OBJDIR)/json_query.o \$(OBJDIR)/json_report.o \$(OBJDIR)/json_status.o \$(OBJDIR)/json_tag.o \$(OBJDIR)/json_timeline.o \$(OBJDIR)/json_user.o \$(OBJDIR)/json_wiki.o : \$(SRCDIR)/json_detail.h\n"

writeln "\$(OBJDIR)/shell.o:\t\$(SRCDIR)/shell.c \$(SRCDIR)/sqlite3.h"
set opt {-Dmain=sqlite3_shell}
append opt " -DSQLITE_OMIT_LOAD_EXTENSION=1"
writeln "\t\$(XTCC) $opt -c \$(SRCDIR)/shell.c -o \$(OBJDIR)/shell.o\n"

writeln "\$(OBJDIR)/th.o:\t\$(SRCDIR)/th.c"
writeln "\t\$(XTCC) -c \$(SRCDIR)/th.c -o \$(OBJDIR)/th.o\n"

writeln "\$(OBJDIR)/th_lang.o:\t\$(SRCDIR)/th_lang.c"
writeln "\t\$(XTCC) -c \$(SRCDIR)/th_lang.c -o \$(OBJDIR)/th_lang.o\n"

writeln {ifdef FOSSIL_ENABLE_TCL
$(OBJDIR)/th_tcl.o:	$(SRCDIR)/th_tcl.c
	$(XTCC) -c $(SRCDIR)/th_tcl.c -o $(OBJDIR)/th_tcl.o
endif}

close $output_file
#
# End of the win/Makefile.mingw output
##############################################################################
##############################################################################
##############################################################################
# Begin win/Makefile.dmc output
#
puts "building ../win/Makefile.dmc"
set output_file [open ../win/Makefile.dmc w]
fconfigure $output_file -translation binary

writeln {#
##############################################################################
# WARNING: DO NOT EDIT, AUTOMATICALLY GENERATED FILE (SEE "src/makemake.tcl")
##############################################################################
#
# This file is automatically generated.  Instead of editing this
# file, edit "makemake.tcl" then run "tclsh makemake.tcl"
# to regenerate this file.
#
B      = ..
SRCDIR = $B\src
OBJDIR = .
O      = .obj
E      = .exe


# Maybe DMDIR, SSL or INCL needs adjustment
DMDIR  = c:\DM
INCL   = -I. -I$(SRCDIR) -I$B\win\include -I$(DMDIR)\extra\include

#SSL   =  -DFOSSIL_ENABLE_SSL=1
SSL    =

CFLAGS = -o
BCC    = $(DMDIR)\bin\dmc $(CFLAGS)
TCC    = $(DMDIR)\bin\dmc $(CFLAGS) $(DMCDEF) $(SSL) $(INCL)
LIBS   = $(DMDIR)\extra\lib\ zlib wsock32 advapi32
}
writeln "SQLITE_OPTIONS = $SQLITE_OPTIONS\n"
writeln -nonewline "SRC   = "
foreach s [lsort $src] {
  writeln -nonewline "${s}_.c "
}
writeln "\n"
writeln -nonewline "OBJ   = "
foreach s [lsort $src] {
  writeln -nonewline "\$(OBJDIR)\\$s\$O "
}
writeln "\$(OBJDIR)\\shell\$O \$(OBJDIR)\\sqlite3\$O \$(OBJDIR)\\th\$O \$(OBJDIR)\\th_lang\$O "
writeln {

RC=$(DMDIR)\bin\rcc
RCFLAGS=-32 -w1 -I$(SRCDIR) /D__DMC__

APPNAME = $(OBJDIR)\fossil$(E)

all: $(APPNAME)

$(APPNAME) : translate$E mkindex$E headers  $(OBJ) $(OBJDIR)\link
	cd $(OBJDIR) 
	$(DMDIR)\bin\link @link

$(OBJDIR)\fossil.res:	$B\win\fossil.rc
	$(RC) $(RCFLAGS) -o$@ $**

$(OBJDIR)\link: $B\win\Makefile.dmc $(OBJDIR)\fossil.res}
writeln -nonewline "\t+echo "
foreach s [lsort $src] {
  writeln -nonewline "$s "
}
writeln "shell sqlite3 th th_lang > \$@"
writeln "\t+echo fossil >> \$@"
writeln "\t+echo fossil >> \$@"
writeln "\t+echo \$(LIBS) >> \$@"
writeln "\t+echo. >> \$@"
writeln "\t+echo fossil >> \$@"

writeln {
translate$E: $(SRCDIR)\translate.c
	$(BCC) -o$@ $**

makeheaders$E: $(SRCDIR)\makeheaders.c
	$(BCC) -o$@ $**

mkindex$E: $(SRCDIR)\mkindex.c
	$(BCC) -o$@ $**

version$E: $B\src\mkversion.c
	$(BCC) -o$@ $**

$(OBJDIR)\shell$O : $(SRCDIR)\shell.c
	$(TCC) -o$@ -c -Dmain=sqlite3_shell $(SQLITE_OPTIONS) $**

$(OBJDIR)\sqlite3$O : $(SRCDIR)\sqlite3.c
	$(TCC) -o$@ -c $(SQLITE_OPTIONS) $**

$(OBJDIR)\th$O : $(SRCDIR)\th.c
	$(TCC) -o$@ -c $**

$(OBJDIR)\th_lang$O : $(SRCDIR)\th_lang.c
	$(TCC) -o$@ -c $**

$(OBJDIR)\cson_amalgamation.h : $(SRCDIR)\cson_amalgamation.h
	cp $@ $@

VERSION.h : version$E $B\manifest.uuid $B\manifest $B\VERSION
	+$** > $@

page_index.h: mkindex$E $(SRC) 
	+$** > $@

clean:
	-del $(OBJDIR)\*.obj
	-del *.obj *_.c *.h *.map

realclean:
	-del $(APPNAME) translate$E mkindex$E makeheaders$E mkversion$E

$(OBJDIR)\json$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_artifact$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_branch$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_config$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_diff$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_dir$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_finfo$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_login$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_query$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_report$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_status$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_tag$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_timeline$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_user$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_wiki$O : $(SRCDIR)\json_detail.h


}
foreach s [lsort $src] {
  writeln "\$(OBJDIR)\\$s\$O : ${s}_.c ${s}.h"
  writeln "\t\$(TCC) -o\$@ -c ${s}_.c\n"
  writeln "${s}_.c : \$(SRCDIR)\\$s.c"
  writeln "\t+translate\$E \$** > \$@\n"
}

writeln -nonewline "headers: makeheaders\$E page_index.h VERSION.h\n\t +makeheaders\$E "
foreach s [lsort $src] {
  writeln -nonewline "${s}_.c:$s.h "
}
writeln "\$(SRCDIR)\\sqlite3.h \$(SRCDIR)\\th.h VERSION.h \$(SRCDIR)\\cson_amalgamation.h"
writeln "\t@copy /Y nul: headers"

close $output_file
#
# End of the win/Makefile.dmc output
##############################################################################
##############################################################################
##############################################################################
# Begin win/Makefile.msc output
#
puts "building ../win/Makefile.msc"
set output_file [open ../win/Makefile.msc w]
fconfigure $output_file -translation binary

writeln {#
##############################################################################
# WARNING: DO NOT EDIT, AUTOMATICALLY GENERATED FILE (SEE "src/makemake.tcl")
##############################################################################
#
# This file is automatically generated.  Instead of editing this
# file, edit "makemake.tcl" then run "tclsh makemake.tcl"
# to regenerate this file.
#
B      = ..
SRCDIR = $B\src
OBJDIR = .
OX     = .
O      = .obj
E      = .exe

# Uncomment to enable JSON API
# FOSSIL_ENABLE_JSON = 1

# Uncomment to enable SSL support
# FOSSIL_ENABLE_SSL = 1

!ifdef FOSSIL_ENABLE_SSL
SSLINCDIR = $(B)\compat\openssl-1.0.1e\include
SSLLIBDIR = $(B)\compat\openssl-1.0.1e\out32
SSLLIB    = ssleay32.lib libeay32.lib user32.lib gdi32.lib
!endif

# zlib options
ZINCDIR = $(B)\compat\zlib
ZLIBDIR = $(B)\compat\zlib
ZLIB    = zlib.lib

INCL   = -I. -I$(SRCDIR) -I$B\win\include -I$(ZINCDIR)

!ifdef FOSSIL_ENABLE_SSL
INCL   = $(INCL) -I$(SSLINCDIR)
!endif

CFLAGS = -nologo -MT -O2
BCC    = $(CC) $(CFLAGS)
TCC    = $(CC) -c $(CFLAGS) $(MSCDEF) $(INCL)
RCC    = rc -D_WIN32 -D_MSC_VER $(MSCDEF) $(INCL)
LIBS   = $(ZLIB) ws2_32.lib advapi32.lib
LIBDIR = -LIBPATH:$(ZLIBDIR)

!ifdef FOSSIL_ENABLE_JSON
TCC = $(TCC) -DFOSSIL_ENABLE_JSON=1
RCC = $(RCC) -DFOSSIL_ENABLE_JSON=1
!endif

!ifdef FOSSIL_ENABLE_SSL
TCC    = $(TCC) -DFOSSIL_ENABLE_SSL=1
RCC    = $(RCC) -DFOSSIL_ENABLE_SSL=1
LIBS   = $(LIBS) $(SSLLIB)
LIBDIR = $(LIBDIR) -LIBPATH:$(SSLLIBDIR)
!endif
}
regsub -all {[-]D} $SQLITE_OPTIONS {/D} MSC_SQLITE_OPTIONS
set j " \\\n                 "
writeln "SQLITE_OPTIONS = [join $MSC_SQLITE_OPTIONS $j]\n"
writeln -nonewline "SRC   = "
set i 0
foreach s [lsort $src] {
  if {$i > 0} {
    writeln " \\"
    writeln -nonewline "        "
  }
  writeln -nonewline "${s}_.c"; incr i
}
writeln "\n"
set AdditionalObj [list shell sqlite3 th th_lang cson_amalgamation]
writeln -nonewline "OBJ   = "
set i 0
foreach s [lsort [concat $src $AdditionalObj]] {
  if {$i > 0} {
    writeln " \\"
    writeln -nonewline "        "
  }
  writeln -nonewline "\$(OX)\\$s\$O"; incr i
}
writeln " \\"
writeln -nonewline "        \$(OX)\\fossil.res\n"
writeln {
APPNAME = $(OX)\fossil$(E)

all: $(OX) $(APPNAME)

zlib:
	@echo Building zlib from "$(ZLIBDIR)"...
	@pushd "$(ZLIBDIR)" && nmake /f win32\Makefile.msc $(ZLIB) && popd

$(APPNAME) : translate$E mkindex$E headers $(OBJ) $(OX)\linkopts zlib
	cd $(OX) 
	link /NODEFAULTLIB:msvcrt -OUT:$@ $(LIBDIR) Wsetargv.obj fossil.res @linkopts

$(OX)\linkopts: $B\win\Makefile.msc}
set redir {>}
foreach s [lsort [concat $src $AdditionalObj]] {
  writeln "\techo \$(OX)\\$s.obj $redir \$@"
  set redir {>>}
}
writeln "\techo \$(LIBS) >> \$@\n\n"

writeln {

$(OX):
	@-mkdir $@

translate$E: $(SRCDIR)\translate.c
	$(BCC) $**

makeheaders$E: $(SRCDIR)\makeheaders.c
	$(BCC) $**

mkindex$E: $(SRCDIR)\mkindex.c
	$(BCC) $**

mkversion$E: $B\src\mkversion.c
	$(BCC) $**

$(OX)\shell$O : $(SRCDIR)\shell.c
	$(TCC) /Fo$@ /Dmain=sqlite3_shell $(SQLITE_OPTIONS) -c $(SRCDIR)\shell.c

$(OX)\sqlite3$O : $(SRCDIR)\sqlite3.c
	$(TCC) /Fo$@ -c $(SQLITE_OPTIONS) $**

$(OX)\th$O : $(SRCDIR)\th.c
	$(TCC) /Fo$@ -c $**

$(OX)\th_lang$O : $(SRCDIR)\th_lang.c
	$(TCC) /Fo$@ -c $**

VERSION.h : mkversion$E $B\manifest.uuid $B\manifest $B\VERSION
	$** > $@
$(OX)\cson_amalgamation$O : $(SRCDIR)\cson_amalgamation.c
	$(TCC) /Fo$@ -c $**

page_index.h: mkindex$E $(SRC) 
	$** > $@

clean:
	-del $(OX)\*.obj
	-del *.obj
	-del *_.c
	-del *.h
	-del *.map
	-del *.manifest
	-del headers
	-del linkopts
	-del *.res

realclean: clean
	-del $(APPNAME)
	-del translate$E
	-del mkindex$E
	-del makeheaders$E
	-del mkversion$E

$(OBJDIR)\json$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_artifact$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_branch$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_config$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_diff$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_dir$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_finfo$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_login$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_query$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_report$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_status$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_tag$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_timeline$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_user$O : $(SRCDIR)\json_detail.h
$(OBJDIR)\json_wiki$O : $(SRCDIR)\json_detail.h

}
foreach s [lsort $src] {
  writeln "\$(OX)\\$s\$O : ${s}_.c ${s}.h"
  writeln "\t\$(TCC) /Fo\$@ -c ${s}_.c\n"
  writeln "${s}_.c : \$(SRCDIR)\\$s.c"
  writeln "\ttranslate\$E \$** > \$@\n"
}

writeln "fossil.res : \$B\\win\\fossil.rc"
writeln "\t\$(RCC)  -fo \$@ \$**"

writeln "headers: makeheaders\$E page_index.h VERSION.h"
writeln -nonewline "\tmakeheaders\$E "
set i 0
foreach s [lsort $src] {
  if {$i > 0} {
    writeln " \\"
    writeln -nonewline "\t\t\t"
  }
  writeln -nonewline "${s}_.c:$s.h"; incr i
}
writeln " \\\n\t\t\t\$(SRCDIR)\\sqlite3.h \\"
writeln "\t\t\t\$(SRCDIR)\\th.h \\"
writeln "\t\t\tVERSION.h \\"
writeln "\t\t\t\$(SRCDIR)\\cson_amalgamation.h"
writeln "\t@copy /Y nul: headers"


close $output_file
#
# End of the win/Makefile.msc output
##############################################################################
##############################################################################
##############################################################################
# Begin win/Makefile.PellesCGMake output
#
puts "building ../win/Makefile.PellesCGMake"
set output_file [open ../win/Makefile.PellesCGMake w]
fconfigure $output_file -translation binary

writeln {#
##############################################################################
# WARNING: DO NOT EDIT, AUTOMATICALLY GENERATED FILE (SEE "src/makemake.tcl")
##############################################################################
#
# This file is automatically generated.  Instead of editing this
# file, edit "makemake.tcl" then run "tclsh makemake.tcl"
# to regenerate this file.
#
# HowTo
# -----
#
# This is a Makefile to compile fossil with PellesC from
#  http://www.smorgasbordet.com/pellesc/index.htm
# In addition to the Compiler envrionment, you need
#  gmake from http://sourceforge.net/projects/unxutils/, Pelles make version
#        couldn't handle the complex dependencies in this build
#  zlib sources
# Then you do
# 1. create a directory PellesC in the project root directory
# 2. Change the variables PellesCDir/ZLIBSRCDIR to the path of your installation
# 3. open a dos prompt window and change working directory into PellesC (step 1)
# 4. run gmake -f ..\win\Makefile.PellesCGMake
#
# this file is tested with
#   PellesC         5.00.13
#   gmake           3.80
#   zlib sources    1.2.5
#   Windows XP SP 2
# and
#   PellesC         6.00.4
#   gmake           3.80
#   zlib sources    1.2.5
#   Windows 7 Home Premium
#  

#  
PellesCDir=c:\Programme\PellesC

# Select between 32/64 bit code, default is 32 bit
#TARGETVERSION=64

ifeq ($(TARGETVERSION),64)
# 64 bit version
TARGETMACHINE_CC=amd64
TARGETMACHINE_LN=amd64
TARGETEXTEND=64
else
# 32 bit version
TARGETMACHINE_CC=x86
TARGETMACHINE_LN=ix86
TARGETEXTEND=
endif

# define the project directories
B=..
SRCDIR=$(B)/src/
WINDIR=$(B)/win/
ZLIBSRCDIR=../../zlib/

# define linker command and options
LINK=$(PellesCDir)/bin/polink.exe
LINKFLAGS=-subsystem:console -machine:$(TARGETMACHINE_LN) /LIBPATH:$(PellesCDir)\lib\win$(TARGETEXTEND) /LIBPATH:$(PellesCDir)\lib kernel32.lib advapi32.lib delayimp$(TARGETEXTEND).lib Wsock32.lib Crtmt$(TARGETEXTEND).lib

# define standard C-compiler and flags, used to compile
# the fossil binary. Some special definitions follow for
# special files follow
CC=$(PellesCDir)\bin\pocc.exe
DEFINES=-D_pgmptr=g.argv[0]
CCFLAGS=-T$(TARGETMACHINE_CC)-coff -Ot -W2 -Gd -Go -Ze -MT $(DEFINES)
INCLUDE=/I $(PellesCDir)\Include\Win /I $(PellesCDir)\Include /I $(ZLIBSRCDIR) /I $(SRCDIR)

# define commands for building the windows resource files
RESOURCE=fossil.res
RC=$(PellesCDir)\bin\porc.exe
RCFLAGS=$(INCLUDE) -D__POCC__=1 -D_M_X$(TARGETVERSION)

# define the special utilities files, needed to generate
# the automatically generated source files
UTILS=translate.exe mkindex.exe makeheaders.exe
UTILS_OBJ=$(UTILS:.exe=.obj)
UTILS_SRC=$(foreach uf,$(UTILS),$(SRCDIR)$(uf:.exe=.c))

# define the sqlite files, which need special flags on compile
SQLITESRC=sqlite3.c
ORIGSQLITESRC=$(foreach sf,$(SQLITESRC),$(SRCDIR)$(sf))
SQLITEOBJ=$(foreach sf,$(SQLITESRC),$(sf:.c=.obj))
SQLITEDEFINES=-DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_FILE_FORMAT=4 -Dlocaltime=fossil_localtime -DSQLITE_ENABLE_LOCKING_STYLE=0 -DSQLITE_WIN32_NO_ANSI

# define the sqlite shell files, which need special flags on compile
SQLITESHELLSRC=shell.c
ORIGSQLITESHELLSRC=$(foreach sf,$(SQLITESHELLSRC),$(SRCDIR)$(sf))
SQLITESHELLOBJ=$(foreach sf,$(SQLITESHELLSRC),$(sf:.c=.obj))
SQLITESHELLDEFINES=-Dmain=sqlite3_shell -DSQLITE_OMIT_LOAD_EXTENSION=1 -Dsqlite3_strglob=strglob

# define the th scripting files, which need special flags on compile
THSRC=th.c th_lang.c
ORIGTHSRC=$(foreach sf,$(THSRC),$(SRCDIR)$(sf))
THOBJ=$(foreach sf,$(THSRC),$(sf:.c=.obj))

# define the zlib files, needed by this compile
ZLIBSRC=adler32.c compress.c crc32.c deflate.c gzclose.c gzlib.c gzread.c gzwrite.c infback.c inffast.c inflate.c inftrees.c trees.c uncompr.c zutil.c
ORIGZLIBSRC=$(foreach sf,$(ZLIBSRC),$(ZLIBSRCDIR)$(sf))
ZLIBOBJ=$(foreach sf,$(ZLIBSRC),$(sf:.c=.obj))

# define all fossil sources, using the standard compile and
# source generation. These are all files in SRCDIR, which are not
# mentioned as special files above:
ORIGSRC=$(filter-out $(UTILS_SRC) $(ORIGTHSRC) $(ORIGSQLITESRC) $(ORIGSQLITESHELLSRC),$(wildcard $(SRCDIR)*.c))
SRC=$(subst $(SRCDIR),,$(ORIGSRC))
TRANSLATEDSRC=$(SRC:.c=_.c)
TRANSLATEDOBJ=$(TRANSLATEDSRC:.c=.obj)

# main target file is the application
APPLICATION=fossil.exe

# define the standard make target
.PHONY:	default
default:	page_index.h headers $(APPLICATION)

# symbolic target to generate the source generate utils
.PHONY:	utils
utils:	$(UTILS)

# link utils
$(UTILS) version.exe:	%.exe:	%.obj
	$(LINK) $(LINKFLAGS) -out:"$@" $<

# compiling standard fossil utils
$(UTILS_OBJ):	%.obj:	$(SRCDIR)%.c
	$(CC) $(CCFLAGS) $(INCLUDE) "$<" -Fo"$@"

# compile special windows utils
version.obj:	$(SRCDIR)mkversion.c
	$(CC) $(CCFLAGS) $(INCLUDE) "$<" -Fo"$@"

# generate the translated c-source files
$(TRANSLATEDSRC):	%_.c:	$(SRCDIR)%.c translate.exe
	translate.exe $< >$@

# generate the index source, containing all web references,..
page_index.h:	$(TRANSLATEDSRC) mkindex.exe
	mkindex.exe $(TRANSLATEDSRC) >$@

# extracting version info from manifest
VERSION.h:	version.exe ..\manifest.uuid ..\manifest ..\VERSION
	version.exe ..\manifest.uuid ..\manifest ..\VERSION  > $@

# generate the simplified headers
headers: makeheaders.exe page_index.h VERSION.h ../src/sqlite3.h ../src/th.h VERSION.h
	makeheaders.exe $(foreach ts,$(TRANSLATEDSRC),$(ts):$(ts:_.c=.h)) ../src/sqlite3.h ../src/th.h VERSION.h
	echo Done >$@

# compile C sources with relevant options

$(TRANSLATEDOBJ):	%_.obj:	%_.c %.h
	$(CC) $(CCFLAGS) $(INCLUDE) "$<" -Fo"$@"

$(SQLITEOBJ):	%.obj:	$(SRCDIR)%.c $(SRCDIR)%.h
	$(CC) $(CCFLAGS) $(SQLITEDEFINES) $(INCLUDE) "$<" -Fo"$@"

$(SQLITESHELLOBJ):	%.obj:	$(SRCDIR)%.c
	$(CC) $(CCFLAGS) $(SQLITESHELLDEFINES) $(INCLUDE) "$<" -Fo"$@"

$(THOBJ):	%.obj:	$(SRCDIR)%.c $(SRCDIR)th.h
	$(CC) $(CCFLAGS) $(INCLUDE) "$<" -Fo"$@"

$(ZLIBOBJ):	%.obj:	$(ZLIBSRCDIR)%.c
	$(CC) $(CCFLAGS) $(INCLUDE) "$<" -Fo"$@"

# create the windows resource with icon and version info
$(RESOURCE):	%.res:	../win/%.rc ../win/*.ico
	$(RC) $(RCFLAGS) $< -Fo"$@"

# link the application
$(APPLICATION):	$(TRANSLATEDOBJ) $(SQLITEOBJ) $(SQLITESHELLOBJ) $(THOBJ) $(ZLIBOBJ) headers $(RESOURCE)
	$(LINK) $(LINKFLAGS) -out:"$@" $(TRANSLATEDOBJ) $(SQLITEOBJ) $(SQLITESHELLOBJ) $(THOBJ) $(ZLIBOBJ) $(RESOURCE)

# cleanup

.PHONY: clean
clean:
	del /F $(TRANSLATEDOBJ) $(SQLITEOBJ) $(THOBJ) $(ZLIBOBJ) $(UTILS_OBJ) version.obj
	del /F $(TRANSLATEDSRC)
	del /F *.h headers
	del /F $(RESOURCE)

.PHONY: clobber
clobber: clean
	del /F *.exe
}
