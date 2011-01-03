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
  graph
  http
  http_socket
  http_transport
  import
  info
  login
  main
  manifest
  md5
  merge
  merge3
  name
  pivot
  popen
  pqueue
  printf
  rebuild
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
  th_main
  timeline
  tkt
  tktsetup
  undo
  update
  url
  user
  verify
  vfile
  wiki
  wikiformat
  winhttp
  xfer
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

writeln {# DO NOT EDIT
#
# This file is automatically generated.  Instead of editing this
# file, edit "makemake.tcl" then run "tclsh makemake.tcl"
# to regenerate this file.
#
# This file is included by primary Makefile.
#

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
writeln "APPNAME = $name\$(E)"
writeln "\n"

writeln {
all:	$(OBJDIR) $(APPNAME)

install:	$(APPNAME)
	mv $(APPNAME) $(INSTALLDIR)

$(OBJDIR):
	-mkdir $(OBJDIR)

$(OBJDIR)/translate:	$(SRCDIR)/translate.c
	$(BCC) -o $(OBJDIR)/translate $(SRCDIR)/translate.c

$(OBJDIR)/makeheaders:	$(SRCDIR)/makeheaders.c
	$(BCC) -o $(OBJDIR)/makeheaders $(SRCDIR)/makeheaders.c

$(OBJDIR)/mkindex:	$(SRCDIR)/mkindex.c
	$(BCC) -o $(OBJDIR)/mkindex $(SRCDIR)/mkindex.c

# WARNING. DANGER. Running the testsuite modifies the repository the
# build is done from, i.e. the checkout belongs to. Do not sync/push
# the repository after running the tests.
test:	$(APPNAME)
	$(TCLSH) test/tester.tcl $(APPNAME)

$(OBJDIR)/VERSION.h:	$(SRCDIR)/../manifest.uuid $(SRCDIR)/../manifest
	awk '{ printf "#define MANIFEST_UUID \"%s\"\n", $$1}' \
		$(SRCDIR)/../manifest.uuid >$(OBJDIR)/VERSION.h
	awk '{ printf "#define MANIFEST_VERSION \"[%.10s]\"\n", $$1}' \
		$(SRCDIR)/../manifest.uuid >>$(OBJDIR)/VERSION.h
	awk '$$1=="D"{printf "#define MANIFEST_DATE \"%s %s\"\n",\
		substr($$2,1,10),substr($$2,12)}' \
		$(SRCDIR)/../manifest >>$(OBJDIR)/VERSION.h

EXTRAOBJ = \
  $(OBJDIR)/sqlite3.o \
  $(OBJDIR)/shell.o \
  $(OBJDIR)/th.o \
  $(OBJDIR)/th_lang.o

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
append mhargs " \$(OBJDIR)/VERSION.h"
writeln "\$(OBJDIR)/page_index.h: \$(TRANS_SRC) \$(OBJDIR)/mkindex"
writeln "\t\$(OBJDIR)/mkindex \$(TRANS_SRC) >$@"
writeln "\$(OBJDIR)/headers:\t\$(OBJDIR)/page_index.h \$(OBJDIR)/makeheaders \$(OBJDIR)/VERSION.h"
writeln "\t\$(OBJDIR)/makeheaders $mhargs"
writeln "\ttouch \$(OBJDIR)/headers"
writeln "\$(OBJDIR)/headers: Makefile"
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
append opt " -Dlocaltime=fossil_localtime"
append opt " -DSQLITE_ENABLE_LOCKING_STYLE=0"
writeln "\t\$(XTCC) $opt -c \$(SRCDIR)/sqlite3.c -o \$(OBJDIR)/sqlite3.o\n"

writeln "\$(OBJDIR)/shell.o:\t\$(SRCDIR)/shell.c"
set opt {-Dmain=sqlite3_shell}
append opt " -DSQLITE_OMIT_LOAD_EXTENSION=1"
writeln "\t\$(XTCC) $opt -c \$(SRCDIR)/shell.c -o \$(OBJDIR)/shell.o\n"

writeln "\$(OBJDIR)/th.o:\t\$(SRCDIR)/th.c"
writeln "\t\$(XTCC) -I\$(SRCDIR) -c \$(SRCDIR)/th.c -o \$(OBJDIR)/th.o\n"

writeln "\$(OBJDIR)/th_lang.o:\t\$(SRCDIR)/th_lang.c"
writeln "\t\$(XTCC) -I\$(SRCDIR) -c \$(SRCDIR)/th_lang.c -o \$(OBJDIR)/th_lang.o\n"

close $output_file
#
# End of the main.mk output
##############################################################################
##############################################################################
##############################################################################
# Begin win/Makefile.mingw
#
puts "building ../win/Makefile.mingw"
set output_file [open ../win/Makefile.mingw w]
fconfigure $output_file -translation binary

writeln {#!/usr/bin/make
#
# This is a makefile for us on windows using mingw.
#
#### The toplevel directory of the source tree.  Fossil can be built
#    in a directory that is separate from the source tree.  Just change
#    the following to point from the build directory to the src/ folder.
#
SRCDIR = src

#### The directory into which object code files should be written.
#
#
OBJDIR = wbld

#### C Compiler and options for use in building executables that
#    will run on the platform that is doing the build.  This is used
#    to compile code-generator programs as part of the build process.
#    See TCC below for the C compiler for building the finished binary.
#
BCC = gcc

#### Enable HTTPS support via OpenSSL (links to libssl and libcrypto)
#
# FOSSIL_ENABLE_SSL=1

#### The directory in which the zlib compression library is installed.
#
#
ZLIBDIR = /programs/gnuwin32

#### C Compile and options for use in building executables that 
#    will run on the target platform.  This is usually the same
#    as BCC, unless you are cross-compiling.  This C compiler builds
#    the finished binary for fossil.  The BCC compiler above is used
#    for building intermediate code-generator tools.
#
TCC = gcc -Os -Wall -DFOSSIL_I18N=0 -L$(ZLIBDIR)/lib -I$(ZLIBDIR)/include

# With HTTPS support
ifdef FOSSIL_ENABLE_SSL
TCC += -DFOSSIL_ENABLE_SSL=1
endif

#### Extra arguments for linking the finished binary.  Fossil needs
#    to link against the Z-Lib compression library.  There are no
#    other dependencies.  We sometimes add the -static option here
#    so that we can build a static executable that will run in a
#    chroot jail.
#
#LIB = -lz -lws2_32
LIB = -lmingwex -lz -lws2_32
# OpenSSL:
ifdef FOSSIL_ENABLE_SSL
LIB += -lcrypto -lssl
endif

#### Tcl shell for use in running the fossil testsuite.  This is only
#    used for testing.  If you do not run
#
TCLSH = tclsh

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
writeln {TRANSLATE   = $(subst /,\\,$(OBJDIR)/translate.exe)
MAKEHEADERS = $(subst /,\\,$(OBJDIR)/makeheaders.exe)
MKINDEX     = $(subst /,\\,$(OBJDIR)/mkindex.exe)
VERSION     = $(subst /,\\,$(OBJDIR)/version.exe)
}

writeln {
all:	$(OBJDIR) $(APPNAME)


install:	$(APPNAME)
	mv $(APPNAME) $(INSTALLDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/translate:	$(SRCDIR)/translate.c
	$(BCC) -o $(OBJDIR)/translate $(SRCDIR)/translate.c

$(OBJDIR)/makeheaders:	$(SRCDIR)/makeheaders.c
	$(BCC) -o $(OBJDIR)/makeheaders $(SRCDIR)/makeheaders.c

$(OBJDIR)/mkindex:	$(SRCDIR)/mkindex.c
	$(BCC) -o $(OBJDIR)/mkindex $(SRCDIR)/mkindex.c

$(VERSION): $(SRCDIR)/../win/version.c
	$(BCC) -o $(OBJDIR)/version $(SRCDIR)/../win/version.c

# WARNING. DANGER. Running the testsuite modifies the repository the
# build is done from, i.e. the checkout belongs to. Do not sync/push
# the repository after running the tests.
test:	$(APPNAME)
	$(TCLSH) test/tester.tcl $(APPNAME)

$(OBJDIR)/VERSION.h:	$(SRCDIR)/../manifest.uuid $(SRCDIR)/../manifest $(VERSION)
	$(VERSION) $(SRCDIR)/../manifest.uuid $(SRCDIR)/../manifest >$(OBJDIR)/VERSION.h

EXTRAOBJ = \
  $(OBJDIR)/sqlite3.o \
  $(OBJDIR)/shell.o \
  $(OBJDIR)/th.o \
  $(OBJDIR)/th_lang.o

$(APPNAME):	$(OBJDIR)/headers $(OBJ) $(EXTRAOBJ)
	$(TCC) -o $(APPNAME) $(OBJ) $(EXTRAOBJ) $(LIB)

# This rule prevents make from using its default rules to try build
# an executable named "manifest" out of the file named "manifest.c"
#
$(SRCDIR)/../manifest:	
	# noop

# Requires msys to be installed in addition to the mingw, for the "rm"
# command.  "del" will not work here because it is not a separate command
# but a MSDOS-shell builtin.
#
clean:	
	rm -rf $(OBJDIR) $(APPNAME)

}

set mhargs {}
foreach s [lsort $src] {
  append mhargs " \$(OBJDIR)/${s}_.c:\$(OBJDIR)/$s.h"
  set extra_h($s) {}
}
append mhargs " \$(SRCDIR)/sqlite3.h"
append mhargs " \$(SRCDIR)/th.h"
append mhargs " \$(OBJDIR)/VERSION.h"
writeln "\$(OBJDIR)/page_index.h: \$(TRANS_SRC) \$(OBJDIR)/mkindex"
writeln "\t\$(MKINDEX) \$(TRANS_SRC) >$@"
writeln "\$(OBJDIR)/headers:\t\$(OBJDIR)/page_index.h \$(OBJDIR)/makeheaders \$(OBJDIR)/VERSION.h"
writeln "\t\$(MAKEHEADERS) $mhargs"
writeln "\techo Done >\$(OBJDIR)/headers"
writeln ""
writeln "\$(OBJDIR)/headers: Makefile"
writeln "Makefile:"
set extra_h(main) \$(OBJDIR)/page_index.h

foreach s [lsort $src] {
  writeln "\$(OBJDIR)/${s}_.c:\t\$(SRCDIR)/$s.c \$(OBJDIR)/translate"
  writeln "\t\$(TRANSLATE) \$(SRCDIR)/$s.c >\$(OBJDIR)/${s}_.c\n"
  writeln "\$(OBJDIR)/$s.o:\t\$(OBJDIR)/${s}_.c \$(OBJDIR)/$s.h $extra_h($s) \$(SRCDIR)/config.h"
  writeln "\t\$(XTCC) -o \$(OBJDIR)/$s.o -c \$(OBJDIR)/${s}_.c\n"
  writeln "$s.h:\t\$(OBJDIR)/headers"
}


writeln "\$(OBJDIR)/sqlite3.o:\t\$(SRCDIR)/sqlite3.c"
set opt {-DSQLITE_OMIT_LOAD_EXTENSION=1}
append opt " -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_FILE_FORMAT=4"
#append opt " -DSQLITE_ENABLE_FTS3=1"
append opt " -Dlocaltime=fossil_localtime"
append opt " -DSQLITE_ENABLE_LOCKING_STYLE=0"
writeln "\t\$(XTCC) $opt -c \$(SRCDIR)/sqlite3.c -o \$(OBJDIR)/sqlite3.o\n"

writeln "\$(OBJDIR)/shell.o:\t\$(SRCDIR)/shell.c"
set opt {-Dmain=sqlite3_shell}
append opt " -DSQLITE_OMIT_LOAD_EXTENSION=1"
writeln "\t\$(XTCC) $opt -c \$(SRCDIR)/shell.c -o \$(OBJDIR)/shell.o\n"

writeln "\$(OBJDIR)/th.o:\t\$(SRCDIR)/th.c"
writeln "\t\$(XTCC) -I\$(SRCDIR) -c \$(SRCDIR)/th.c -o \$(OBJDIR)/th.o\n"

writeln "\$(OBJDIR)/th_lang.o:\t\$(SRCDIR)/th_lang.c"
writeln "\t\$(XTCC) -I\$(SRCDIR) -c \$(SRCDIR)/th_lang.c -o \$(OBJDIR)/th_lang.o\n"


close $output_file
#
# End of the main.mk output
##############################################################################
##############################################################################
##############################################################################
# Begin win/Makefile.dmc
#
puts "building ../win/Makefile.dmc"
set output_file [open ../win/Makefile.dmc w]
fconfigure $output_file -translation binary

writeln {# DO NOT EDIT
#
# This file is automatically generated.  Instead of editing this
# file, edit "makemake.tcl" then run "tclsh src/makemake.tcl"
# to regenerate this file.
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

I18N   =  -DFOSSIL_I18N=0

CFLAGS = -o 
BCC    = $(DMDIR)\bin\dmc $(CFLAGS)
TCC    = $(DMDIR)\bin\dmc $(CFLAGS) $(DMCDEF) $(I18N) $(SSL) $(INCL)
LIBS   = $(DMDIR)\extra\lib\ zlib wsock32
}
writeln -nonewline "SRC   = "
foreach s [lsort $src] {
  writeln -nonewline "${s}_.c "
}
writeln "\n"
writeln -nonewline "OBJ   = "
foreach s [lsort $src] {
  writeln -nonewline "\$(OBJDIR)\\$s\$O "
}
writeln "\$(OBJDIR)\\shell\$O \$(OBJDIR)\\sqlcmd\$O \$(OBJDIR)\\sqlite3\$O \$(OBJDIR)\\th\$O \$(OBJDIR)\\th_lang\$O "
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
writeln "shell sqlcmd sqlite3 th th_lang > \$@"
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

version$E: $B\win\version.c
	$(BCC) -o$@ $**

$(OBJDIR)\shell$O : $(SRCDIR)\shell.c
	$(TCC) -o$@ -c -Dmain=sqlite3_shell -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_FILE_FORMAT=4 -Dlocaltime=fossil_localtime -DSQLITE_ENABLE_LOCKING_STYLE=0 $**

$(OBJDIR)\sqlcmd$O : $(SRCDIR)\sqlcmd.c
	$(TCC) -o$@ -c -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_FILE_FORMAT=4 -Dlocaltime=fossil_localtime -DSQLITE_ENABLE_LOCKING_STYLE=0 $**

$(OBJDIR)\sqlite3$O : $(SRCDIR)\sqlite3.c
	$(TCC) -o$@ -c -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_FILE_FORMAT=4 -Dlocaltime=fossil_localtime -DSQLITE_ENABLE_LOCKING_STYLE=0 $**

$(OBJDIR)\th$O : $(SRCDIR)\th.c
	$(TCC) -o$@ -c $**

$(OBJDIR)\th_lang$O : $(SRCDIR)\th_lang.c
	$(TCC) -o$@ -c $**

VERSION.h : version$E $B\manifest.uuid $B\manifest
	+$** > $@

page_index.h: mkindex$E $(SRC) 
	+$** > $@

clean:
	-del $(OBJDIR)\*.obj
	-del *.obj *_.c *.h *.map

realclean:
	-del $(APPNAME) translate$E mkindex$E makeheaders$E version$E

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
writeln "\$(SRCDIR)\\sqlite3.h \$(SRCDIR)\\th.h VERSION.h"
writeln "\t@copy /Y nul: headers"

close $output_file
#
# End of the win/Makefile.dmc output
##############################################################################
##############################################################################
##############################################################################
# Begin win/Makefile.msc
#
puts "building ../win/Makefile.msc"
set output_file [open ../win/Makefile.msc w]
fconfigure $output_file -translation binary

writeln {# DO NOT EDIT
#
# This file is automatically generated.  Instead of editing this
# file, edit "makemake.tcl" then run "tclsh src/makemake.tcl"
# to regenerate this file.
B      = ..
SRCDIR = $B\src
OBJDIR = .
O      = .obj
E      = .exe

# Maybe MSCDIR, SSL, ZLIB, or INCL needs adjustment
MSCDIR = c:\msc

# Uncomment below for SSL support
SSL =
SSLLIB =
#SSL = -DFOSSIL_ENABLE_SSL=1
#SSLLIB  = ssleay32.lib libeay32.lib user32.lib gdi32.lib advapi32.lib

# zlib options
# When using precompiled from http://zlib.net/zlib125-dll.zip
#ZINCDIR = C:\zlib125-dll\include
#ZLIBDIR = C:\zlib125-dll\lib
#ZLIB    = zdll.lib
ZINCDIR = $(MSCDIR)\extra\include
ZLIBDIR = $(MSCDIR)\extra\lib
ZLIB    = zlib.lib

INCL   = -I. -I$(SRCDIR) -I$B\win\include -I$(MSCDIR)\extra\include -I$(ZINCDIR)

I18N   =  -DFOSSIL_I18N=0

CFLAGS = -nologo -MT -O2
BCC    = $(CC) $(CFLAGS)
TCC    = $(CC) -c $(CFLAGS) $(MSCDEF) $(I18N) $(SSL) $(INCL)
LIBS   = $(ZLIB) ws2_32.lib $(SSLLIB)
LIBDIR = -LIBPATH:$(MSCDIR)\extra\lib -LIBPATH:$(ZLIBDIR)
}
writeln -nonewline "SRC   = "
foreach s [lsort $src] {
  writeln -nonewline "${s}_.c "
}
writeln "\n"
writeln -nonewline "OBJ   = "
foreach s [lsort $src] {
  writeln -nonewline "\$(OBJDIR)\\$s\$O "
}
writeln "\$(OBJDIR)\\sqlite3\$O \$(OBJDIR)\\th\$O \$(OBJDIR)\\th_lang\$O "
writeln {

APPNAME = $(OBJDIR)\fossil$(E)

all: $(OBJDIR) $(APPNAME)

$(APPNAME) : translate$E mkindex$E headers $(OBJ) $(OBJDIR)\linkopts
	cd $(OBJDIR) 
	link -LINK -OUT:$@ $(LIBDIR) @linkopts

$(OBJDIR)\linkopts: $B\win\Makefile.msc}
writeln -nonewline "\techo "
foreach s [lsort $src] {
  writeln -nonewline "$s "
}
writeln "sqlite3 th th_lang > \$@"
writeln "\techo \$(LIBS) >> \$@\n\n"

writeln {

$(OBJDIR):
	@-mkdir $@

translate$E: $(SRCDIR)\translate.c
	$(BCC) $**

makeheaders$E: $(SRCDIR)\makeheaders.c
	$(BCC) $**

mkindex$E: $(SRCDIR)\mkindex.c
	$(BCC) $**

version$E: $B\win\version.c
	$(BCC) $**

$(OBJDIR)\sqlite3$O : $(SRCDIR)\sqlite3.c
	$(TCC) /Fo$@ -c -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_FILE_FORMAT=4 -Dlocaltime=fossil_localtime -DSQLITE_ENABLE_LOCKING_STYLE=0 $**

$(OBJDIR)\th$O : $(SRCDIR)\th.c
	$(TCC) /Fo$@ -c $**

$(OBJDIR)\th_lang$O : $(SRCDIR)\th_lang.c
	$(TCC) /Fo$@ -c $**

VERSION.h : version$E $B\manifest.uuid $B\manifest
	$** > $@

page_index.h: mkindex$E $(SRC) 
	$** > $@

clean:
	-del $(OBJDIR)\*.obj
	-del *.obj *_.c *.h *.map
	-del headers linkopts

realclean:
	-del $(APPNAME) translate$E mkindex$E makeheaders$E version$E

}
foreach s [lsort $src] {
  writeln "\$(OBJDIR)\\$s\$O : ${s}_.c ${s}.h"
  writeln "\t\$(TCC) /Fo\$@ -c ${s}_.c\n"
  writeln "${s}_.c : \$(SRCDIR)\\$s.c"
  writeln "\ttranslate\$E \$** > \$@\n"
}

writeln -nonewline "headers: makeheaders\$E page_index.h VERSION.h\n\tmakeheaders\$E "
foreach s [lsort $src] {
  writeln -nonewline "${s}_.c:$s.h "
}
writeln "\$(SRCDIR)\\sqlite3.h \$(SRCDIR)\\th.h VERSION.h"
writeln "\t@copy /Y nul: headers"


close $output_file
#
# End of the win/Makefile.msc output
##############################################################################
##############################################################################
##############################################################################
# Begin win/Makefile.PellesCGMake
#
puts "building ../win/Makefile.PellesCGMake"
set output_file [open ../win/Makefile.PellesCGMake w]
fconfigure $output_file -translation binary

writeln {# DO NOT EDIT
#
# This file is automatically generated.  Instead of editing this
# file, edit "makemake.tcl" then run "tclsh src/makemake.tcl"
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
DEFINES=-DFOSSIL_I18N=0 -D_pgmptr=g.argv[0]
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
SQLITEDEFINES=-DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_FILE_FORMAT=4 -Dlocaltime=fossil_localtime -DSQLITE_ENABLE_LOCKING_STYLE=0

# define the sqlite shell files, which need special flags on compile
SQLITESHELLSRC=shell.c
ORIGSQLITESHELLSRC=$(foreach sf,$(SQLITESHELLSRC),$(SRCDIR)$(sf))
SQLITESHELLOBJ=$(foreach sf,$(SQLITESHELLSRC),$(sf:.c=.obj))
SQLITESHELLDEFINES=-Dmain=sqlite3_shell -DSQLITE_OMIT_LOAD_EXTENSION=1

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
version.obj:	$(WINDIR)version.c
	$(CC) $(CCFLAGS) $(INCLUDE) "$<" -Fo"$@"

# generate the translated c-source files
$(TRANSLATEDSRC):	%_.c:	$(SRCDIR)%.c translate.exe
	translate.exe $< >$@

# generate the index source, containing all web references,..
page_index.h:	$(TRANSLATEDSRC) mkindex.exe
	mkindex.exe $(TRANSLATEDSRC) >$@

# extracting version info from manifest
VERSION.h:	version.exe ..\manifest.uuid ..\manifest
	version.exe ..\manifest.uuid ..\manifest  > $@

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
