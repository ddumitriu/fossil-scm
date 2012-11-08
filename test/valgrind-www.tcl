#!/usr/bin/tclsh
#
# Run this script in an open Fossil checkout at the top-level with a 
# fresh build of Fossil itself.  This script will run fossil on hundreds
# of different web-pages looking for memory allocation problems using 
# valgrind.  Valgrind output appears on stderr.  Suggested test scenario:
#
#     make
#     tclsh valgrind-www.tcl 2>&1 | tee valgrind-out.txt
#
# Then examine the valgrind-out.txt file for issues.
#
proc run_query {url} {
  set fd [open q.txt w]
  puts $fd "GET $url HTTP/1.0\r\n\r"
  close $fd
  return [exec valgrind ./fossil test-http <q.txt 2>@ stderr]
}
set todo {}
foreach url {
  /home
  /timeline
  /brlist
  /taglist
  /reportlist
  /setup
  /dir
} {
  set seen($url) 1
  lappend todo $url
}
for {set i 0} {$i<[llength $todo] && $i<1000} {incr i} {
  set url [lindex $todo $i]
  puts "====== ([expr {$i+1}]) $url ======"
  set x [run_query $url]
  while {[regexp {<[aA] .*?href="(/[a-z].*?)".*?>(.*)$} $x all url tail]} {
    set u2 [string map {&lt; < &gt; > &quot; \" &amp; &} $url]
    if {![info exists seen($u2)]} {
      lappend todo $u2
      set seen($u2) 1
    }
    set x $tail
  }
}
