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

## Pass IX. This is the final pass for breaking changeset dependency
## cycles. The previous breaker passes (6 and 8) broke cycles covering
## revision and symbol changesets, respectively. This pass now breaks
## any remaining cycles, each of which has to contain at least one
## revision and at least one symbol changeset.

# # ## ### ##### ######## ############# #####################
## Requirements

package require Tcl 8.4                                   ; # Required runtime.
package require snit                                      ; # OO system.
package require struct::list                              ; # Higher order list operations.
package require struct::set                               ; # Set operations.
package require vc::tools::misc                           ; # Min, max.
package require vc::tools::log                            ; # User feedback.
package require vc::tools::trouble                        ; # Error reporting.
package require vc::fossil::import::cvs::repository       ; # Repository management.
package require vc::fossil::import::cvs::cyclebreaker     ; # Breaking dependency cycles.
package require vc::fossil::import::cvs::state            ; # State storage.
package require vc::fossil::import::cvs::integrity        ; # State integrity checks.
package require vc::fossil::import::cvs::project::rev     ; # Project level changesets

# # ## ### ##### ######## ############# #####################
## Register the pass with the management

vc::fossil::import::cvs::pass define \
    BreakAllCsetCycles \
    {Break Remaining ChangeSet Dependency Cycles} \
    ::vc::fossil::import::cvs::pass::breakacycle

# # ## ### ##### ######## ############# #####################
##

snit::type ::vc::fossil::import::cvs::pass::breakacycle {
    # # ## ### ##### ######## #############
    ## Public API

    typemethod setup {} {
	# Define the names and structure of the persistent state of
	# this pass.

	state reading changeset
	state reading csitem
	state reading csorder
	return
    }

    typemethod load {} {
	# Pass manager interface. Executed to load data computed by
	# this pass into memory when this pass is skipped instead of
	# executed.
	return
    }

    typemethod run {} {
	# Pass manager interface. Executed to perform the
	# functionality of the pass.

	set len [string length [project::rev num]]
	set myatfmt %${len}s
	incr len 12
	set mycsfmt %${len}s

	cyclebreaker precmd   [myproc BreakBackward]
	cyclebreaker savecmd  [myproc KeepOrder]

	state transaction {
	    LoadCommitOrder
	    cyclebreaker run break-all [myproc Changesets]
	}

	repository printcsetstatistics
	integrity changesets
	return
    }

    typemethod discard {} {
	# Pass manager interface. Executed for all passes after the
	# run passes, to remove all data of this pass from the state,
	# as being out of date.
	return
    }

    # # ## ### ##### ######## #############
    ## Internal methods

    proc Changesets {} { project::rev all }

    proc LoadCommitOrder {} {
	::variable mycset
	::variable myrevisionchangesets

	state transaction {
	    foreach {cid pos} [state run { SELECT cid, pos FROM csorder }] {
		set cset [project::rev of $cid]
		$cset setpos $pos
		set mycset($pos) $cset
		lappend myrevisionchangesets $cset
	    }
	}
	return
    }

    # # ## ### ##### ######## #############

    proc BreakBackward {graph} {
	# We go over all branch changesets, i.e. the changesets
	# created by the symbols which are translated as branches, and
	# break any which are 'backward', which means that they have
	# at least one incoming revision changeset which is committed
	# after at least one of the outgoing revision changesets, per
	# the order computed in pass 6. In "cvs2svn" this is called
	# "retrograde".

	foreach cset [$graph nodes] {
	    if {![$cset isbranch]} continue
	    CheckAndBreakBackward $graph $cset
	}
	return
    }

    proc CheckAndBreakBackward {graph cset} {
	while {[IsBackward $graph $cset]} {
	    # Knowing that the branch changeset is backward we now
	    # look at the individual branches in the changeset and
	    # determine which of them are responsible for the
	    # overlap. This allows us to split them into two sets, one
	    # of non-overlapping branches, and of overlapping
	    # ones. Each set induces a new changeset, and the second
	    # one may still be backward and in need of further
	    # splitting. Hence the looping.
	    #
	    # The border used for the split is the minimal commit
	    # position among the minimal sucessor commit positions for
	    # the branches in the changeset.

	    # Note that individual branches may not have changesets
	    # which are their predecessors and/or successors, leaving
	    # the limits partially or completely undefined.

	    # limits : dict (revision -> list (max predecessor commit, min sucessor commit))

	    ComputeLimits $cset limits border

	    log write 5 breakacycle "Breaking backward changeset [$cset str] with commit position $border as border"

	    # Secondly we sort the file level items based on there
	    # they sit relative to the border into before and after
	    # the border.

	    SplitItems $limits $border normalitems backwarditems

	    set replacements [project::rev split $cset $normalitems $backwarditems]
	    cyclebreaker replace $graph $cset $replacements

	    # At last check that the normal frament is indeed not
	    # backward, and iterate over the possibly still backward
	    # second fragment.

	    struct::list assign $replacements normal backward
	    integrity assert {
		![IsBackward $graph $normal]
	    } {The normal fragment is unexpectedly backward}

	    set cset $backward
	}
	return
    }

    proc IsBackward {dg cset} {
	# A branch is "backward" if it has at least one incoming
	# revision changeset which is committed after at least one of
	# the outgoing revision changesets, per the order computed by
	# pass 6.

	# Rephrased, the maximal commit position found among the
	# incoming revision changesets is larger than the minimal
	# commit position found among the outgoing revision
	# changesets. Assuming that we have both incoming and outgoing
	# revision changesets for the branch.

	# The helper "Positions" computes the set of commit positions
	# for a set of changesets, which can be a mix of revision and
	# symbol changesets.

	set predecessors [Positions [$dg nodes -in  $cset]]
	set successors   [Positions [$dg nodes -out $cset]]

	return [expr {
		      [llength $predecessors] &&
		      [llength $successors]   &&
		      ([max $predecessors] >= [min $successors])
		  }]
    }

    proc Positions {changesets} {
	# To compute the set of commit positions from the set of
	# changesets we first map each changeset to its position (*)
	# and then filter out the invalid responses (the empty string)
	# returned by the symbol changesets.
	#
	# (*) This data was loaded into memory earlir in the pass, by
	#     LoadCommitOrder.

	return [struct::list filter [struct::list map $changesets \
					 [myproc ToPosition]] \
		    [myproc ValidPosition]]
    }

    proc ToPosition    {cset} { $cset pos }
    proc ValidPosition {pos}  { expr {$pos ne ""} }

    proc ComputeLimits {cset lv bv} {
	upvar 1 $lv thelimits $bv border

	# Initialize the boundaries for all items.

	array set limits {}
	foreach revision [$cset items] {
	    set limits($revision) {0 {}}
	}

	# Compute and store the maximal predecessors per item (branch)

	foreach {item csets} [$cset predecessormap] {
	    set s [Positions $csets]
	    if {![llength $s]} continue
	    set limits($item) [lreplace $limits($item) 0 0 [max $s]]
	}

	# Compute and store the minimal successors per item (branch)

	foreach {item csets} [$cset successormap] {
	    set s [Positions $csets]
	    if {![llength $s]} continue
	    set limits($item) [lreplace $limits($item) 1 1 [min $s]]
	}

	# Check that the ordering at the file level is correct. We
	# cannot have backward ordering per branch, or something is
	# wrong.

	foreach item [array names limits] {
	    struct::list assign $limits($item) maxp mins
	    # Handle min successor position "" as representing infinity
	    integrity assert {
		($mins eq "") || ($maxp < $mins) 
	    } {Item <$item> is backward at file level ($maxp >= $mins)}
	}

	# Save the limits for the splitter, and compute the border at
	# which to split as the minimum of all minimal successor
	# positions.

	set thelimits [array get limits]
	set border [min [struct::list filter [struct::list map [Values $thelimits] \
						  [myproc MinSuccessorPosition]] \
			     [myproc ValidPosition]]]
	return
    }

    proc Values {dict} {
	set res {}
	foreach {k v} $dict { lappend res $v }
	return $res
    }

    proc MinSuccessorPosition {item} { lindex $item 1 }

    proc SplitItems {limits border nv bv} {
	upvar 1 $nv normalitems $bv backwarditems

	set normalitems   {}
	set backwarditems {}

	foreach {rev v} $limits {
	    struct::list assign $v maxp mins
	    if {$maxp >= $border} {
		lappend backwarditems  $rev
	    } else {
		lappend normalitems $rev
	    }
	}

	integrity assert {[llength $normalitems]}   {Set of normal items is empty}
	integrity assert {[llength $backwarditems]} {Set of backward items is empty}
	return
    }


    # # ## ### ##### ######## #############

    proc KeepOrder {graph at cset} {
	::variable myatfmt
	::variable mycsfmt

	set cid [$cset id]

	log write 8 breakacycle "Changeset @ [format $myatfmt $at]: [format $mycsfmt [$cset str]] <<[FormatTR $graph $cset]>>"

	# We see here a mixture of symbol and revision changesets.
	# The symbol changesets are ignored as irrelevant.

	if {[$cset pos] eq ""} return

	# For the revision changesets we are sure that they are
	# consumed in the same order as generated by pass 7
	# (RevTopologicalSort). Per the code in cvs2svn.

	# This works if and only if none of the symbol changesets are
	# "backwards", hence our breaking of the backward changesets
	# first, in the pre-hook.

	# Note that tah changesets cannot be backward as they don't
	# have successors at all.

	# An interesting thing IMHO, is that after breaking the
	# backward symbol changesets we should not have any circles
	# any longer. Each circle which would still be present has to
	# involve a backward symbol, and we split them all, so there
	# can't be a circle..

	# Proof:
	# Let us assume we that have a circle
	# 	C: R1 -> ... -> Rx -> S -> Ry -> ... -> Rn -> R1
	# Let us further assume that the symbol changeset S in that
	# circle is not backward. That means ORD(Rx) < ORD(Ry).  The
	# earlier topological sorting without symbols now forces this
	# relationship through to be ORD(Rx) < ORD(R1) < ORD(Rx).  We
	# have reached an impossibility, a paradox. Our initial
	# assumption of S not being backward cannot hold.
	#
	# Alternate, direct, reasoning: Without S the chain of
	# dependencies is Ry -> .. -> R1 -> .. -> Rx, therefore
	# ORD(Ry) < ORD(Rx) holds, and this means S is backward.

	struct::set exclude myrevisionchangesets $cset

	::variable mylastpos
	set new [$cset pos]

	if {$new != ($mylastpos + 1)} {
	    if {$mylastpos < 0} {
		set old "<NONE>"
	    } else {
		::variable mycset
		set old [$mycset($mylastpos) str]@$mylastpos
	    }

	    integrity assert 0 {Ordering of revision changesets violated, [$cset str]@$new is not immediately after $old}
	}

	set mylastpos $new
	return
    }

    proc FormatTR {graph cset} {
	return [join [struct::list map [$graph node set $cset timerange] {clock format}] { -- }]
    }

    typevariable mylastpos            -1 ; # Position of last revision changeset saved.
    typevariable myrevisionchangesets {} ; # Set of revision changesets

    typevariable myatfmt ; # Format for log output to gain better alignment of the various columns.
    typevariable mycsfmt ; # Ditto for the changesets.

    # # ## ### ##### ######## #############

    typevariable mycset -array {} ; # Map from commit positions to the
				    # changeset (object ref) at that
				    # position.

    # # ## ### ##### ######## #############
    ## Configuration

    pragma -hasinstances   no ; # singleton
    pragma -hastypeinfo    no ; # no introspection
    pragma -hastypedestroy no ; # immortal

    # # ## ### ##### ######## #############
}

namespace eval ::vc::fossil::import::cvs::pass {
    namespace export breakacycle
    namespace eval breakacycle {
	namespace import ::vc::fossil::import::cvs::cyclebreaker
	namespace import ::vc::fossil::import::cvs::repository
	namespace import ::vc::fossil::import::cvs::state
	namespace import ::vc::fossil::import::cvs::integrity
	namespace eval project {
	    namespace import ::vc::fossil::import::cvs::project::rev
	}
	namespace import ::vc::tools::misc::*
	namespace import ::vc::tools::trouble
	namespace import ::vc::tools::log
	log register breakacycle
    }
}

# # ## ### ##### ######## ############# #####################
## Ready

package provide vc::fossil::import::cvs::pass::breakacycle 1.0
return
