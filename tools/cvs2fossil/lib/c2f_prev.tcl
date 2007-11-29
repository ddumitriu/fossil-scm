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

## Revisions per project, aka Changesets. These objects are first used
## in pass 5, which creates the initial set covering the repository.

# # ## ### ##### ######## ############# #####################
## Requirements

package require Tcl 8.4                               ; # Required runtime.
package require snit                                  ; # OO system.
package require vc::tools::misc                       ; # Text formatting
package require vc::tools::trouble                    ; # Error reporting.
package require vc::tools::log                        ; # User feedback.
package require vc::fossil::import::cvs::state        ; # State storage.
package require vc::fossil::import::cvs::integrity    ; # State integrity checks.
package require vc::fossil::import::cvs::project::sym ; # Project level symbols

# # ## ### ##### ######## ############# #####################
##

snit::type ::vc::fossil::import::cvs::project::rev {
    # # ## ### ##### ######## #############
    ## Public API

    constructor {project cstype srcid revisions {theid {}}} {
	if {$theid ne ""} {
	    set myid $theid
	} else {
	    set myid [incr mycounter]
	}

	integrity assert {[info exists mycstype($cstype)]} {Bad changeset type '$cstype'.}

	set myproject   $project
	set mytype      $cstype
	set mytypeobj   ::vc::fossil::import::cvs::project::rev::${cstype}
	set mysrcid	$srcid
	set myrevisions $revisions
	set mypos       {} ; # Commit location is not known yet.

	# Keep track of the generated changesets and of the inverse
	# mapping from revisions to them.
	lappend mychangesets   $self
	set     myidmap($myid) $self
	foreach r $revisions { lappend myrevmap($r) $self }
	return
    }

    method str {} {
	set str    "<"
	set detail ""
	if {$mytype eq "sym"} {
	    struct::list assign [state run {
		SELECT T.name, S.name
		FROM   symtype T, symbol S
		WHERE  S.sid = $mysrcid
		AND    T.tid = S.type
	    }] stype detail
	    append str $stype " "
	    set detail " '$detail'"
	}
	append str "$mytype ${myid}${detail}>"
	return $str
    }

    method id        {} { return $myid }
    method revisions {} { return $myrevisions }
    method data      {} { return [list $myproject $mytype $mysrcid] }

    delegate method bysymbol   to mytypeobj
    delegate method byrevision to mytypeobj
    delegate method isbranch   to mytypeobj
    delegate method istag      to mytypeobj

    method setpos {p} { set mypos $p ; return }
    method pos    {}  { return $mypos }

    # result = dict (revision -> list (changeset))
    method successormap {} {
	# NOTE / FUTURE: Possible bottleneck.
	array set tmp {}
	foreach {rev children} [$self nextmap] {
	    foreach child $children {
		# 8.5 lappend tmp($rev) {*}$myrevmap($child)
		foreach cset $myrevmap($child) {
		    lappend tmp($rev) $cset
		}
	    }
	    set tmp($rev) [lsort -unique $tmp($rev)]
	}
	return [array get tmp]
    }

    method successors {} {
	# NOTE / FUTURE: Possible bottleneck.
	set csets {}
	foreach {_ children} [$self nextmap] {
	    foreach child $children {
		# 8.5 lappend csets {*}$myrevmap($child)
		foreach cset $myrevmap($child) {
		    lappend csets $cset
		}
	    }
	}
	return [lsort -unique $csets]
    }

    # result = dict (revision -> list (changeset))
    method predecessormap {} {
	# NOTE / FUTURE: Possible bottleneck.
	array set tmp {}
	foreach {rev children} [$self premap] {
	    foreach child $children {
		# 8.5 lappend tmp($rev) {*}$myrevmap($child)
		foreach cset $myrevmap($child) {
		    lappend tmp($rev) $cset
		}
	    }
	    set tmp($rev) [lsort -unique $tmp($rev)]
	}
	return [array get tmp]
    }

    # revision -> list (revision)
    method nextmap {} {
	if {[llength $mynextmap]} { return $mynextmap }
	$mytypeobj successors tmp $myrevisions
	set mynextmap [array get tmp]
	return $mynextmap
    }

    # revision -> list (revision)
    method premap {} {
	if {[llength $mypremap]} { return $mypremap }
	$mytypeobj predecessors tmp $myrevisions
	set mypremap [array get tmp]
	return $mypremap
    }

    method breakinternaldependencies {} {
	# This method inspects the changesets for internal
	# dependencies. Nothing is done if there are no
	# such. Otherwise the changeset is split into a set of
	# fragments without internal dependencies, transforming the
	# internal dependencies into external ones. The new changesets
	# are added to the list of all changesets.

	# We perform all necessary splits in one go, instead of only
	# one. The previous algorithm, adapted from cvs2svn, computed
	# a lot of state which was thrown away and then computed again
	# for each of the fragments. It should be easier to update and
	# reuse that state.

	# The code checks only sucessor dependencies, as this
	# automatically covers the predecessor dependencies as well (A
	# successor dependency a -> b is also a predecessor dependency
	# b -> a).

	# Array of dependencies (parent -> child). This is pulled from
	# the state, and limited to successors within the changeset.

	array set dependencies {}
	$mytypeobj internalsuccessors dependencies $myrevisions
	if {![array size dependencies]} {return 0} ; # Nothing to break.

	log write 5 csets ...[$self str].......................................................

	# We have internal dependencies to break. We now iterate over
	# all positions in the list (which is chronological, at least
	# as far as the timestamps are correct and unique) and
	# determine the best position for the break, by trying to
	# break as many dependencies as possible in one go. When a
	# break was found this is redone for the fragments coming and
	# after, after upding the crossing information.

	# Data structures:
	# Map:  POS   revision id      -> position in list.
	#       CROSS position in list -> number of dependencies crossing it
	#       DEPC  dependency       -> positions it crosses
	# List: RANGE Of the positions itself.
	# A dependency is a single-element map parent -> child

	InitializeBreakState $myrevisions

	set fragments {}
	set pending   [list $range]
	set at        0
	array set breaks {}

	while {$at < [llength $pending]} {
	    set current [lindex $pending $at]

	    log write 6 csets {. . .. ... ..... ........ .............}
	    log write 6 csets {Scheduled   [join [PRs [lrange $pending $at end]] { }]}
	    log write 6 csets {Considering [PR $current] \[$at/[llength $pending]\]}

	    set best [FindBestBreak $current]

	    if {$best < 0} {
		# The inspected range has no internal
		# dependencies. This is a complete fragment.
		lappend fragments $current

		log write 6 csets "No breaks, final"
	    } else {
		# Split the range and schedule the resulting fragments
		# for further inspection. Remember the number of
		# dependencies cut before we remove them from
		# consideration, for documentation later.

		set breaks($best) $cross($best)

		log write 6 csets "Best break @ $best, cutting [nsp $cross($best) dependency dependencies]"

		# Note: The value of best is an abolute location in
		# myrevisions. Use the start of current to make it an
		# index absolute to current.

		set brel [expr {$best - [lindex $current 0]}]
		set bnext $brel ; incr bnext
		set fragbefore [lrange $current 0 $brel]
		set fragafter  [lrange $current $bnext end]

		log write 6 csets "New pieces  [PR $fragbefore] [PR $fragafter]"

		integrity assert {[llength $fragbefore]} {Found zero-length fragment at the beginning}
		integrity assert {[llength $fragafter]}  {Found zero-length fragment at the end}

		lappend pending $fragbefore $fragafter
		CutAt $best
	    }

	    incr at
	}

	log write 6 csets ". . .. ... ..... ........ ............."

	# (*) We clear out the associated part of the myrevmap
	# in-memory index in preparation for new data. A simple unset
	# is enough, we have no symbol changesets at this time, and
	# thus never more than one reference in the list.

	foreach r $myrevisions { unset myrevmap($r) }

	# Create changesets for the fragments, reusing the current one
	# for the first fragment. We sort them in order to allow
	# checking for gaps and nice messages.

	set fragments [lsort -index 0 -integer $fragments]

	#puts \t.[join [PRs $fragments] .\n\t.].

	Border [lindex $fragments 0] firsts firste

	integrity assert {$firsts == 0} {Bad fragment start @ $firsts, gap, or before beginning of the range}

	set laste $firste
	foreach fragment [lrange $fragments 1 end] {
	    Border $fragment s e
	    integrity assert {$laste == ($s - 1)} {Bad fragment border <$laste | $s>, gap or overlap}

	    set new [$type %AUTO% $myproject $mytype $mysrcid [lrange $myrevisions $s $e]]

            log write 4 csets "Breaking [$self str ] @ $laste, new [$new str], cutting $breaks($laste)"

	    set laste $e
	}

	integrity assert {
	    $laste == ([llength $myrevisions]-1)
	} {Bad fragment end @ $laste, gap, or beyond end of the range}

	# Put the first fragment into the current changeset, and
	# update the in-memory index. We can simply (re)add the
	# revisions because we cleared the previously existing
	# information, see (*) above. Persistence does not matter
	# here, none of the changesets has been saved to the
	# persistent state yet.

	set myrevisions [lrange $myrevisions 0 $firste]
	foreach r $myrevisions { lappend myrevmap($r) $self }

	return 1
    }

    method persist {} {
	set tid $mycstype($mytype)
	set pid [$myproject id]
	set pos 0

	state transaction {
	    state run {
		INSERT INTO changeset (cid,   pid,  type, src)
		VALUES                ($myid, $pid, $tid, $mysrcid);
	    }

	    foreach rid $myrevisions {
		state run {
		    INSERT INTO csrevision (cid,   pos,  rid)
		    VALUES                 ($myid, $pos, $rid);
		}
		incr pos
	    }
	}
	return
    }

    method timerange {} { return [$mytypeobj timerange $myrevisions] }

    method drop {} {
	state transaction {
	    state run {
		DELETE FROM changeset  WHERE cid = $myid;
		DELETE FROM csrevision WHERE cid = $myid;
	    }
	}
	foreach r $myrevisions {
	    if {[llength $myrevmap($r)] == 1} {
		unset myrevmap($r)
	    } else {
		set pos [lsearch -exact $myrevmap($r) $self]
		set myrevmap($r) [lreplace $myrevmap($r) $pos $pos]
	    }
	}
	set pos          [lsearch -exact $mychangesets $self]
	set mychangesets [lreplace $mychangesets $pos $pos]
	return
    }

    typemethod split {cset args} {
	# As part of the creation of the new changesets specified in
	# ARGS as sets of revisions, all subsets of CSET's revision
	# set, CSET will be dropped from all databases, in and out of
	# memory, and then destroyed.

	struct::list assign [$cset data] project cstype cssrc

	$cset drop
	$cset destroy

	set newcsets {}
	foreach fragmentrevisions $args {
	    integrity assert {
		[llength $fragmentrevisions]
	    } {Attempted to create an empty changeset, i.e. without revisions}
	    lappend newcsets [$type %AUTO% $project $cstype $cssrc $fragmentrevisions]
	}

	foreach c $newcsets { $c persist }
	return $newcsets
    }

    typemethod strlist {changesets} {
	return [join [struct::list map $changesets [myproc ID]]]
    }

    proc ID {cset} { $cset str }

    # # ## ### ##### ######## #############
    ## State

    variable myid        {} ; # Id of the cset for the persistent
			      # state.
    variable myproject   {} ; # Reference of the project object the
			      # changeset belongs to.
    variable mytype      {} ; # What the changeset is based on
			      # (revisions, tags, or branches).
			      # Values: See mycstype. Note that we
			      # have to keep the names of the helper
			      # singletons in sync with the contents
			      # of state table 'cstype', and various
			      # other places using them hardwired.
    variable mytypeobj   {} ; # Reference to the container for the
			      # type dependent code. Derived from
			      # mytype.
    variable mysrcid     {} ; # Id of the metadata or symbol the cset
			      # is based on.
    variable myrevisions {} ; # List of the file level revisions in
			      # the cset.
    variable mypremap    {} ; # Dictionary mapping from the revisions
			      # to their predecessors. Cache to avoid
			      # loading this from the state more than
			      # once.
    variable mynextmap   {} ; # Dictionary mapping from the revisions
			      # to their successors. Cache to avoid
			      # loading this from the state more than
			      # once.
    variable mypos       {} ; # Commit position of the changeset, if
			      # known.

    # # ## ### ##### ######## #############
    ## Internal methods

    typevariable mycounter        0 ; # Id counter for csets. Last id used.
    typevariable mycstype -array {} ; # Map cstypes (names) to persistent
				      # ids. Note that we have to keep
				      # the names in the table 'cstype'
				      # in sync with the names of the
				      # helper singletons.

    typemethod getcstypes {} {
	foreach {tid name} [state run {
	    SELECT tid, name FROM cstype;
	}] { set mycstype($name) $tid }
	return
    }

    typemethod loadcounter {} {
	# Initialize the counter from the state
	set mycounter [state one { SELECT MAX(cid) FROM changeset }]
	return
    }

    typemethod num {} { return $mycounter }

    proc PullInternalSuccessorRevisions {dv revisions} {
	upvar 1 $dv dependencies
	set theset ('[join $revisions {','}]')

	# See PullSuccessorRevisions below for the main explanation of
	# the various cases. This piece is special in that it
	# restricts the successors we look for to the same set of
	# revisions we start from. Sensible as we are looking for
	# changeset internal dependencies.

	array set dep {}

	foreach {rid child} [state run "
   -- (1) Primary child
	    SELECT R.rid, R.child
	    FROM   revision R
	    WHERE  R.rid   IN $theset     -- Restrict to revisions of interest
	    AND    R.child IS NOT NULL    -- Has primary child
	    AND    R.child IN $theset     -- Which is also of interest
    UNION
    -- (2) Secondary (branch) children
	    SELECT R.rid, B.brid
	    FROM   revision R, revisionbranchchildren B
	    WHERE  R.rid   IN $theset     -- Restrict to revisions of interest
	    AND    R.rid = B.rid          -- Select subset of branch children
	    AND    B.brid IN $theset      -- Which is also of interest
    UNION
    -- (4) Child of trunk root successor of last NTDB on trunk.
	    SELECT R.rid, RA.child
	    FROM revision R, revision RA
	    WHERE R.rid   IN $theset      -- Restrict to revisions of interest
	    AND   R.isdefault             -- Restrict to NTDB
	    AND   R.dbchild IS NOT NULL   -- and last NTDB belonging to trunk
	    AND   RA.rid = R.dbchild      -- Go directly to trunk root
	    AND   RA.child IS NOT NULL    -- Has primary child.
            AND   RA.child IN $theset     -- Which is also of interest
	"] {
	    # Consider moving this to the integrity module.
	    integrity assert {$rid != $child} {Revision $rid depends on itself.}
	    lappend dependencies($rid) $child
	    set dep($rid,$child) .
	}

	# The sql statements above looks only for direct dependencies
	# between revision in the changeset. However due to the
	# vagaries of meta data it is possible for two revisions of
	# the same file to end up in the same changeset, without a
	# direct dependency between them. However we know that there
	# has to be a an indirect dependency, be it through primary
	# children, branch children, or a combination thereof.

	# We now fill in these pseudo-dependencies, if no such
	# dependency exists already. The direction of the dependency
	# is actually irrelevant for this.

	# NOTE: This is different from cvs2svn. Our spiritual ancestor
	# does not use such pseudo-dependencies, however it uses a
	# COMMIT_THRESHOLD, a time interval commits should fall. This
	# will greatly reduces the risk of getting far separated
	# revisions of the same file into one changeset.

	# We allow revisions to be far apart in time in the same
	# changeset, but need the pseudo-dependencies for this.

	array set fids {}
	foreach {rid fid} [state run "
	    SELECT R.rid, R.fid FROM revision R WHERE R.rid IN $theset
	"] { lappend fids($fid) $rid }

	foreach {fid rids} [array get fids] {
	    if {[llength $rids] < 2} continue
	    foreach a $rids {
		foreach b $rids {
		    if {$a == $b} continue
		    if {[info exists dep($a,$b)]} continue
		    if {[info exists dep($b,$a)]} continue
		    lappend dependencies($a) $b
		    set dep($a,$b) .
		    set dep($b,$a) .
		}
	    }
	}
	return
    }

    proc PullSuccessorRevisions {dv revisions} {
	upvar 1 $dv dependencies
	set theset ('[join $revisions {','}]')

	# The following cases specify when a revision S is a successor
	# of a revision R. Each of the cases translates into one of
	# the branches of the SQL UNION coming below.
	#
	# (1) S can be a primary child of R, i.e. in the same LOD. R
	#     references S directly. R.child = S(.rid), if it exists.
	#
	# (2) S can be a secondary, i.e. branch, child of R. Here the
	#     link is made through the helper table
	#     REVISIONBRANCHCHILDREN. R.rid -> RBC.rid, RBC.brid =
	#     S(.rid)
	#
	# (3) Originally this use case defined the root of a detached
	#     NTDB as the successor of the trunk root. This leads to a
	#     bad tangle later on. With a detached NTDB the original
	#     trunk root revision was removed as irrelevant, allowing
	#     the nominal root to be later in time than the NTDB
	#     root. Now setting this dependency will be backward in
	#     time. REMOVED.
	#
	# (4) If R is the last of the NTDB revisions which belong to
	#     the trunk, then the primary child of the trunk root (the
	#     '1.2' revision) is a successor, if it exists.

	foreach {rid child} [state run "
   -- (1) Primary child
	    SELECT R.rid, R.child
	    FROM   revision R
	    WHERE  R.rid   IN $theset     -- Restrict to revisions of interest
	    AND    R.child IS NOT NULL    -- Has primary child
    UNION
    -- (2) Secondary (branch) children
	    SELECT R.rid, B.brid
	    FROM   revision R, revisionbranchchildren B
	    WHERE  R.rid   IN $theset     -- Restrict to revisions of interest
	    AND    R.rid = B.rid          -- Select subset of branch children
    UNION
    -- (4) Child of trunk root successor of last NTDB on trunk.
	    SELECT R.rid, RA.child
	    FROM revision R, revision RA
	    WHERE R.rid   IN $theset      -- Restrict to revisions of interest
	    AND   R.isdefault             -- Restrict to NTDB
	    AND   R.dbchild IS NOT NULL   -- and last NTDB belonging to trunk
	    AND   RA.rid = R.dbchild      -- Go directly to trunk root
	    AND   RA.child IS NOT NULL    -- Has primary child.
	"] {
	    # Consider moving this to the integrity module.
	    integrity assert {$rid != $child} {Revision $rid depends on itself.}
	    lappend dependencies($rid) $child
	}
	return
    }

    proc PullPredecessorRevisions {dv revisions} {
	upvar 1 $dv dependencies
	set theset ('[join $revisions {','}]')

	# The following cases specify when a revision P is a
	# predecessor of a revision R. Each of the cases translates
	# into one of the branches of the SQL UNION coming below.
	#
	# (1) The immediate parent R.parent of R is a predecessor of
	#     R. NOTE: This is true for R either primary or secondary
	#     child of P. It not necessary to distinguish the two
	#     cases, in contrast to the code retrieving the successor
	#     information.
	#
	# (2) The complement of successor case (3). The trunk root is
	#     a predecessor of a NTDB root. REMOVED. See
	#     PullSuccessorRevisions for the explanation.
	#
	# (3) The complement of successor case (4). The last NTDB
	#     revision belonging to the trunk is a predecessor of the
	#     primary child of the trunk root (The '1.2' revision).

	foreach {rid parent} [state run "
   -- (1) Primary parent, can be in different LOD for first in a branch
	    SELECT R.rid, R.parent
	    FROM   revision R
	    WHERE  R.rid   IN $theset     -- Restrict to revisions of interest
	    AND    R.parent IS NOT NULL   -- Has primary parent
    UNION
    -- (3) Last NTDB on trunk is predecessor of child of trunk root
	    SELECT R.rid, RA.dbparent
	    FROM revision R, revision RA
	    WHERE R.rid IN $theset       -- Restrict to revisions of interest
	    AND NOT R.isdefault          -- not on NTDB
	    AND R.parent IS NOT NULL     -- which are not root
	    AND RA.rid = R.parent        -- go to their parent
	    AND RA.dbparent IS NOT NULL  -- which has to refer to NTDB's root
	"] {
	    # Consider moving this to the integrity module.
	    integrity assert {$rid != $parent} {Revision $rid depends on itself.}
	    lappend dependencies($rid) $parent
	}
	return
    }

    proc InitializeBreakState {revisions} {
	upvar 1 pos pos cross cross range range depc depc delta delta \
	    dependencies dependencies

	# First we create a map of positions to make it easier to
	# determine whether a dependency crosses a particular index.

	array set pos   {}
	array set cross {}
	array set depc  {}
	set range       {}
	set n 0
	foreach rev $revisions {
	    lappend range $n
	    set pos($rev) $n
	    set cross($n) 0
	    incr n
	}

	# Secondly we count the crossings per position, by iterating
	# over the recorded internal dependencies.

	# Note: If the timestamps are badly out of order it is
	#       possible to have a backward successor dependency,
	#       i.e. with start > end. We may have to swap the indices
	#       to ensure that the following loop runs correctly.
	#
	# Note 2: start == end is not possible. It indicates a
	#         self-dependency due to the uniqueness of positions,
	#         and that is something we have ruled out already, see
	#         PullInternalSuccessorRevisions.

	foreach {rid children} [array get dependencies] {
	    foreach child $children {
		set dkey    [list $rid $child]
		set start   $pos($rid)
		set end     $pos($child)
		set crosses {}

		if {$start > $end} {
		    while {$end < $start} {
			lappend crosses $end
			incr cross($end)
			incr end
		    }
		} else {
		    while {$start < $end} {
			lappend crosses $start
			incr cross($start)
			incr start
		    }
		}
		set depc($dkey) $crosses
	    }
	}

	InitializeDeltas $revisions
	return
    }

    proc InitializeDeltas {revisions} {
	upvar 1 delta delta

	# Pull the timestamps for all revisions in the changesets and
	# compute their deltas for use by the break finder.

	array set delta {}
	array set stamp {}

	set theset ('[join $revisions {','}]')
	foreach {rid time} [state run "
	    SELECT R.rid, R.date
	    FROM revision R
	    WHERE R.rid IN $theset
	"] {
	    set stamp($rid) $time
	}

	set n 0
	foreach rid [lrange $revisions 0 end-1] rnext [lrange $revisions 1 end] {
	    set delta($n) [expr {$stamp($rnext) - $stamp($rid)}]
	    incr n
	}
	return
    }

    proc FindBestBreak {range} {
	upvar 1 cross cross delta delta

	# Determine the best break location in the given range of
	# positions. First we look for the locations with the maximal
	# number of crossings. If there are several we look for the
	# shortest time interval among them. If we still have multiple
	# possibilities after that we select the earliest location
	# among these.

	# Note: If the maximal number of crossings is 0 then the range
	#       has no internal dependencies, and no break location at
	#       all. This possibility is signaled via result -1.

	# Note: A range of length 1 or less cannot have internal
	#       dependencies, as that needs at least two revisions in
	#       the range.

	if {[llength $range] < 2} { return -1 }

	set max -1
	set best {}

	foreach location $range {
	    set crossings $cross($location)
	    if {$crossings > $max} {
		set max  $crossings
		set best [list $location]
		continue
	    } elseif {$crossings == $max} {
		lappend best $location
	    }
	}

	if {$max == 0}            { return -1 }
	if {[llength $best] == 1} { return [lindex $best 0] }

	set locations $best
	set best {}
	set min -1

	foreach location $locations {
	    set interval $delta($location)
	    if {($min < 0) || ($interval < $min)} {
		set min  $interval
		set best [list $location]
	    } elseif {$interval == $min} {
		lappend best $location
	    }
	}

	if {[llength $best] == 1} { return [lindex $best 0] }

	return [lindex [lsort -integer -increasing $best] 0]
    }

    proc CutAt {location} {
	upvar 1 cross cross depc depc

	# It was decided to split the changeset at the given
	# location. This cuts a number of dependencies. Here we update
	# the cross information so that the break finder has accurate
	# data when we look at the generated fragments.

	set six [log visible? 6]

	foreach {dep range} [array get depc] {
	    # Check all dependencies still known, take their range and
	    # see if the break location falls within.

	    Border $range s e
	    if {$location < $s} continue ; # break before range, ignore
	    if {$location > $e} continue ; # break after range, ignore.

	    # This dependency crosses the break location. We remove it
	    # from the crossings counters, and then also from the set
	    # of known dependencies, as we are done with it.

	    foreach loc $depc($dep) { incr cross($loc) -1 }
	    unset depc($dep)

	    if {!$six} continue

	    struct::list assign $dep parent child
	    log write 5 csets "Broke dependency [PD $parent] --> [PD $child]"
	}

	return
    }

    # Print identifying data for a revision (project, file, dotted rev
    # number), for high verbosity log output.

    proc PD {id} {
	foreach {p f r} [state run {
		SELECT P.name , F.name, R.rev
		FROM revision R, file F, project P
		WHERE R.rid = $id
		AND   R.fid = F.fid
		AND   F.pid = P.pid
	}] break
	return "'$p : $f/$r'"
    }

    # Printing one or more ranges, formatted, and only their border to
    # keep the strings short.

    proc PRs {ranges} {
	return [struct::list map $ranges [myproc PR]]
    }

    proc PR {range} {
	Border $range s e
	return <${s}...${e}>
    }

    proc Border {range sv ev} {
	upvar 1 $sv s $ev e
	set s [lindex $range 0]
	set e [lindex $range end]
	return
    }

    # # ## ### ##### ######## #############

    typevariable mychangesets    {} ; # List of all known changesets.
    typevariable myrevmap -array {} ; # Map from revisions to the list
				      # of changesets containing
				      # it. NOTE: While only one
				      # revision changeset can contain
				      # the revision, there can
				      # however also be one or more
				      # additional symbol changesets
				      # which use it, hence a list.
    typevariable myidmap  -array {} ; # Map from changeset id to changeset.
    typevariable mybranchcode    {} ; # Local copy of project::sym/mybranch.

    typemethod all   {}   { return $mychangesets }
    typemethod of    {id} { return $myidmap($id) }
    typemethod ofrev {id} { return $myrevmap($id) }

    # # ## ### ##### ######## #############
    ## Configuration

    pragma -hastypeinfo    no  ; # no type introspection
    pragma -hasinfo        no  ; # no object introspection
    pragma -simpledispatch yes ; # simple fast dispatch

    # # ## ### ##### ######## #############
}

# # ## ### ##### ######## ############# #####################
## Helper singleton. Commands for revision changesets.

snit::type ::vc::fossil::import::cvs::project::rev::rev {
    typemethod byrevision {} { return 1 }
    typemethod bysymbol   {} { return 0 }
    typemethod istag      {} { return 0 }
    typemethod isbranch   {} { return 0 }

    # result = list (mintime, maxtime)
    typemethod timerange {items} {
	set theset ('[join $items {','}]')
	return [state run "
	    SELECT MIN(R.date), MAX(R.date)
	    FROM revision R
	    WHERE R.rid IN $theset
	"]
    }

    # var(dv) = dict (revision -> list (revision))
    typemethod internalsuccessors {dv revisions} {
    }

    # var(dv) = dict (item -> list (item)), item  = list (type id)
    typemethod successors {dv revisions} {
    }

    # var(dv) = dict (item -> list (item)), item  = list (type id)
    typemethod predecessors {dv revisions} {
    }
}

# # ## ### ##### ######## ############# #####################
## Helper singleton. Commands for tag symbol changesets.

snit::type ::vc::fossil::import::cvs::project::rev::sym::tag {
    typemethod byrevision {} { return 0 }
    typemethod bysymbol   {} { return 1 }
    typemethod istag      {} { return 1 }
    typemethod isbranch   {} { return 0 }

    # result = list (mintime, maxtime)
    typemethod timerange {tags} {
    }

    # var(dv) = dict (item -> list (item)), item  = list (type id)
    typemethod successors {dv tags} {
    }

    # var(dv) = dict (item -> list (item)), item  = list (type id)
    typemethod predecessors {dv tags} {
    }
}

# # ## ### ##### ######## ############# #####################
## Helper singleton. Commands for branch symbol changesets.

snit::type ::vc::fossil::import::cvs::project::rev::sym::branch {
    typemethod byrevision {} { return 0 }
    typemethod bysymbol   {} { return 1 }
    typemethod istag      {} { return 0 }
    typemethod isbranch   {} { return 1 }

    # result = list (mintime, maxtime)
    typemethod timerange {branches} {
    }

    # var(dv) = dict (item -> list (item)), item  = list (type id)
    typemethod successors {dv branches} {
    }

    # var(dv) = dict (item -> list (item)), item  = list (type id)
    typemethod predecessors {dv branches} {
    }

    # # ## ### ##### ######## #############
    ## Configuration

    pragma -hasinstances   no ; # singleton
    pragma -hastypeinfo    no ; # no introspection
    pragma -hastypedestroy no ; # immortal
}

# # ## ### ##### ######## ############# #####################
##

namespace eval ::vc::fossil::import::cvs::project {
    namespace export rev
    namespace eval rev {
	namespace import ::vc::fossil::import::cvs::state
	namespace import ::vc::fossil::import::cvs::integrity
	namespace eval project {
	    namespace import ::vc::fossil::import::cvs::project::sym
	}
	::variable mybranchcode [project::sym branch]
	namespace import ::vc::tools::misc::*
	namespace import ::vc::tools::trouble
	namespace import ::vc::tools::log
	log register csets

	# Set up the helper singletons
	namespace eval rev {
	    namespace import ::vc::fossil::import::cvs::state
	    namespace import ::vc::fossil::import::cvs::integrity
	}
	namespace eval sym::tag {
	    namespace import ::vc::fossil::import::cvs::state
	    namespace import ::vc::fossil::import::cvs::integrity
	}
	namespace eval sym::branch {
	    namespace import ::vc::fossil::import::cvs::state
	    namespace import ::vc::fossil::import::cvs::integrity
	}
    }
}

# # ## ### ##### ######## ############# #####################
## Ready

package provide vc::fossil::import::cvs::project::rev 1.0
return
