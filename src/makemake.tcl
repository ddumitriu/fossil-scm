#!/usr/bin/tclsh
#
# Run this TCL script to generate the "main.mk" makefile.
#

# Basenames of all source files:
#
set src {
  add
  blob
  cgi
  checkin
  checkout
  clone
  comformat
  content
  db
  delta
  deltacmd
  descendents
  diff
  diffcmd
  encode
  file
  http
  info
  login
  main
  manifest
  md5
  merge
  merge3
  name
  pivot
  printf
  rebuild
  schema
  setup
  sha1
  style
  sync
  timeline
  update
  url
  user
  verify
  vfile
  wiki
  wikiformat
  xfer
}

# Name of the final application
#
set name fossil

puts {# This file is included by linux-gcc.mk or linux-mingw.mk or possible
# some other makefiles.  This file contains the rules that are common
# to building regardless of the target.
#

XTCC = $(TCC) $(CFLAGS) -I. -I$(SRCDIR)

}
puts -nonewline "SRC ="
foreach s [lsort $src] {
  puts -nonewline " \\\n  \$(SRCDIR)/$s.c"
}
puts "\n"
puts -nonewline "TRANS_SRC ="
foreach s [lsort $src] {
  puts -nonewline " \\\n  ${s}_.c"
}
puts "\n"
puts -nonewline "OBJ ="
foreach s [lsort $src] {
  puts -nonewline " \\\n  $s.o"
}
puts "\n"
puts "APPNAME = $name\$(E)"
puts "\n"

puts {
all:	$(APPNAME)

install:	$(APPNAME)
	mv $(APPNAME) $(INSTALLDIR)

translate:	$(SRCDIR)/translate.c
	$(BCC) -o translate $(SRCDIR)/translate.c

makeheaders:	$(SRCDIR)/makeheaders.c
	$(BCC) -o makeheaders $(SRCDIR)/makeheaders.c

mkindex:	$(SRCDIR)/mkindex.c
	$(BCC) -o mkindex $(SRCDIR)/mkindex.c

$(APPNAME):	headers $(OBJ) sqlite3.o
	$(TCC) -o $(APPNAME) $(OBJ) sqlite3.o $(LIB)

clean:	
	rm -f *.o *_.c $(APPNAME)
	rm -f translate makeheaders mkindex page_index.h headers}

set hfiles {}
foreach s [lsort $src] {lappend hfiles $s.h}
puts "\trm -f $hfiles\n"

set mhargs {}
foreach s [lsort $src] {
  append mhargs " ${s}_.c:$s.h"
  set extra_h($s) {}
}
append mhargs " \$(SRCDIR)/sqlite3.h"
puts "headers:\tmakeheaders mkindex \$(TRANS_SRC)"
puts "\t./makeheaders $mhargs"
puts "\t./mkindex \$(TRANS_SRC) >page_index.h"
puts "\ttouch headers\n"
set extra_h(main) page_index.h

foreach s [lsort $src] {
  puts "${s}_.c:\t\$(SRCDIR)/$s.c \$(SRCDIR)/VERSION translate"
  puts "\t./translate \$(SRCDIR)/$s.c | sed -f \$(SRCDIR)/VERSION >${s}_.c\n"
  puts "$s.o:\t${s}_.c $s.h $extra_h($s) \$(SRCDIR)/config.h"
  puts "\t\$(XTCC) -o $s.o -c ${s}_.c\n"
  puts "$s.h:\tmakeheaders"
  puts "\t./makeheaders $mhargs\n\ttouch headers\n"
}


puts "sqlite3.o:\t\$(SRCDIR)/sqlite3.c"
set opt {-DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_PRIVATE=}
append opt " -DTHREADSAFE=0 -DSQLITE_DEFAULT_FILE_FORMAT=4"
puts "\t\$(XTCC) $opt -c \$(SRCDIR)/sqlite3.c -o sqlite3.o\n"
