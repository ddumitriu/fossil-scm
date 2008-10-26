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
** This file contains code to implement the ticket configuration
** setup screens.
*/
#include "config.h"
#include "tktsetup.h"
#include <assert.h>

/*
** Main sub-menu for configuring the ticketing system.
** WEBPAGE: tktsetup
*/
void tktsetup_page(void){
  login_check_credentials();
  if( !g.okSetup ){
    login_needed();
  }

  style_header("Ticket Setup");
  @ <table border="0" cellspacing="20">
  setup_menu_entry("Table", "tktsetup_tab",
    "Specify the schema of the  \"ticket\" table in the database.");
  setup_menu_entry("Timeline", "tktsetup_timeline",
    "How to display ticket status in the timeline");
  setup_menu_entry("Common", "tktsetup_com",
    "Common TH1 code run before all ticket processing.");
  setup_menu_entry("New Ticket Page", "tktsetup_newpage",
    "HTML with embedded TH1 code for the \"new ticket\" webpage.");
  setup_menu_entry("View Ticket Page", "tktsetup_viewpage",
    "HTML with embedded TH1 code for the \"view ticket\" webpage.");
  setup_menu_entry("Edit Ticket Page", "tktsetup_editpage",
    "HTML with embedded TH1 code for the \"edit ticket\" webpage.");
  setup_menu_entry("Report Template", "tktsetup_rpttplt",
    "The default ticket report format.");
  setup_menu_entry("Key Template", "tktsetup_keytplt",
    "The default color key for reports.");
  @ </table>
  style_footer();
}

/*
** NOTE:  When changing the table definition below, also change the
** equivalent definition found in schema.c.
*/
/* @-comment: ** */
static const char zDefaultTicketTable[] =
@ CREATE TABLE ticket(
@   -- Do not change any column that begins with tkt_
@   tkt_id INTEGER PRIMARY KEY,
@   tkt_uuid TEXT UNIQUE,
@   tkt_mtime DATE,
@   -- Add as many field as required below this line
@   type TEXT,
@   status TEXT,
@   subsystem TEXT,
@   priority TEXT,
@   severity TEXT,
@   foundin TEXT,
@   private_contact TEXT,
@   resolution TEXT,
@   title TEXT,
@   comment TEXT
@ );
;

/*
** Return the ticket table definition
*/
const char *ticket_table_schema(void){
  return db_get("ticket-table", (char*)zDefaultTicketTable);
}

/*
** Common implementation for the ticket setup editor pages.
*/
static void tktsetup_generic(
  const char *zTitle,           /* Page title */
  const char *zDbField,         /* Configuration field being edited */
  const char *zDfltValue,       /* Default text value */
  const char *zDesc,            /* Description of this field */
  char *(*xText)(const char*),  /* Validitity test or NULL */
  void (*xRebuild)(void),       /* Run after successulf update */
  int height                    /* Height of the edit box */
){
  const char *z;
  int isSubmit;
  
  login_check_credentials();
  if( !g.okSetup ){
    login_needed();
  }
  if( P("setup") ){
    cgi_redirect("tktsetup");
  }
  isSubmit = P("submit")!=0;
  z = P("x");
  if( z==0 ){
    z = db_get(zDbField, (char*)zDfltValue);
  }
  style_header("Edit %s", zTitle);
  if( P("clear")!=0 ){
    login_verify_csrf_secret();
    db_unset(zDbField, 0);
    if( xRebuild ) xRebuild();
    z = zDfltValue;
  }else if( isSubmit ){
    char *zErr = 0;
    login_verify_csrf_secret();
    if( xText && (zErr = xText(z))!=0 ){
      @ <p><font color="red"><b>ERROR: %h(zErr)</b></font></p>
    }else{
      db_set(zDbField, z, 0);
      if( xRebuild ) xRebuild();
      cgi_redirect("tktsetup");
    }
  }
  @ <form action="%s(g.zBaseURL)/%s(g.zPath)" method="POST">
  login_insert_csrf_secret();
  @ <p>%s(zDesc)</p>
  @ <textarea name="x" rows="%d(height)" cols="80">%h(z)</textarea>
  @ <blockquote>
  @ <input type="submit" name="submit" value="Apply Changes">
  @ <input type="submit" name="clear" value="Revert To Default">
  @ <input type="submit" name="setup" value="Cancel">
  @ </blockquote>
  @ </form>
  @ <hr>
  @ <h2>Default %s(zTitle)</h2>
  @ <blockquote><pre>
  @ %h(zDfltValue)
  @ </pre></blockquote>
  style_footer();
}

/*
** WEBPAGE: tktsetup_tab
*/
void tktsetup_tab_page(void){
  static const char zDesc[] =
  @ <p>Enter a valid CREATE TABLE statement for the "ticket" table.  The
  @ table must contain columns named "tkt_id", "tkt_uuid", and "tkt_mtime"
  @ with an unique index on "tkt_uuid" and "tkt_mtime".</p>
  ;
  tktsetup_generic(
    "Ticket Table Schema",
    "ticket-table",
    zDefaultTicketTable,
    zDesc,
    ticket_schema_check,
    ticket_rebuild,
    20
  );
}

static const char zDefaultTicketCommon[] =
@ set type_choices {
@    Code_Defect
@    Build_Problem
@    Documentation
@    Feature_Request
@    Incident
@ }
@ set priority_choices {
@   Immediate
@   High
@   Medium
@   Low
@   Zero
@ }
@ set severity_choices {
@   Critical
@   Severe
@   Important
@   Minor
@   Cosmetic
@ }
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
@   Review
@   Deferred
@   Fixed
@   Tested
@   Closed
@ }
@ set subsystem_choices {
@ }
;

/*
** Return the ticket common code.
*/
const char *ticket_common_code(void){
  return db_get("ticket-common", (char*)zDefaultTicketCommon);
}

/*
** WEBPAGE: tktsetup_com
*/
void tktsetup_com_page(void){
  static const char zDesc[] =
  @ <p>Enter TH1 script that initializes variables prior to generating
  @ any of the ticket view, edit, or creation pages.</p>
  ;
  tktsetup_generic(
    "Ticket Common Script",
    "ticket-common",
    zDefaultTicketCommon,
    zDesc,
    0,
    0,
    30
  );
}

static const char zDefaultNew[] =
@ <th1>
@   if {[info exists submit]} {
@      set status Open
@      submit_ticket
@   }
@ </th1>
@ <h1 align="center">Enter A New Bug Report</h1>
@ <table cellpadding="5">
@ <tr>
@ <td colspan="2">
@ Enter a one-line summary of the problem:<br>
@ <input type="text" name="title" size="60" value="$<title>">
@ </td>
@ </tr>
@ 
@ <tr>
@ <td align="right">Type:
@ <th1>combobox type $type_choices 1</th1>
@ </td>
@ <td>What type of ticket is this?</td>
@ </tr>
@ 
@ <tr>
@ <td align="right">Version: 
@ <input type="text" name="foundin" size="20" value="$<foundin>">
@ </td>
@ <td>In what version or build number do you observe the problem?</td>
@ </tr>
@ 
@ <tr>
@ <td align="right">Severity:
@ <th1>combobox severity $severity_choices 1</th1>
@ </td>
@ <td>How debilitating is the problem?  How badly does the problem
@ affect the operation of the product?</td>
@ </tr>
@ 
@ <tr>
@ <td align="right">EMail:
@ <input type="text" name="private_contact" value="$<private_contact>" size="30">
@ </td>
@ <td><u>Not publicly visible</u>. Used by developers to contact you with
@ questions.</td>
@ </tr>
@ 
@ <tr>
@ <td colspan="2">
@ Enter a detailed description of the problem.
@ For code defects, be sure to provide details on exactly how
@ the problem can be reproduced.  Provide as much detail as
@ possible.
@ <br>
@ <th1>set nline [linecount $comment 50 10]</th1>
@ <textarea name="comment" cols="80" rows="$nline"
@  wrap="virtual" class="wikiedit">$<comment></textarea><br>
@ <input type="submit" name="preview" value="Preview">
@ </tr>
@
@ <th1>enable_output [info exists preview]</th1>
@ <tr><td colspan="2">
@ Description Preview:<br><hr>
@ <th1>wiki $comment</th1>
@ <hr>
@ </td></tr>
@ <th1>enable_output 1</th1>
@ 
@ <tr>
@ <td align="right">
@ <input type="submit" name="submit" value="Submit">
@ </td>
@ <td>After filling in the information above, press this button to create
@ the new ticket</td>
@ </tr>
@ <tr>
@ <td align="right">
@ <input type="submit" name="cancel" value="Cancel">
@ </td>
@ <td>Abandon and forget this bug report</td>
@ </tr>
@ </table>
;

/*
** Return the code used to generate the new ticket page
*/
const char *ticket_newpage_code(void){
  return db_get("ticket-newpage", (char*)zDefaultNew);
}

/*
** WEBPAGE: tktsetup_newpage
*/
void tktsetup_newpage_page(void){
  static const char zDesc[] =
  @ <p>Enter HTML with embedded TH1 script that will render the "new ticket"
  @ page</p>
  ;
  tktsetup_generic(
    "HTML For New Tickets",
    "ticket-newpage",
    zDefaultNew,
    zDesc,
    0,
    0,
    40
  );
}

static const char zDefaultView[] =
@ <table cellpadding="5">
@ <tr><td align="right">Ticket&nbsp;UUID:</td><td bgcolor="#d0d0d0" colspan="3">
@ $<tkt_uuid>
@ </td></tr>
@ <tr><td align="right">Title:</td>
@ <td bgcolor="#d0d0d0" colspan="3" valign="top">
@ $<title>
@ </td></tr>
@ <tr><td align="right">Status:</td><td bgcolor="#d0d0d0">
@ $<status>
@ </td>
@ <td align="right">Type:</td><td bgcolor="#d0d0d0">
@ $<type>
@ </td></tr>
@ <tr><td align="right">Severity:</td><td bgcolor="#d0d0d0">
@ $<severity>
@ </td>
@ <td align="right">Priority:</td><td bgcolor="#d0d0d0">
@ $<priority>
@ </td></tr>
@ <tr><td align="right">Subsystem:</td><td bgcolor="#d0d0d0">
@ $<subsystem>
@ </td>
@ <td align="right">Resolution:</td><td bgcolor="#d0d0d0">
@ $<resolution>
@ </td></tr>
@ <tr><td align="right">Last&nbsp;Modified:</td><td bgcolor="#d0d0d0">
@ $<tkt_datetime>
@ </td>
@ <th1>enable_output [hascap e]</th1>
@   <td align="right">Contact:</td><td bgcolor="#d0d0d0">
@   $<private_contact>
@   </td>
@ <th1>enable_output 1</th1>
@ </tr>
@ <tr><td align="right">Version&nbsp;Found&nbsp;In:</td>
@ <td colspan="3" valign="top" bgcolor="#d0d0d0">
@ $<foundin>
@ </td></tr>
@ <tr><td>Description &amp; Comments:</td></tr>
@ <tr><td colspan="4" bgcolor="#d0d0d0">
@ <span  bgcolor="#d0d0d0"><th1>wiki $comment</th1></span>
@ </td></tr>
@ </table>
;


/*
** Return the code used to generate the view ticket page
*/
const char *ticket_viewpage_code(void){
  return db_get("ticket-viewpage", (char*)zDefaultView);
}

/*
** WEBPAGE: tktsetup_viewpage
*/
void tktsetup_viewpage_page(void){
  static const char zDesc[] =
  @ <p>Enter HTML with embedded TH1 script that will render the "view ticket"
  @ page</p>
  ;
  tktsetup_generic(
    "HTML For Viewing Tickets",
    "ticket-viewpage",
    zDefaultView,
    zDesc,
    0,
    0,
    40
  );
}

static const char zDefaultEdit[] =
@ <th1>
@   if {![info exists username]} {set username $login}
@   if {[info exists submit]} {
@     if {[info exists cmappnd]} {
@       if {[string length $cmappnd]>0} {
@         set ctxt "\n\n<hr><i>[htmlize $login]"
@         if {$username ne $login} {
@           set ctxt "$ctxt claiming to be [htmlize $username]"
@         }
@         set ctxt "$ctxt added on [date]:</i><br>\n$cmappnd"
@         append_field comment $ctxt
@       }
@     }
@     submit_ticket
@   }
@ </th1>
@ <table cellpadding="5">
@ <tr><td align="right">Title:</td><td>
@ <input type="text" name="title" value="$<title>" size="60">
@ </td></tr>
@ <tr><td align="right">Status:</td><td>
@ <th1>combobox status $status_choices 1</th1>
@ </td></tr>
@ <tr><td align="right">Type:</td><td>
@ <th1>combobox type $type_choices 1</th1>
@ </td></tr>
@ <tr><td align="right">Severity:</td><td>
@ <th1>combobox severity $severity_choices 1</th1>
@ </td></tr>
@ <tr><td align="right">Priority:</td><td>
@ <th1>combobox priority $priority_choices 1</th1>
@ </td></tr>
@ <tr><td align="right">Resolution:</td><td>
@ <th1>combobox resolution $resolution_choices 1</th1>
@ </td></tr>
@ <tr><td align="right">Subsystem:</td><td>
@ <th1>combobox subsystem $subsystem_choices 1</th1>
@ </td></tr>
@ <th1>enable_output [hascap e]</th1>
@   <tr><td align="right">Contact:</td><td>
@   <input type="text" name="private_contact" size="40"
@    value="$<private_contact>">
@   </td></tr>
@ <th1>enable_output 1</th1>
@ <tr><td align="right">Version&nbsp;Found&nbsp;In:</td><td>
@ <input type="text" name="foundin" size="50" value="$<foundin>">
@ </td></tr>
@ <tr><td colspan="2">
@ <th1>
@   if {![info exists eall]} {set eall 0}
@   if {[info exists aonlybtn]} {set eall 0}
@   if {[info exists eallbtn]} {set eall 1}
@   if {![hascap w]} {set eall 0}
@   if {![info exists cmappnd]} {set cmappnd {}}
@   set nline [linecount $comment 15 10]
@   enable_output $eall
@ </th1>
@   Description And Comments:<br>
@   <textarea name="comment" cols="80" rows="$nline"
@    wrap="virtual" class="wikiedit">$<comment></textarea><br>
@   <input type="hidden" name="eall" value="1">
@   <input type="submit" name="aonlybtn" value="Append Remark">
@ <th1>enable_output [expr {!$eall}]</th1>
@   Append Remark from 
@   <input type="text" name="username" value="$<username>" size="30">:<br>
@   <textarea name="cmappnd" cols="80" rows="15"
@    wrap="virtual" class="wikiedit">$<cmappnd></textarea><br>
@ <th1>enable_output [expr {[hascap w] && !$eall}]</th1>
@   <input type="submit" name="eallbtn" value="Edit All">
@ <th1>enable_output 1</th1>
@ </td></tr>
@ <tr><td align="right"></td><td>
@ <input type="submit" name="submit" value="Submit Changes">
@ <input type="submit" name="cancel" value="Cancel">
@ </td></tr>
@ </table>
;

/*
** Return the code used to generate the edit ticket page
*/
const char *ticket_editpage_code(void){
  return db_get("ticket-editpage", (char*)zDefaultEdit);
}

/*
** WEBPAGE: tktsetup_editpage
*/
void tktsetup_editpage_page(void){
  static const char zDesc[] =
  @ <p>Enter HTML with embedded TH1 script that will render the "edit ticket"
  @ page</p>
  ;
  tktsetup_generic(
    "HTML For Editing Tickets",
    "ticket-editpage",
    zDefaultEdit,
    zDesc,
    0,
    0,
    40
  );
}

/*
** The default template ticket report format:
*/
static char zDefaultReport[] = 
@ SELECT
@   CASE WHEN status IN ('Open','Verified') THEN '#f2dcdc'
@        WHEN status='Review' THEN '#e8e8e8'
@        WHEN status='Fixed' THEN '#cfe8bd'
@        WHEN status='Tested' THEN '#bde5d6'
@        WHEN status='Deferred' THEN '#cacae5'
@        ELSE '#c8c8c8' END AS 'bgcolor',
@   substr(tkt_uuid,1,10) AS '#',
@   datetime(tkt_mtime) AS 'mtime',
@   type,
@   status,
@   subsystem,
@   title,
@   comment AS '_comments'
@ FROM ticket
;


/*
** Return the template ticket report format:
*/
char *ticket_report_template(void){
  return db_get("ticket-report-template", zDefaultReport);
}

/*
** WEBPAGE: tktsetup_rpttplt
*/
void tktsetup_rpttplt_page(void){
  static const char zDesc[] =
  @ <p>Enter the default ticket report format template.  This is the
  @ the template report format that initially appears when creating a
  @ new ticket summary report.</p>
  ;
  tktsetup_generic(
    "Default Report Template",
    "ticket-report-template",
    zDefaultReport,
    zDesc,
    0,
    0,
    20
  );
}

/*
** The default template ticket key:
*/
static const char zDefaultKey[] = 
@ #ffffff Key:
@ #f2dcdc Active
@ #e8e8e8 Review
@ #cfe8bd Fixed
@ #bde5d6 Tested
@ #cacae5 Deferred
@ #c8c8c8 Closed
;


/*
** Return the template ticket report format:
*/
const char *ticket_key_template(void){
  return db_get("ticket-key-template", (char*)zDefaultKey);
}

/*
** WEBPAGE: tktsetup_keytplt
*/
void tktsetup_keytplt_page(void){
  static const char zDesc[] =
  @ <p>Enter the default ticket report color-key template.  This is the
  @ the color-key that initially appears when creating a
  @ new ticket summary report.</p>
  ;
  tktsetup_generic(
    "Default Report Color-Key Template",
    "ticket-key-template",
    zDefaultKey,
    zDesc,
    0,
    0,
    10
  );
}

/*
** WEBPAGE: tktsetup_timeline
*/
void tktsetup_timeline_page(void){
  login_check_credentials();
  if( !g.okSetup ){
    login_needed();
  }

  if( P("setup") ){
    cgi_redirect("tktsetup");
  }
  style_header("Ticket Display On Timelines");
  db_begin_transaction();
  @ <form action="%s(g.zBaseURL)/tktsetup_timeline" method="POST">
  login_insert_csrf_secret();

  @ <hr>
  entry_attribute("Ticket Title", 40, "ticket-title-expr", "t", "title");
  @ <p>An SQL expression in a query against the TICKET table that will
  @ return the title of the ticket for display purposes.</p>

  @ <hr>
  entry_attribute("Ticket Status", 40, "ticket-status-column", "s", "status");
  @ <p>The name of the column in the TICKET table that contains the ticket
  @ status in human-readable form.  Case sensitive.</p>

  @ <hr>
  entry_attribute("Ticket Closed", 40, "ticket-closed-expr", "c",
                  "status='Closed'");
  @ <p>An SQL expression that evaluates to true in a TICKET table query if
  @ the ticket is closed.</p>

  @ <hr>
  @ <p>
  @ <input type="submit"  name="submit" value="Apply Changes">
  @ <input type="submit" name="setup" value="Cancel">
  @ </p>
  @ </form>
  db_end_transaction(0);
  style_footer();
  
}
