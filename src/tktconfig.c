/*
** Copyright (c) 2007 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public
** License version 2 as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** General Public License for more details.
** 
** You should have received a copy of the GNU General Public
** License along with this library; if not, write to the
** Free Software Foundation, Inc., 59 Temple Place - Suite 330,
** Boston, MA  02111-1307, USA.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*******************************************************************************
** 
** This file contains a string constant that is the default ticket
** configuration.
*/
#include "config.h"
#include "tktconfig.h"

/*
** This is a sample "ticket configuration" file for fossil.
**
** There is considerable flexibility in how tickets are defined
** in fossil.  Each repository can define its own ticket setup
** differently.  Each repository has an instance of a file, like
** this one that defines how that repository deals with tickets.
**
** This file is in the form of a script in TH1 - a minimalist
** TCL clone.
*/

/* @-comment: ** */
const char zDefaultTicketConfig[] = 
@ ############################################################################
@ # Every ticket configuration *must* define an SQL statement that creates
@ # the TICKET table.  This table must have three columns named
@ # tkt_id, tkt_uuid, and tkt_mtime.  tkt_id must be the integer primary
@ # key and tkt_uuid and tkt_mtime must be unique.  A configuration should
@ # define addition columns as necessary.  All columns should be in all
@ # lower-case letters and should not begin with "tkt".
@ #
@ set ticket_sql {
@    CREATE TABLE ticket(
@      -- Do not change any column that begins with tkt_
@      tkt_id INTEGER PRIMARY KEY,
@      tkt_uuid TEXT,
@      tkt_mtime DATE,
@      -- Add as many field as required below this line
@      type TEXT,
@      status TEXT,
@      subsystem TEXT,
@      priority TEXT,
@      severity TEXT,
@      foundin TEXT,
@      contact TEXT,
@      title TEXT,
@      comment TEXT,
@      -- Do not alter this UNIQUE clause:
@      UNIQUE(tkt_uuid, tkt_mtime)
@    );
@    -- Add indices as desired
@ }
@ 
@ ############################################################################
@ # You can define additional variables here.  These variables will be
@ # accessible to the page templates when they run.
@ #
@ set type_choices {
@    Code_Defect
@    Build_Problem
@    Documentation
@    Feature_Request
@    Incident
@ }
@ set priority_choices {Immediate High Medium Low Zero}
@ set severity_choices {Critical Severe Important Minor Cosmetic}
@ set resolution_choices {
@   Open
@   Fixed
@   Rejected
@   Unable_To_Reproduce
@   Works_As_Designed
@   External_Bug
@   Not_A_Bug
@   Duplicate
@   Overcome_By_Events
@   Drive_By_Patch
@ }
@ set status_choices {
@   Open
@   Verified
@   In_Process
@   Deferred
@   Fixed
@   Tested
@   Closed
@ }
@ set subsystem_choices {one two three}
@ 
@ ##########################################################################
@ # The "tktnew_template" variable is set to text which is a template for
@ # the HTML of the "New Ticket" page.  Within this template, text contained
@ # within [...] is subscript.  That subscript runs when the page is
@ # rendered.
@ #
@ set tktnew_template {
@   <!-- load database field names not found in CGI with an empty string -->
@   <!-- start a form -->
@   <th1>
@     if {[info exists submit]} {
@        set status Open
@        submit_ticket
@     }
@   </th1>
@   <table cellpadding="5">
@   <tr>
@   <td colspan="2">
@   Enter a one-line summary of the problem:<br>
@   <input type="text" name="title" size="60" value="$<title>">
@   </td>
@   </tr>
@   
@   <tr>
@   <td align="right">Type:
@   <th1>combobox type $type_choices 1</th1>
@   </td>
@   <td>What type of ticket is this?</td>
@   </tr>
@   
@   <tr>
@   <td align="right">Version: 
@   <input type="text" name="foundin" size="20" value="$<foundin>">
@   </td>
@   <td>In what version or build number do you observer the problem?</td>
@   </tr>
@   
@   <tr>
@   <td align="right">Severity:
@   <th1>comboboxy severity severity_choices 1</th1>
@   </td>
@   <td>How debilitating is the problem?  How badly does the problem
@   effect the operation of the product?</td>
@   </tr>
@   
@   <tr>
@   <td align="right">EMail:
@   <input type="text" name="contact" value="$<contact>" size="30">
@   </td>
@   <td>Not publically visible. Used by developers to contact you with
@   questions.</td>
@   </tr>
@   
@   <tr>
@   <td colspan="2">
@   Enter a detailed description of the problem.
@   For code defects, be sure to provide details on exactly how
@   the problem can be reproduced.  Provide as much detail as
@   possible.
@   <br>
@   <th1>
@      if {![info exists comment]} {
@        set nline 10
@      } else {
@        set nline [linecount $comment]
@        if {$nline>50} {set nline 50}
@      }
@   </th1>
@   <textarea name="comment" cols="80" rows="$nline"
@    wrap="virtual" class="wikiedit">$<comment></textarea><br>
@   <input type="submit" name="preview" value="Preview">
@   </tr>
@ 
@   <th1>enable_output [info exists preview]</th1>
@   <tr><td colspan="2">
@   Description Preview:<br><hr>
@   <th1>wiki $comment</th1>
@   <hr>
@   </td></tr>
@   <th1>enable_output 1</th1>
@   
@   <tr>
@   <td align="right">
@   <input type="submit" name="submit" value="Submit">
@   </td>
@   <td>After filling in the information above, press this button to create
@   the new ticket</td>
@   </tr>
@   </table>
@   <!-- end of form -->
@ }
@ 
@ ##########################################################################
@ # The template for the "edit ticket" page
@ #
@ # Then generated text is inserted inside a form which feeds back to itself.
@ # All CGI parameters are loaded into variables.  All database files are
@ # loaded into variables if they have not previously been loaded by
@ # CGI parameters.
@ set tktedit_template {
@   <th1>
@     if {![info exists username]} {set username $login}
@     if {[info exists submit]} {
@       if {[info exists $cmappnd] && [string length $cmappnd]>0} {
@         set ctxt "\n\n<hr><i>"
@         if {$username==$login} {
@           set usr "$ctxt[htmlize $login]"
@         } else {
@           set usr "[htmlize $login claimingn to be [htmlize $username]"
@         }
@         append_field comment \
@            "\n\n<hr><i>$usr added on [date]:</i><br>\n$comment"
@       }
@       submit_ticket
@     }
@   </th1>
@   <table cellpadding="5">
@   <tr><td align="right">Title:</td><td>
@   <input type="text" name="title" value="$<title>" size="60">
@   </td></tr>
@   <tr><td align="right">Status:</td><td>
@   <th1>combobox status $status_choices 1</th1>
@   </td></tr>
@   <tr><td align="right">Type:</td><td>
@   <th1>combobox type $type_choices 1</th1>
@   </td></tr>
@   <tr><td align="right">Severity:</td><td>
@   <th1>combobox severity $severity_choices 1</th1>
@   </td></tr>
@   <tr><td align="right">Priority:</td><td>
@   <th1>combobox priority $priority_choices 1</th1>
@   </td></tr>
@   <tr><td align="right">Resolution:</td><td>
@   <th1>combobox resolution $resolution_choices 1</th1>
@   </td></tr>
@   <tr><td align="right">Subsystem:</td><td>
@   <th1>combobox subsystem $subsystem_choices 1</th1>
@   </td></tr>
@   <th1>enable_output [hascap e]</th1>
@     <tr><td align="right">Contact:</td><td>
@     <input type="text" name="contact" size="40" value="$<contact>">
@     </td></tr>
@   <th1>enable_output 1</th1>
@   <tr><td align="right">Version&nbsp;Found&nbsp;In:</td><td>
@   <input type="text" name="foundin" size="50" value="$<foundin>">
@   </td></tr>
@   <tr><td colspan="2">
@
@   [
@      0 /eall get /eall set           # eall means "edit all".  default==no
@      /aonlybtn exists not /eall set  # Edit all if no aonlybtn CGI param
@      /eallbtn exists /eall set       # Edit all if eallbtn CGI param
@      /w hascap eall and /eall set    # WrTkt permission needed to edit all
@   ]
@ 
@   [eall enable_output]
@     Description And Comments:<br>
@     <textarea name="comment" cols="80" 
@      rows="[{} /comment get linecount 15 max 10 min html]"
@      wrap="virtual" class="wikiedit">[comment html]</textarea><br>
@     <input type="hidden" name="eall" value="1">
@     <input type="submit" name="aonlybtn" value="Append Remark">
@   
@   [eall not enable_output]
@     Append Remark from 
@     <input type="text" name="username" value="[username html]" size="30">:<br>
@     <textarea name="cmappnd" cols="80" rows="15"
@      wrap="virtual" class="wikiedit">[{} /cmappnd get html]</textarea><br>
@     [/w hascap eall not and enable_output]
@     <input type="submit" name="eallbtn" value="Edit All">
@
@   [1 enable_output]
@   </td></tr>
@   <tr><td align="right"></td><td>
@   <input type="submit" name="submit" value="Submit Changes">
@   </td></tr>
@   </table>
@ } /tktedit_template set
@ 
@ ##########################################################################
@ # The template for the "view ticket" page
@ set tktview_template {
@   <!-- load database fields automatically loaded into variables -->
@   <table cellpadding="5">
@   <tr><td align="right">Title:</td><td>
@   $<title>
@   </td></tr>
@   <tr><td align="right">Status:</td><td>
@   $<status>
@   </td></tr>
@   <tr><td align="right">Type:</td><td>
@   $<type>
@   </td></tr>
@   <tr><td align="right">Severity:</td><td>
@   $<severity>
@   </td></tr>
@   <tr><td align="right">Priority:</td><td>
@   $<priority>
@   </td></tr>
@   <tr><td align="right">Resolution:</td><td>
@   $<resolution>
@   </td></tr>
@   <tr><td align="right">Subsystem:</td><td>
@   $<subsystem>
@   </td></tr>
@   <th1>enable_output [hascap e]</th1>
@     <tr><td align="right">Contact:</td><td>
@     $<contact>
@     </td></tr>
@   <th1>enable_output 1</th1>
@   <tr><td align="right">Version&nbsp;Found&nbsp;In:</td><td>
@   $<foundin>
@   </td></tr>
@   <tr><td colspan="2">
@   Description And Comments:<br>
@   <th1>wiki $comment</th1>
@   </td></tr>
@   </table>
@ }
;
