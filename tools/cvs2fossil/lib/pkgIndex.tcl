# # ## ### ##### ######## ############# #####################
## Package management.
## Index of the local packages required by cvs2fossil
# # ## ### ##### ######## ############# #####################
if {![package vsatisfies [package require Tcl] 8.4]} return
package ifneeded vc::fossil::import::cvs                    1.0 [list source [file join $dir cvs2fossil.tcl]]
package ifneeded vc::fossil::import::cvs::blobstore         1.0 [list source [file join $dir c2f_blobstore.tcl]]
package ifneeded vc::fossil::import::cvs::file              1.0 [list source [file join $dir c2f_file.tcl]]
package ifneeded vc::fossil::import::cvs::file::rev         1.0 [list source [file join $dir c2f_frev.tcl]]
package ifneeded vc::fossil::import::cvs::file::sym         1.0 [list source [file join $dir c2f_fsym.tcl]]
package ifneeded vc::fossil::import::cvs::file::trunk       1.0 [list source [file join $dir c2f_ftrunk.tcl]]
package ifneeded vc::fossil::import::cvs::fossil            1.0 [list source [file join $dir c2f_fossil.tcl]]
package ifneeded vc::fossil::import::cvs::option            1.0 [list source [file join $dir c2f_option.tcl]]
package ifneeded vc::fossil::import::cvs::integrity         1.0 [list source [file join $dir c2f_integrity.tcl]]
package ifneeded vc::fossil::import::cvs::pass              1.0 [list source [file join $dir c2f_pass.tcl]]
package ifneeded vc::fossil::import::cvs::pass::collar      1.0 [list source [file join $dir c2f_pcollar.tcl]]
package ifneeded vc::fossil::import::cvs::pass::collrev     1.0 [list source [file join $dir c2f_pcollrev.tcl]]
package ifneeded vc::fossil::import::cvs::pass::collsym     1.0 [list source [file join $dir c2f_pcollsym.tcl]]
package ifneeded vc::fossil::import::cvs::pass::filtersym   1.0 [list source [file join $dir c2f_pfiltersym.tcl]]
package ifneeded vc::fossil::import::cvs::pass::initcsets   1.0 [list source [file join $dir c2f_pinitcsets.tcl]]
package ifneeded vc::fossil::import::cvs::pass::csetdeps    1.0 [list source [file join $dir c2f_pcsetdeps.tcl]]
package ifneeded vc::fossil::import::cvs::pass::breakrcycle 1.0 [list source [file join $dir c2f_pbreakrcycle.tcl]]
package ifneeded vc::fossil::import::cvs::pass::rtopsort    1.0 [list source [file join $dir c2f_prtopsort.tcl]]
package ifneeded vc::fossil::import::cvs::pass::breakscycle 1.0 [list source [file join $dir c2f_pbreakscycle.tcl]]
package ifneeded vc::fossil::import::cvs::pass::breakacycle 1.0 [list source [file join $dir c2f_pbreakacycle.tcl]]
package ifneeded vc::fossil::import::cvs::pass::atopsort    1.0 [list source [file join $dir c2f_patopsort.tcl]]
package ifneeded vc::fossil::import::cvs::pass::import      1.0 [list source [file join $dir c2f_pimport.tcl]]
package ifneeded vc::fossil::import::cvs::gtcore            1.0 [list source [file join $dir c2f_gtcore.tcl]]
package ifneeded vc::fossil::import::cvs::cyclebreaker      1.0 [list source [file join $dir c2f_cyclebreaker.tcl]]
package ifneeded vc::fossil::import::cvs::project           1.0 [list source [file join $dir c2f_project.tcl]]
package ifneeded vc::fossil::import::cvs::project::rev      1.0 [list source [file join $dir c2f_prev.tcl]]
package ifneeded vc::fossil::import::cvs::project::revlink  1.0 [list source [file join $dir c2f_prevlink.tcl]]
package ifneeded vc::fossil::import::cvs::project::sym      1.0 [list source [file join $dir c2f_psym.tcl]]
package ifneeded vc::fossil::import::cvs::project::trunk    1.0 [list source [file join $dir c2f_ptrunk.tcl]]
package ifneeded vc::fossil::import::cvs::repository        1.0 [list source [file join $dir c2f_repository.tcl]]
package ifneeded vc::fossil::import::cvs::state             1.0 [list source [file join $dir c2f_state.tcl]]
package ifneeded vc::rcs::parser                            1.0 [list source [file join $dir rcsparser.tcl]]
package ifneeded vc::tools::dot                             1.0 [list source [file join $dir dot.tcl]]
package ifneeded vc::tools::id                              1.0 [list source [file join $dir id.tcl]]
package ifneeded vc::tools::log                             1.0 [list source [file join $dir log.tcl]]
package ifneeded vc::tools::misc                            1.0 [list source [file join $dir misc.tcl]]
package ifneeded vc::tools::trouble                         1.0 [list source [file join $dir trouble.tcl]]
