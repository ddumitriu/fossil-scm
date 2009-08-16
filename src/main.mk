# DO NOT EDIT
#
# This file is automatically generated.  Instead of editing this
# file, edit "makemake.tcl" then run "tclsh makemake.tcl >main.mk"
# to regenerate this file.
#
# This file is included by linux-gcc.mk or linux-mingw.mk or possible
# some other makefiles.  This file contains the rules that are common
# to building regardless of the target.
#

XTCC = $(TCC) $(CFLAGS) -I. -I$(SRCDIR)


SRC = \
  $(SRCDIR)/add.c \
  $(SRCDIR)/allrepo.c \
  $(SRCDIR)/bag.c \
  $(SRCDIR)/blob.c \
  $(SRCDIR)/branch.c \
  $(SRCDIR)/browse.c \
  $(SRCDIR)/captcha.c \
  $(SRCDIR)/cgi.c \
  $(SRCDIR)/checkin.c \
  $(SRCDIR)/checkout.c \
  $(SRCDIR)/clearsign.c \
  $(SRCDIR)/clone.c \
  $(SRCDIR)/comformat.c \
  $(SRCDIR)/configure.c \
  $(SRCDIR)/construct.c \
  $(SRCDIR)/content.c \
  $(SRCDIR)/db.c \
  $(SRCDIR)/delta.c \
  $(SRCDIR)/deltacmd.c \
  $(SRCDIR)/descendants.c \
  $(SRCDIR)/diff.c \
  $(SRCDIR)/diffcmd.c \
  $(SRCDIR)/doc.c \
  $(SRCDIR)/encode.c \
  $(SRCDIR)/file.c \
  $(SRCDIR)/http.c \
  $(SRCDIR)/http_socket.c \
  $(SRCDIR)/http_transport.c \
  $(SRCDIR)/info.c \
  $(SRCDIR)/login.c \
  $(SRCDIR)/main.c \
  $(SRCDIR)/manifest.c \
  $(SRCDIR)/md5.c \
  $(SRCDIR)/merge.c \
  $(SRCDIR)/merge3.c \
  $(SRCDIR)/name.c \
  $(SRCDIR)/pivot.c \
  $(SRCDIR)/pqueue.c \
  $(SRCDIR)/printf.c \
  $(SRCDIR)/rebuild.c \
  $(SRCDIR)/report.c \
  $(SRCDIR)/rss.c \
  $(SRCDIR)/rstats.c \
  $(SRCDIR)/schema.c \
  $(SRCDIR)/setup.c \
  $(SRCDIR)/sha1.c \
  $(SRCDIR)/shun.c \
  $(SRCDIR)/stat.c \
  $(SRCDIR)/style.c \
  $(SRCDIR)/sync.c \
  $(SRCDIR)/tag.c \
  $(SRCDIR)/th_main.c \
  $(SRCDIR)/timeline.c \
  $(SRCDIR)/tkt.c \
  $(SRCDIR)/tktsetup.c \
  $(SRCDIR)/undo.c \
  $(SRCDIR)/update.c \
  $(SRCDIR)/url.c \
  $(SRCDIR)/user.c \
  $(SRCDIR)/verify.c \
  $(SRCDIR)/vfile.c \
  $(SRCDIR)/wiki.c \
  $(SRCDIR)/wikiformat.c \
  $(SRCDIR)/winhttp.c \
  $(SRCDIR)/xfer.c \
  $(SRCDIR)/zip.c

TRANS_SRC = \
  add_.c \
  allrepo_.c \
  bag_.c \
  blob_.c \
  branch_.c \
  browse_.c \
  captcha_.c \
  cgi_.c \
  checkin_.c \
  checkout_.c \
  clearsign_.c \
  clone_.c \
  comformat_.c \
  configure_.c \
  construct_.c \
  content_.c \
  db_.c \
  delta_.c \
  deltacmd_.c \
  descendants_.c \
  diff_.c \
  diffcmd_.c \
  doc_.c \
  encode_.c \
  file_.c \
  http_.c \
  http_socket_.c \
  http_transport_.c \
  info_.c \
  login_.c \
  main_.c \
  manifest_.c \
  md5_.c \
  merge_.c \
  merge3_.c \
  name_.c \
  pivot_.c \
  pqueue_.c \
  printf_.c \
  rebuild_.c \
  report_.c \
  rss_.c \
  rstats_.c \
  schema_.c \
  setup_.c \
  sha1_.c \
  shun_.c \
  stat_.c \
  style_.c \
  sync_.c \
  tag_.c \
  th_main_.c \
  timeline_.c \
  tkt_.c \
  tktsetup_.c \
  undo_.c \
  update_.c \
  url_.c \
  user_.c \
  verify_.c \
  vfile_.c \
  wiki_.c \
  wikiformat_.c \
  winhttp_.c \
  xfer_.c \
  zip_.c

OBJ = \
  add.o \
  allrepo.o \
  bag.o \
  blob.o \
  branch.o \
  browse.o \
  captcha.o \
  cgi.o \
  checkin.o \
  checkout.o \
  clearsign.o \
  clone.o \
  comformat.o \
  configure.o \
  construct.o \
  content.o \
  db.o \
  delta.o \
  deltacmd.o \
  descendants.o \
  diff.o \
  diffcmd.o \
  doc.o \
  encode.o \
  file.o \
  http.o \
  http_socket.o \
  http_transport.o \
  info.o \
  login.o \
  main.o \
  manifest.o \
  md5.o \
  merge.o \
  merge3.o \
  name.o \
  pivot.o \
  pqueue.o \
  printf.o \
  rebuild.o \
  report.o \
  rss.o \
  rstats.o \
  schema.o \
  setup.o \
  sha1.o \
  shun.o \
  stat.o \
  style.o \
  sync.o \
  tag.o \
  th_main.o \
  timeline.o \
  tkt.o \
  tktsetup.o \
  undo.o \
  update.o \
  url.o \
  user.o \
  verify.o \
  vfile.o \
  wiki.o \
  wikiformat.o \
  winhttp.o \
  xfer.o \
  zip.o

APPNAME = fossil$(E)



all:	$(APPNAME)

install:	$(APPNAME)
	mv $(APPNAME) $(INSTALLDIR)

translate:	$(SRCDIR)/translate.c
	$(BCC) -o translate $(SRCDIR)/translate.c

makeheaders:	$(SRCDIR)/makeheaders.c
	$(BCC) -o makeheaders $(SRCDIR)/makeheaders.c

mkindex:	$(SRCDIR)/mkindex.c
	$(BCC) -o mkindex $(SRCDIR)/mkindex.c

# WARNING. DANGER. Running the testsuite modifies the repository the
# build is done from, i.e. the checkout belongs to. Do not sync/push
# the repository after running the tests.
test:	$(APPNAME)
	$(TCLSH) test/tester.tcl $(APPNAME)

VERSION.h:	$(SRCDIR)/../manifest.uuid $(SRCDIR)/../manifest
	awk '{ printf "#define MANIFEST_UUID \"%s\"\n", $$1}'  $(SRCDIR)/../manifest.uuid >VERSION.h
	awk '{ printf "#define MANIFEST_VERSION \"[%.10s]\"\n", $$1}'  $(SRCDIR)/../manifest.uuid >>VERSION.h
	awk '$$1=="D"{printf "#define MANIFEST_DATE \"%s %s\"\n", substr($$2,1,10),substr($$2,12)}'  $(SRCDIR)/../manifest >>VERSION.h

$(APPNAME):	headers $(OBJ) sqlite3.o th.o th_lang.o
	$(TCC) -o $(APPNAME) $(OBJ) sqlite3.o th.o th_lang.o $(LIB)

# This rule prevents make from using its default rules to try build
# an executable named "manifest" out of the file named "manifest.c"
#
$(SRCDIR)/../manifest:	
	# noop

clean:	
	rm -f *.o *_.c $(APPNAME) VERSION.h
	rm -f translate makeheaders mkindex page_index.h headers
	rm -f add.h allrepo.h bag.h blob.h branch.h browse.h captcha.h cgi.h checkin.h checkout.h clearsign.h clone.h comformat.h configure.h construct.h content.h db.h delta.h deltacmd.h descendants.h diff.h diffcmd.h doc.h encode.h file.h http.h http_socket.h http_transport.h info.h login.h main.h manifest.h md5.h merge.h merge3.h name.h pivot.h pqueue.h printf.h rebuild.h report.h rss.h rstats.h schema.h setup.h sha1.h shun.h stat.h style.h sync.h tag.h th_main.h timeline.h tkt.h tktsetup.h undo.h update.h url.h user.h verify.h vfile.h wiki.h wikiformat.h winhttp.h xfer.h zip.h

page_index.h: $(TRANS_SRC) mkindex
	./mkindex $(TRANS_SRC) >$@
headers:	page_index.h makeheaders VERSION.h
	./makeheaders  add_.c:add.h allrepo_.c:allrepo.h bag_.c:bag.h blob_.c:blob.h branch_.c:branch.h browse_.c:browse.h captcha_.c:captcha.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clearsign_.c:clearsign.h clone_.c:clone.h comformat_.c:comformat.h configure_.c:configure.h construct_.c:construct.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendants_.c:descendants.h diff_.c:diff.h diffcmd_.c:diffcmd.h doc_.c:doc.h encode_.c:encode.h file_.c:file.h http_.c:http.h http_socket_.c:http_socket.h http_transport_.c:http_transport.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h pqueue_.c:pqueue.h printf_.c:printf.h rebuild_.c:rebuild.h report_.c:report.h rss_.c:rss.h rstats_.c:rstats.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h shun_.c:shun.h stat_.c:stat.h style_.c:style.h sync_.c:sync.h tag_.c:tag.h th_main_.c:th_main.h timeline_.c:timeline.h tkt_.c:tkt.h tktsetup_.c:tktsetup.h undo_.c:undo.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h winhttp_.c:winhttp.h xfer_.c:xfer.h zip_.c:zip.h $(SRCDIR)/sqlite3.h $(SRCDIR)/th.h VERSION.h
	touch headers
headers: Makefile
Makefile:
add_.c:	$(SRCDIR)/add.c translate
	./translate $(SRCDIR)/add.c >add_.c

add.o:	add_.c add.h  $(SRCDIR)/config.h
	$(XTCC) -o add.o -c add_.c

add.h:	headers
allrepo_.c:	$(SRCDIR)/allrepo.c translate
	./translate $(SRCDIR)/allrepo.c >allrepo_.c

allrepo.o:	allrepo_.c allrepo.h  $(SRCDIR)/config.h
	$(XTCC) -o allrepo.o -c allrepo_.c

allrepo.h:	headers
bag_.c:	$(SRCDIR)/bag.c translate
	./translate $(SRCDIR)/bag.c >bag_.c

bag.o:	bag_.c bag.h  $(SRCDIR)/config.h
	$(XTCC) -o bag.o -c bag_.c

bag.h:	headers
blob_.c:	$(SRCDIR)/blob.c translate
	./translate $(SRCDIR)/blob.c >blob_.c

blob.o:	blob_.c blob.h  $(SRCDIR)/config.h
	$(XTCC) -o blob.o -c blob_.c

blob.h:	headers
branch_.c:	$(SRCDIR)/branch.c translate
	./translate $(SRCDIR)/branch.c >branch_.c

branch.o:	branch_.c branch.h  $(SRCDIR)/config.h
	$(XTCC) -o branch.o -c branch_.c

branch.h:	headers
browse_.c:	$(SRCDIR)/browse.c translate
	./translate $(SRCDIR)/browse.c >browse_.c

browse.o:	browse_.c browse.h  $(SRCDIR)/config.h
	$(XTCC) -o browse.o -c browse_.c

browse.h:	headers
captcha_.c:	$(SRCDIR)/captcha.c translate
	./translate $(SRCDIR)/captcha.c >captcha_.c

captcha.o:	captcha_.c captcha.h  $(SRCDIR)/config.h
	$(XTCC) -o captcha.o -c captcha_.c

captcha.h:	headers
cgi_.c:	$(SRCDIR)/cgi.c translate
	./translate $(SRCDIR)/cgi.c >cgi_.c

cgi.o:	cgi_.c cgi.h  $(SRCDIR)/config.h
	$(XTCC) -o cgi.o -c cgi_.c

cgi.h:	headers
checkin_.c:	$(SRCDIR)/checkin.c translate
	./translate $(SRCDIR)/checkin.c >checkin_.c

checkin.o:	checkin_.c checkin.h  $(SRCDIR)/config.h
	$(XTCC) -o checkin.o -c checkin_.c

checkin.h:	headers
checkout_.c:	$(SRCDIR)/checkout.c translate
	./translate $(SRCDIR)/checkout.c >checkout_.c

checkout.o:	checkout_.c checkout.h  $(SRCDIR)/config.h
	$(XTCC) -o checkout.o -c checkout_.c

checkout.h:	headers
clearsign_.c:	$(SRCDIR)/clearsign.c translate
	./translate $(SRCDIR)/clearsign.c >clearsign_.c

clearsign.o:	clearsign_.c clearsign.h  $(SRCDIR)/config.h
	$(XTCC) -o clearsign.o -c clearsign_.c

clearsign.h:	headers
clone_.c:	$(SRCDIR)/clone.c translate
	./translate $(SRCDIR)/clone.c >clone_.c

clone.o:	clone_.c clone.h  $(SRCDIR)/config.h
	$(XTCC) -o clone.o -c clone_.c

clone.h:	headers
comformat_.c:	$(SRCDIR)/comformat.c translate
	./translate $(SRCDIR)/comformat.c >comformat_.c

comformat.o:	comformat_.c comformat.h  $(SRCDIR)/config.h
	$(XTCC) -o comformat.o -c comformat_.c

comformat.h:	headers
configure_.c:	$(SRCDIR)/configure.c translate
	./translate $(SRCDIR)/configure.c >configure_.c

configure.o:	configure_.c configure.h  $(SRCDIR)/config.h
	$(XTCC) -o configure.o -c configure_.c

configure.h:	headers
construct_.c:	$(SRCDIR)/construct.c translate
	./translate $(SRCDIR)/construct.c >construct_.c

construct.o:	construct_.c construct.h  $(SRCDIR)/config.h
	$(XTCC) -o construct.o -c construct_.c

construct.h:	headers
content_.c:	$(SRCDIR)/content.c translate
	./translate $(SRCDIR)/content.c >content_.c

content.o:	content_.c content.h  $(SRCDIR)/config.h
	$(XTCC) -o content.o -c content_.c

content.h:	headers
db_.c:	$(SRCDIR)/db.c translate
	./translate $(SRCDIR)/db.c >db_.c

db.o:	db_.c db.h  $(SRCDIR)/config.h
	$(XTCC) -o db.o -c db_.c

db.h:	headers
delta_.c:	$(SRCDIR)/delta.c translate
	./translate $(SRCDIR)/delta.c >delta_.c

delta.o:	delta_.c delta.h  $(SRCDIR)/config.h
	$(XTCC) -o delta.o -c delta_.c

delta.h:	headers
deltacmd_.c:	$(SRCDIR)/deltacmd.c translate
	./translate $(SRCDIR)/deltacmd.c >deltacmd_.c

deltacmd.o:	deltacmd_.c deltacmd.h  $(SRCDIR)/config.h
	$(XTCC) -o deltacmd.o -c deltacmd_.c

deltacmd.h:	headers
descendants_.c:	$(SRCDIR)/descendants.c translate
	./translate $(SRCDIR)/descendants.c >descendants_.c

descendants.o:	descendants_.c descendants.h  $(SRCDIR)/config.h
	$(XTCC) -o descendants.o -c descendants_.c

descendants.h:	headers
diff_.c:	$(SRCDIR)/diff.c translate
	./translate $(SRCDIR)/diff.c >diff_.c

diff.o:	diff_.c diff.h  $(SRCDIR)/config.h
	$(XTCC) -o diff.o -c diff_.c

diff.h:	headers
diffcmd_.c:	$(SRCDIR)/diffcmd.c translate
	./translate $(SRCDIR)/diffcmd.c >diffcmd_.c

diffcmd.o:	diffcmd_.c diffcmd.h  $(SRCDIR)/config.h
	$(XTCC) -o diffcmd.o -c diffcmd_.c

diffcmd.h:	headers
doc_.c:	$(SRCDIR)/doc.c translate
	./translate $(SRCDIR)/doc.c >doc_.c

doc.o:	doc_.c doc.h  $(SRCDIR)/config.h
	$(XTCC) -o doc.o -c doc_.c

doc.h:	headers
encode_.c:	$(SRCDIR)/encode.c translate
	./translate $(SRCDIR)/encode.c >encode_.c

encode.o:	encode_.c encode.h  $(SRCDIR)/config.h
	$(XTCC) -o encode.o -c encode_.c

encode.h:	headers
file_.c:	$(SRCDIR)/file.c translate
	./translate $(SRCDIR)/file.c >file_.c

file.o:	file_.c file.h  $(SRCDIR)/config.h
	$(XTCC) -o file.o -c file_.c

file.h:	headers
http_.c:	$(SRCDIR)/http.c translate
	./translate $(SRCDIR)/http.c >http_.c

http.o:	http_.c http.h  $(SRCDIR)/config.h
	$(XTCC) -o http.o -c http_.c

http.h:	headers
http_socket_.c:	$(SRCDIR)/http_socket.c translate
	./translate $(SRCDIR)/http_socket.c >http_socket_.c

http_socket.o:	http_socket_.c http_socket.h  $(SRCDIR)/config.h
	$(XTCC) -o http_socket.o -c http_socket_.c

http_socket.h:	headers
http_transport_.c:	$(SRCDIR)/http_transport.c translate
	./translate $(SRCDIR)/http_transport.c >http_transport_.c

http_transport.o:	http_transport_.c http_transport.h  $(SRCDIR)/config.h
	$(XTCC) -o http_transport.o -c http_transport_.c

http_transport.h:	headers
info_.c:	$(SRCDIR)/info.c translate
	./translate $(SRCDIR)/info.c >info_.c

info.o:	info_.c info.h  $(SRCDIR)/config.h
	$(XTCC) -o info.o -c info_.c

info.h:	headers
login_.c:	$(SRCDIR)/login.c translate
	./translate $(SRCDIR)/login.c >login_.c

login.o:	login_.c login.h  $(SRCDIR)/config.h
	$(XTCC) -o login.o -c login_.c

login.h:	headers
main_.c:	$(SRCDIR)/main.c translate
	./translate $(SRCDIR)/main.c >main_.c

main.o:	main_.c main.h page_index.h $(SRCDIR)/config.h
	$(XTCC) -o main.o -c main_.c

main.h:	headers
manifest_.c:	$(SRCDIR)/manifest.c translate
	./translate $(SRCDIR)/manifest.c >manifest_.c

manifest.o:	manifest_.c manifest.h  $(SRCDIR)/config.h
	$(XTCC) -o manifest.o -c manifest_.c

manifest.h:	headers
md5_.c:	$(SRCDIR)/md5.c translate
	./translate $(SRCDIR)/md5.c >md5_.c

md5.o:	md5_.c md5.h  $(SRCDIR)/config.h
	$(XTCC) -o md5.o -c md5_.c

md5.h:	headers
merge_.c:	$(SRCDIR)/merge.c translate
	./translate $(SRCDIR)/merge.c >merge_.c

merge.o:	merge_.c merge.h  $(SRCDIR)/config.h
	$(XTCC) -o merge.o -c merge_.c

merge.h:	headers
merge3_.c:	$(SRCDIR)/merge3.c translate
	./translate $(SRCDIR)/merge3.c >merge3_.c

merge3.o:	merge3_.c merge3.h  $(SRCDIR)/config.h
	$(XTCC) -o merge3.o -c merge3_.c

merge3.h:	headers
name_.c:	$(SRCDIR)/name.c translate
	./translate $(SRCDIR)/name.c >name_.c

name.o:	name_.c name.h  $(SRCDIR)/config.h
	$(XTCC) -o name.o -c name_.c

name.h:	headers
pivot_.c:	$(SRCDIR)/pivot.c translate
	./translate $(SRCDIR)/pivot.c >pivot_.c

pivot.o:	pivot_.c pivot.h  $(SRCDIR)/config.h
	$(XTCC) -o pivot.o -c pivot_.c

pivot.h:	headers
pqueue_.c:	$(SRCDIR)/pqueue.c translate
	./translate $(SRCDIR)/pqueue.c >pqueue_.c

pqueue.o:	pqueue_.c pqueue.h  $(SRCDIR)/config.h
	$(XTCC) -o pqueue.o -c pqueue_.c

pqueue.h:	headers
printf_.c:	$(SRCDIR)/printf.c translate
	./translate $(SRCDIR)/printf.c >printf_.c

printf.o:	printf_.c printf.h  $(SRCDIR)/config.h
	$(XTCC) -o printf.o -c printf_.c

printf.h:	headers
rebuild_.c:	$(SRCDIR)/rebuild.c translate
	./translate $(SRCDIR)/rebuild.c >rebuild_.c

rebuild.o:	rebuild_.c rebuild.h  $(SRCDIR)/config.h
	$(XTCC) -o rebuild.o -c rebuild_.c

rebuild.h:	headers
report_.c:	$(SRCDIR)/report.c translate
	./translate $(SRCDIR)/report.c >report_.c

report.o:	report_.c report.h  $(SRCDIR)/config.h
	$(XTCC) -o report.o -c report_.c

report.h:	headers
rss_.c:	$(SRCDIR)/rss.c translate
	./translate $(SRCDIR)/rss.c >rss_.c

rss.o:	rss_.c rss.h  $(SRCDIR)/config.h
	$(XTCC) -o rss.o -c rss_.c

rss.h:	headers
rstats_.c:	$(SRCDIR)/rstats.c translate
	./translate $(SRCDIR)/rstats.c >rstats_.c

rstats.o:	rstats_.c rstats.h  $(SRCDIR)/config.h
	$(XTCC) -o rstats.o -c rstats_.c

rstats.h:	headers
schema_.c:	$(SRCDIR)/schema.c translate
	./translate $(SRCDIR)/schema.c >schema_.c

schema.o:	schema_.c schema.h  $(SRCDIR)/config.h
	$(XTCC) -o schema.o -c schema_.c

schema.h:	headers
setup_.c:	$(SRCDIR)/setup.c translate
	./translate $(SRCDIR)/setup.c >setup_.c

setup.o:	setup_.c setup.h  $(SRCDIR)/config.h
	$(XTCC) -o setup.o -c setup_.c

setup.h:	headers
sha1_.c:	$(SRCDIR)/sha1.c translate
	./translate $(SRCDIR)/sha1.c >sha1_.c

sha1.o:	sha1_.c sha1.h  $(SRCDIR)/config.h
	$(XTCC) -o sha1.o -c sha1_.c

sha1.h:	headers
shun_.c:	$(SRCDIR)/shun.c translate
	./translate $(SRCDIR)/shun.c >shun_.c

shun.o:	shun_.c shun.h  $(SRCDIR)/config.h
	$(XTCC) -o shun.o -c shun_.c

shun.h:	headers
stat_.c:	$(SRCDIR)/stat.c translate
	./translate $(SRCDIR)/stat.c >stat_.c

stat.o:	stat_.c stat.h  $(SRCDIR)/config.h
	$(XTCC) -o stat.o -c stat_.c

stat.h:	headers
style_.c:	$(SRCDIR)/style.c translate
	./translate $(SRCDIR)/style.c >style_.c

style.o:	style_.c style.h  $(SRCDIR)/config.h
	$(XTCC) -o style.o -c style_.c

style.h:	headers
sync_.c:	$(SRCDIR)/sync.c translate
	./translate $(SRCDIR)/sync.c >sync_.c

sync.o:	sync_.c sync.h  $(SRCDIR)/config.h
	$(XTCC) -o sync.o -c sync_.c

sync.h:	headers
tag_.c:	$(SRCDIR)/tag.c translate
	./translate $(SRCDIR)/tag.c >tag_.c

tag.o:	tag_.c tag.h  $(SRCDIR)/config.h
	$(XTCC) -o tag.o -c tag_.c

tag.h:	headers
th_main_.c:	$(SRCDIR)/th_main.c translate
	./translate $(SRCDIR)/th_main.c >th_main_.c

th_main.o:	th_main_.c th_main.h  $(SRCDIR)/config.h
	$(XTCC) -o th_main.o -c th_main_.c

th_main.h:	headers
timeline_.c:	$(SRCDIR)/timeline.c translate
	./translate $(SRCDIR)/timeline.c >timeline_.c

timeline.o:	timeline_.c timeline.h  $(SRCDIR)/config.h
	$(XTCC) -o timeline.o -c timeline_.c

timeline.h:	headers
tkt_.c:	$(SRCDIR)/tkt.c translate
	./translate $(SRCDIR)/tkt.c >tkt_.c

tkt.o:	tkt_.c tkt.h  $(SRCDIR)/config.h
	$(XTCC) -o tkt.o -c tkt_.c

tkt.h:	headers
tktsetup_.c:	$(SRCDIR)/tktsetup.c translate
	./translate $(SRCDIR)/tktsetup.c >tktsetup_.c

tktsetup.o:	tktsetup_.c tktsetup.h  $(SRCDIR)/config.h
	$(XTCC) -o tktsetup.o -c tktsetup_.c

tktsetup.h:	headers
undo_.c:	$(SRCDIR)/undo.c translate
	./translate $(SRCDIR)/undo.c >undo_.c

undo.o:	undo_.c undo.h  $(SRCDIR)/config.h
	$(XTCC) -o undo.o -c undo_.c

undo.h:	headers
update_.c:	$(SRCDIR)/update.c translate
	./translate $(SRCDIR)/update.c >update_.c

update.o:	update_.c update.h  $(SRCDIR)/config.h
	$(XTCC) -o update.o -c update_.c

update.h:	headers
url_.c:	$(SRCDIR)/url.c translate
	./translate $(SRCDIR)/url.c >url_.c

url.o:	url_.c url.h  $(SRCDIR)/config.h
	$(XTCC) -o url.o -c url_.c

url.h:	headers
user_.c:	$(SRCDIR)/user.c translate
	./translate $(SRCDIR)/user.c >user_.c

user.o:	user_.c user.h  $(SRCDIR)/config.h
	$(XTCC) -o user.o -c user_.c

user.h:	headers
verify_.c:	$(SRCDIR)/verify.c translate
	./translate $(SRCDIR)/verify.c >verify_.c

verify.o:	verify_.c verify.h  $(SRCDIR)/config.h
	$(XTCC) -o verify.o -c verify_.c

verify.h:	headers
vfile_.c:	$(SRCDIR)/vfile.c translate
	./translate $(SRCDIR)/vfile.c >vfile_.c

vfile.o:	vfile_.c vfile.h  $(SRCDIR)/config.h
	$(XTCC) -o vfile.o -c vfile_.c

vfile.h:	headers
wiki_.c:	$(SRCDIR)/wiki.c translate
	./translate $(SRCDIR)/wiki.c >wiki_.c

wiki.o:	wiki_.c wiki.h  $(SRCDIR)/config.h
	$(XTCC) -o wiki.o -c wiki_.c

wiki.h:	headers
wikiformat_.c:	$(SRCDIR)/wikiformat.c translate
	./translate $(SRCDIR)/wikiformat.c >wikiformat_.c

wikiformat.o:	wikiformat_.c wikiformat.h  $(SRCDIR)/config.h
	$(XTCC) -o wikiformat.o -c wikiformat_.c

wikiformat.h:	headers
winhttp_.c:	$(SRCDIR)/winhttp.c translate
	./translate $(SRCDIR)/winhttp.c >winhttp_.c

winhttp.o:	winhttp_.c winhttp.h  $(SRCDIR)/config.h
	$(XTCC) -o winhttp.o -c winhttp_.c

winhttp.h:	headers
xfer_.c:	$(SRCDIR)/xfer.c translate
	./translate $(SRCDIR)/xfer.c >xfer_.c

xfer.o:	xfer_.c xfer.h  $(SRCDIR)/config.h
	$(XTCC) -o xfer.o -c xfer_.c

xfer.h:	headers
zip_.c:	$(SRCDIR)/zip.c translate
	./translate $(SRCDIR)/zip.c >zip_.c

zip.o:	zip_.c zip.h  $(SRCDIR)/config.h
	$(XTCC) -o zip.o -c zip_.c

zip.h:	headers
sqlite3.o:	$(SRCDIR)/sqlite3.c
	$(XTCC) -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_PRIVATE= -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_FILE_FORMAT=4 -Dlocaltime=fossil_localtime -c $(SRCDIR)/sqlite3.c -o sqlite3.o

th.o:	$(SRCDIR)/th.c
	$(XTCC) -I$(SRCDIR) -c $(SRCDIR)/th.c -o th.o

th_lang.o:	$(SRCDIR)/th_lang.c
	$(XTCC) -I$(SRCDIR) -c $(SRCDIR)/th_lang.c -o th_lang.o

