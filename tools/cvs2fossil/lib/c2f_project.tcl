## -*- tcl -*-
# # ## ### ##### ######## ############# #####################
## Copyright (c) 2007 Andreas Kupries.
#
# This software is licensed as described in the file LICENSE, which
# you should have received as part of this distribution.
#
# This software consists of voluntary contributions made by many
# individuals.  For exact contribution history, see the revision
# history and logs, available at http://fossil-scm.hwaci.com/fossil
# # ## ### ##### ######## ############# #####################

## Project, part of a CVS repository. Multiple instances are possible.

# # ## ### ##### ######## ############# #####################
## Requirements

package require Tcl 8.4                                 ; # Required runtime.
package require snit                                    ; # OO system.
package require vc::fossil::import::cvs::file           ; # CVS archive file.
package require vc::fossil::import::cvs::state          ; # State storage.
package require vc::fossil::import::cvs::project::sym   ; # Per project symbols.
package require vc::fossil::import::cvs::project::trunk ; # Per project trunk, main lod
package require struct::list                            ; # Advanced list operations..

# # ## ### ##### ######## ############# #####################
## 

snit::type ::vc::fossil::import::cvs::project {
    # # ## ### ##### ######## #############
    ## Public API

    constructor {path r} {
	set mybase       $path
	set myrepository $r
	set mytrunk      [trunk %AUTO%]
	return
    }

    method base  {} { return $mybase  }
    method trunk {} { return $mytrunk }

    method printbase {} {
	if {$mybase eq ""} {return <Repository>}
	return $mybase
    }

    method setid {id} { set myid $id ; return }

    method addfile {rcs usr executable} {
	set myfiles($rcs) [list $usr $executable]
	return
    }

    method filenames {} {
	return [lsort -dict [array names myfiles]]
    }

    method files {} {
	return [TheFiles]
    }

    delegate method defauthor       to myrepository
    delegate method defcmessage     to myrepository
    delegate method trunkonly       to myrepository
    delegate method commitmessageof to myrepository

    method defmeta {bid aid cid} {
	return [$myrepository defmeta $myid $bid $aid $cid]
    }

    method getsymbol {name} {
	if {![info exists mysymbols($name)]} {
	    set mysymbols($name) \
		[sym %AUTO% $name [$myrepository defsymbol $myid $name]]
	}
	return $mysymbols($name)
    }

    # pass I persistence
    method persist {} {
	state transaction {
	    # Project data first. Required so that we have its id
	    # ready for the files.

	    state run {
		INSERT INTO project (pid,  name)
		VALUES              (NULL, $mybase);
	    }
	    set myid [state id]

	    # Then all files, with proper backreference to their
	    # project.

	    foreach {rcs item} [array get myfiles] {
		struct::list assign $item usr executable
		state run {
		    INSERT INTO file (fid,  pid,   name, visible, exec)
		    VALUES           (NULL, $myid, $rcs, $usr,    $executable);
		}
	    }
	}
	return
    }

    # pass II persistence
    method persistrev {} {
	state transaction {
	    # TODO: per project persistence (symbols, meta data)
	    foreach f [TheFiles] {
		$f persist
	    }
	}
	return
    }

    # # ## ### ##### ######## #############
    ## State

    variable mybase           {} ; # Project directory.
    variable myid             {} ; # Project id in the persistent state.
    variable mytrunk          {} ; # Reference to the main line of
				   # development for the project.
    variable myfiles   -array {} ; # Maps the rcs archive paths to
				   # their user-visible files.
    variable myfobj           {} ; # File objects for the rcs archives
    variable myrepository     {} ; # Repository the prject belongs to.
    variable mysymbols -array {} ; # Map symbol names to project-level
				   # symbol objects.

    # # ## ### ##### ######## #############
    ## Internal methods

    proc TheFiles {} {
	upvar 1 myfiles myfiles myfobj myfobj self self
	if {![llength $myfobj]} {
	    set myfobj [EmptyFiles myfiles]
	}
	return $myfobj
    }

    proc EmptyFiles {fv} {
	upvar 1 $fv myfiles self self
	set res {}
	foreach rcs [lsort -dict [array names myfiles]] {
	    struct::list assign $myfiles($rcs) f executable
	    lappend res [file %AUTO% $rcs $f $executable $self]
	}
	return $res
    }

    # # ## ### ##### ######## #############
    ## Configuration

    pragma -hastypeinfo    no  ; # no type introspection
    pragma -hasinfo        no  ; # no object introspection
    pragma -hastypemethods no  ; # type is not relevant.

    # # ## ### ##### ######## #############
}

namespace eval ::vc::fossil::import::cvs {
    namespace export project
    namespace eval project {
	namespace import ::vc::fossil::import::cvs::file
	namespace import ::vc::fossil::import::cvs::state
	# Import not required, already a child namespace.
	# namespace import ::vc::fossil::import::cvs::project::sym
    }
}

# # ## ### ##### ######## ############# #####################
## Ready

package provide vc::fossil::import::cvs::project 1.0
return
