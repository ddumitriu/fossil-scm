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

## Pass VI. This pass goes over the set of revision based changesets
## and breaks all dependency cycles they may be in. We need a
## dependency tree. Identical to pass VII, except for the selection of
## the changesets.

# # ## ### ##### ######## ############# #####################
## Requirements

package require Tcl 8.4                                   ; # Required runtime.
package require snit                                      ; # OO system.
package require struct::list                              ; # Higher order list operations.
package require vc::tools::log                            ; # User feedback.
package require vc::fossil::import::cvs::cyclebreaker     ; # Breaking dependency cycles.
package require vc::fossil::import::cvs::state            ; # State storage.
package require vc::fossil::import::cvs::project::rev     ; # Project level changesets

# # ## ### ##### ######## ############# #####################
## Register the pass with the management

vc::fossil::import::cvs::pass define \
    BreakRevCsetCycles \
    {Break Revision ChangeSet Dependency Cycles} \
    ::vc::fossil::import::cvs::pass::breakrcycle

# # ## ### ##### ######## ############# #####################
## 

snit::type ::vc::fossil::import::cvs::pass::breakrcycle {
    # # ## ### ##### ######## #############
    ## Public API

    typemethod setup {} {
	# Define the names and structure of the persistent state of
	# this pass.

	state reading revision
	state reading changeset
	state reading csrevision

	state writing csorder {
	    -- Commit order of changesets based on their dependencies
	    cid INTEGER  NOT NULL  REFERENCES changeset,
	    pos INTEGER  NOT NULL,
	    UNIQUE (cid),
	    UNIQUE (pos)
	}
	return
    }

    typemethod load {} {
	# Pass manager interface. Executed to load data computed by
	# this pass into memory when this pass is skipped instead of
	# executed.

	state reading changeset
	project::rev loadcounter
	return
    }

    typemethod run {} {
	# Pass manager interface. Executed to perform the
	# functionality of the pass.

	state transaction {
	    cyclebreaker run [struct::list filter [project::rev all] \
				  [myproc IsByRevision]] \
		[myproc SaveOrder]
	}
	return
    }

    typemethod discard {} {
	# Pass manager interface. Executed for all passes after the
	# run passes, to remove all data of this pass from the state,
	# as being out of date.

	state discard csorder
	return
    }

    # # ## ### ##### ######## #############
    ## Internal methods

    proc IsByRevision {cset} { $cset byrevision }

    proc SaveOrder {at cset} {
	set cid [$cset id]

	log write 4 breakrcycle "Comitting @ $at: <$cid>"
	state run {
	    INSERT INTO csorder (cid,  pos)
	    VALUES              ($cid, $at)
	}
	# TODO: Write the project level changeset dependencies as well.
	return
    }

    # # ## ### ##### ######## #############
    ## Configuration

    pragma -hasinstances   no ; # singleton
    pragma -hastypeinfo    no ; # no introspection
    pragma -hastypedestroy no ; # immortal

    # # ## ### ##### ######## #############
}

namespace eval ::vc::fossil::import::cvs::pass {
    namespace export breakrcycle
    namespace eval breakrcycle {
	namespace import ::vc::fossil::import::cvs::cyclebreaker
	namespace import ::vc::fossil::import::cvs::state
	namespace eval project {
	    namespace import ::vc::fossil::import::cvs::project::rev
	}
	namespace import ::vc::tools::log
	log register breakrcycle
    }
}

# # ## ### ##### ######## ############# #####################
## Ready

package provide vc::fossil::import::cvs::pass::breakrcycle 1.0
return
