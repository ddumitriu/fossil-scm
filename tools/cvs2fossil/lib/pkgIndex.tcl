# # ## ### ##### ######## ############# #####################
## Package management.
## Index of the local packages required by cvs2fossil
# # ## ### ##### ######## ############# #####################
if {![package vsatisfies [package require Tcl] 8.4]} return
package ifneeded vc::fossil::import::cvs                  1.0 [list source [file join $dir cvs2fossil.tcl]]
package ifneeded vc::fossil::import::cvs::file            1.0 [list source [file join $dir c2f_file.tcl]]
package ifneeded vc::fossil::import::cvs::file::lodmgr    1.0 [list source [file join $dir c2f_flodmgr.tcl]]
package ifneeded vc::fossil::import::cvs::file::rev       1.0 [list source [file join $dir c2f_frev.tcl]]
package ifneeded vc::fossil::import::cvs::file::sym       1.0 [list source [file join $dir c2f_fsym.tcl]]
package ifneeded vc::fossil::import::cvs::file::trunk     1.0 [list source [file join $dir c2f_ftrunk.tcl]]
package ifneeded vc::fossil::import::cvs::option          1.0 [list source [file join $dir c2f_option.tcl]]
package ifneeded vc::fossil::import::cvs::pass            1.0 [list source [file join $dir c2f_pass.tcl]]
package ifneeded vc::fossil::import::cvs::pass::collar    1.0 [list source [file join $dir c2f_pcollar.tcl]]
package ifneeded vc::fossil::import::cvs::pass::collrev   1.0 [list source [file join $dir c2f_pcollrev.tcl]]
package ifneeded vc::fossil::import::cvs::pass::collsym   1.0 [list source [file join $dir c2f_pcollsym.tcl]]
package ifneeded vc::fossil::import::cvs::project         1.0 [list source [file join $dir c2f_project.tcl]]
package ifneeded vc::fossil::import::cvs::project::lodmgr 1.0 [list source [file join $dir c2f_plodmgr.tcl]]
package ifneeded vc::fossil::import::cvs::project::rev    1.0 [list source [file join $dir c2f_prev.tcl]]
package ifneeded vc::fossil::import::cvs::project::sym    1.0 [list source [file join $dir c2f_psym.tcl]]
package ifneeded vc::fossil::import::cvs::project::trunk  1.0 [list source [file join $dir c2f_ptrunk.tcl]]
package ifneeded vc::fossil::import::cvs::repository      1.0 [list source [file join $dir c2f_repository.tcl]]
package ifneeded vc::fossil::import::cvs::state           1.0 [list source [file join $dir c2f_state.tcl]]
package ifneeded vc::rcs::parser                          1.0 [list source [file join $dir rcsparser.tcl]]
package ifneeded vc::tools::log                           1.0 [list source [file join $dir log.tcl]]
package ifneeded vc::tools::misc                          1.0 [list source [file join $dir misc.tcl]]
package ifneeded vc::tools::trouble                       1.0 [list source [file join $dir trouble.tcl]]
package ifneeded vc::tools::id                            1.0 [list source [file join $dir id.tcl]]
