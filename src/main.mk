# This file is included by linux-gcc.mk or linux-mingw.mk or possible
# some other makefiles.  This file contains the rules that are common
# to building regardless of the target.
#

XTCC = $(TCC) $(CFLAGS) -I. -I$(SRCDIR)


SRC = \
  $(SRCDIR)/add.c \
  $(SRCDIR)/blob.c \
  $(SRCDIR)/cgi.c \
  $(SRCDIR)/checkin.c \
  $(SRCDIR)/checkout.c \
  $(SRCDIR)/clone.c \
  $(SRCDIR)/comformat.c \
  $(SRCDIR)/content.c \
  $(SRCDIR)/db.c \
  $(SRCDIR)/delta.c \
  $(SRCDIR)/deltacmd.c \
  $(SRCDIR)/descendents.c \
  $(SRCDIR)/diff.c \
  $(SRCDIR)/diffcmd.c \
  $(SRCDIR)/encode.c \
  $(SRCDIR)/file.c \
  $(SRCDIR)/http.c \
  $(SRCDIR)/info.c \
  $(SRCDIR)/login.c \
  $(SRCDIR)/main.c \
  $(SRCDIR)/manifest.c \
  $(SRCDIR)/md5.c \
  $(SRCDIR)/merge.c \
  $(SRCDIR)/merge3.c \
  $(SRCDIR)/name.c \
  $(SRCDIR)/pivot.c \
  $(SRCDIR)/printf.c \
  $(SRCDIR)/rebuild.c \
  $(SRCDIR)/schema.c \
  $(SRCDIR)/setup.c \
  $(SRCDIR)/sha1.c \
  $(SRCDIR)/style.c \
  $(SRCDIR)/sync.c \
  $(SRCDIR)/timeline.c \
  $(SRCDIR)/update.c \
  $(SRCDIR)/url.c \
  $(SRCDIR)/user.c \
  $(SRCDIR)/verify.c \
  $(SRCDIR)/vfile.c \
  $(SRCDIR)/wiki.c \
  $(SRCDIR)/wikiformat.c \
  $(SRCDIR)/xfer.c

TRANS_SRC = \
  add_.c \
  blob_.c \
  cgi_.c \
  checkin_.c \
  checkout_.c \
  clone_.c \
  comformat_.c \
  content_.c \
  db_.c \
  delta_.c \
  deltacmd_.c \
  descendents_.c \
  diff_.c \
  diffcmd_.c \
  encode_.c \
  file_.c \
  http_.c \
  info_.c \
  login_.c \
  main_.c \
  manifest_.c \
  md5_.c \
  merge_.c \
  merge3_.c \
  name_.c \
  pivot_.c \
  printf_.c \
  rebuild_.c \
  schema_.c \
  setup_.c \
  sha1_.c \
  style_.c \
  sync_.c \
  timeline_.c \
  update_.c \
  url_.c \
  user_.c \
  verify_.c \
  vfile_.c \
  wiki_.c \
  wikiformat_.c \
  xfer_.c

OBJ = \
  add.o \
  blob.o \
  cgi.o \
  checkin.o \
  checkout.o \
  clone.o \
  comformat.o \
  content.o \
  db.o \
  delta.o \
  deltacmd.o \
  descendents.o \
  diff.o \
  diffcmd.o \
  encode.o \
  file.o \
  http.o \
  info.o \
  login.o \
  main.o \
  manifest.o \
  md5.o \
  merge.o \
  merge3.o \
  name.o \
  pivot.o \
  printf.o \
  rebuild.o \
  schema.o \
  setup.o \
  sha1.o \
  style.o \
  sync.o \
  timeline.o \
  update.o \
  url.o \
  user.o \
  verify.o \
  vfile.o \
  wiki.o \
  wikiformat.o \
  xfer.o

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

$(APPNAME):	headers $(OBJ) sqlite3.o
	$(TCC) -o $(APPNAME) $(OBJ) sqlite3.o $(LIB)

clean:	
	rm -f *.o *_.c $(APPNAME)
	rm -f translate makeheaders mkindex page_index.h headers
	rm -f add.h blob.h cgi.h checkin.h checkout.h clone.h comformat.h content.h db.h delta.h deltacmd.h descendents.h diff.h diffcmd.h encode.h file.h http.h info.h login.h main.h manifest.h md5.h merge.h merge3.h name.h pivot.h printf.h rebuild.h schema.h setup.h sha1.h style.h sync.h timeline.h update.h url.h user.h verify.h vfile.h wiki.h wikiformat.h xfer.h

headers:	makeheaders mkindex $(TRANS_SRC)
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	./mkindex $(TRANS_SRC) >page_index.h
	touch headers

add_.c:	$(SRCDIR)/add.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/add.c | sed -f $(SRCDIR)/VERSION >add_.c

add.o:	add_.c add.h  $(SRCDIR)/config.h
	$(XTCC) -o add.o -c add_.c

add.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

blob_.c:	$(SRCDIR)/blob.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/blob.c | sed -f $(SRCDIR)/VERSION >blob_.c

blob.o:	blob_.c blob.h  $(SRCDIR)/config.h
	$(XTCC) -o blob.o -c blob_.c

blob.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

cgi_.c:	$(SRCDIR)/cgi.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/cgi.c | sed -f $(SRCDIR)/VERSION >cgi_.c

cgi.o:	cgi_.c cgi.h  $(SRCDIR)/config.h
	$(XTCC) -o cgi.o -c cgi_.c

cgi.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

checkin_.c:	$(SRCDIR)/checkin.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/checkin.c | sed -f $(SRCDIR)/VERSION >checkin_.c

checkin.o:	checkin_.c checkin.h  $(SRCDIR)/config.h
	$(XTCC) -o checkin.o -c checkin_.c

checkin.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

checkout_.c:	$(SRCDIR)/checkout.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/checkout.c | sed -f $(SRCDIR)/VERSION >checkout_.c

checkout.o:	checkout_.c checkout.h  $(SRCDIR)/config.h
	$(XTCC) -o checkout.o -c checkout_.c

checkout.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

clone_.c:	$(SRCDIR)/clone.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/clone.c | sed -f $(SRCDIR)/VERSION >clone_.c

clone.o:	clone_.c clone.h  $(SRCDIR)/config.h
	$(XTCC) -o clone.o -c clone_.c

clone.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

comformat_.c:	$(SRCDIR)/comformat.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/comformat.c | sed -f $(SRCDIR)/VERSION >comformat_.c

comformat.o:	comformat_.c comformat.h  $(SRCDIR)/config.h
	$(XTCC) -o comformat.o -c comformat_.c

comformat.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

content_.c:	$(SRCDIR)/content.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/content.c | sed -f $(SRCDIR)/VERSION >content_.c

content.o:	content_.c content.h  $(SRCDIR)/config.h
	$(XTCC) -o content.o -c content_.c

content.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

db_.c:	$(SRCDIR)/db.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/db.c | sed -f $(SRCDIR)/VERSION >db_.c

db.o:	db_.c db.h  $(SRCDIR)/config.h
	$(XTCC) -o db.o -c db_.c

db.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

delta_.c:	$(SRCDIR)/delta.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/delta.c | sed -f $(SRCDIR)/VERSION >delta_.c

delta.o:	delta_.c delta.h  $(SRCDIR)/config.h
	$(XTCC) -o delta.o -c delta_.c

delta.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

deltacmd_.c:	$(SRCDIR)/deltacmd.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/deltacmd.c | sed -f $(SRCDIR)/VERSION >deltacmd_.c

deltacmd.o:	deltacmd_.c deltacmd.h  $(SRCDIR)/config.h
	$(XTCC) -o deltacmd.o -c deltacmd_.c

deltacmd.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

descendents_.c:	$(SRCDIR)/descendents.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/descendents.c | sed -f $(SRCDIR)/VERSION >descendents_.c

descendents.o:	descendents_.c descendents.h  $(SRCDIR)/config.h
	$(XTCC) -o descendents.o -c descendents_.c

descendents.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

diff_.c:	$(SRCDIR)/diff.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/diff.c | sed -f $(SRCDIR)/VERSION >diff_.c

diff.o:	diff_.c diff.h  $(SRCDIR)/config.h
	$(XTCC) -o diff.o -c diff_.c

diff.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

diffcmd_.c:	$(SRCDIR)/diffcmd.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/diffcmd.c | sed -f $(SRCDIR)/VERSION >diffcmd_.c

diffcmd.o:	diffcmd_.c diffcmd.h  $(SRCDIR)/config.h
	$(XTCC) -o diffcmd.o -c diffcmd_.c

diffcmd.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

encode_.c:	$(SRCDIR)/encode.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/encode.c | sed -f $(SRCDIR)/VERSION >encode_.c

encode.o:	encode_.c encode.h  $(SRCDIR)/config.h
	$(XTCC) -o encode.o -c encode_.c

encode.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

file_.c:	$(SRCDIR)/file.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/file.c | sed -f $(SRCDIR)/VERSION >file_.c

file.o:	file_.c file.h  $(SRCDIR)/config.h
	$(XTCC) -o file.o -c file_.c

file.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

http_.c:	$(SRCDIR)/http.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/http.c | sed -f $(SRCDIR)/VERSION >http_.c

http.o:	http_.c http.h  $(SRCDIR)/config.h
	$(XTCC) -o http.o -c http_.c

http.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

info_.c:	$(SRCDIR)/info.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/info.c | sed -f $(SRCDIR)/VERSION >info_.c

info.o:	info_.c info.h  $(SRCDIR)/config.h
	$(XTCC) -o info.o -c info_.c

info.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

login_.c:	$(SRCDIR)/login.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/login.c | sed -f $(SRCDIR)/VERSION >login_.c

login.o:	login_.c login.h  $(SRCDIR)/config.h
	$(XTCC) -o login.o -c login_.c

login.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

main_.c:	$(SRCDIR)/main.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/main.c | sed -f $(SRCDIR)/VERSION >main_.c

main.o:	main_.c main.h page_index.h $(SRCDIR)/config.h
	$(XTCC) -o main.o -c main_.c

main.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

manifest_.c:	$(SRCDIR)/manifest.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/manifest.c | sed -f $(SRCDIR)/VERSION >manifest_.c

manifest.o:	manifest_.c manifest.h  $(SRCDIR)/config.h
	$(XTCC) -o manifest.o -c manifest_.c

manifest.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

md5_.c:	$(SRCDIR)/md5.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/md5.c | sed -f $(SRCDIR)/VERSION >md5_.c

md5.o:	md5_.c md5.h  $(SRCDIR)/config.h
	$(XTCC) -o md5.o -c md5_.c

md5.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

merge_.c:	$(SRCDIR)/merge.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/merge.c | sed -f $(SRCDIR)/VERSION >merge_.c

merge.o:	merge_.c merge.h  $(SRCDIR)/config.h
	$(XTCC) -o merge.o -c merge_.c

merge.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

merge3_.c:	$(SRCDIR)/merge3.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/merge3.c | sed -f $(SRCDIR)/VERSION >merge3_.c

merge3.o:	merge3_.c merge3.h  $(SRCDIR)/config.h
	$(XTCC) -o merge3.o -c merge3_.c

merge3.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

name_.c:	$(SRCDIR)/name.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/name.c | sed -f $(SRCDIR)/VERSION >name_.c

name.o:	name_.c name.h  $(SRCDIR)/config.h
	$(XTCC) -o name.o -c name_.c

name.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

pivot_.c:	$(SRCDIR)/pivot.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/pivot.c | sed -f $(SRCDIR)/VERSION >pivot_.c

pivot.o:	pivot_.c pivot.h  $(SRCDIR)/config.h
	$(XTCC) -o pivot.o -c pivot_.c

pivot.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

printf_.c:	$(SRCDIR)/printf.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/printf.c | sed -f $(SRCDIR)/VERSION >printf_.c

printf.o:	printf_.c printf.h  $(SRCDIR)/config.h
	$(XTCC) -o printf.o -c printf_.c

printf.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

rebuild_.c:	$(SRCDIR)/rebuild.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/rebuild.c | sed -f $(SRCDIR)/VERSION >rebuild_.c

rebuild.o:	rebuild_.c rebuild.h  $(SRCDIR)/config.h
	$(XTCC) -o rebuild.o -c rebuild_.c

rebuild.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

schema_.c:	$(SRCDIR)/schema.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/schema.c | sed -f $(SRCDIR)/VERSION >schema_.c

schema.o:	schema_.c schema.h  $(SRCDIR)/config.h
	$(XTCC) -o schema.o -c schema_.c

schema.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

setup_.c:	$(SRCDIR)/setup.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/setup.c | sed -f $(SRCDIR)/VERSION >setup_.c

setup.o:	setup_.c setup.h  $(SRCDIR)/config.h
	$(XTCC) -o setup.o -c setup_.c

setup.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

sha1_.c:	$(SRCDIR)/sha1.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/sha1.c | sed -f $(SRCDIR)/VERSION >sha1_.c

sha1.o:	sha1_.c sha1.h  $(SRCDIR)/config.h
	$(XTCC) -o sha1.o -c sha1_.c

sha1.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

style_.c:	$(SRCDIR)/style.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/style.c | sed -f $(SRCDIR)/VERSION >style_.c

style.o:	style_.c style.h  $(SRCDIR)/config.h
	$(XTCC) -o style.o -c style_.c

style.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

sync_.c:	$(SRCDIR)/sync.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/sync.c | sed -f $(SRCDIR)/VERSION >sync_.c

sync.o:	sync_.c sync.h  $(SRCDIR)/config.h
	$(XTCC) -o sync.o -c sync_.c

sync.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

timeline_.c:	$(SRCDIR)/timeline.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/timeline.c | sed -f $(SRCDIR)/VERSION >timeline_.c

timeline.o:	timeline_.c timeline.h  $(SRCDIR)/config.h
	$(XTCC) -o timeline.o -c timeline_.c

timeline.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

update_.c:	$(SRCDIR)/update.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/update.c | sed -f $(SRCDIR)/VERSION >update_.c

update.o:	update_.c update.h  $(SRCDIR)/config.h
	$(XTCC) -o update.o -c update_.c

update.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

url_.c:	$(SRCDIR)/url.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/url.c | sed -f $(SRCDIR)/VERSION >url_.c

url.o:	url_.c url.h  $(SRCDIR)/config.h
	$(XTCC) -o url.o -c url_.c

url.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

user_.c:	$(SRCDIR)/user.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/user.c | sed -f $(SRCDIR)/VERSION >user_.c

user.o:	user_.c user.h  $(SRCDIR)/config.h
	$(XTCC) -o user.o -c user_.c

user.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

verify_.c:	$(SRCDIR)/verify.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/verify.c | sed -f $(SRCDIR)/VERSION >verify_.c

verify.o:	verify_.c verify.h  $(SRCDIR)/config.h
	$(XTCC) -o verify.o -c verify_.c

verify.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

vfile_.c:	$(SRCDIR)/vfile.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/vfile.c | sed -f $(SRCDIR)/VERSION >vfile_.c

vfile.o:	vfile_.c vfile.h  $(SRCDIR)/config.h
	$(XTCC) -o vfile.o -c vfile_.c

vfile.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

wiki_.c:	$(SRCDIR)/wiki.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/wiki.c | sed -f $(SRCDIR)/VERSION >wiki_.c

wiki.o:	wiki_.c wiki.h  $(SRCDIR)/config.h
	$(XTCC) -o wiki.o -c wiki_.c

wiki.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

wikiformat_.c:	$(SRCDIR)/wikiformat.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/wikiformat.c | sed -f $(SRCDIR)/VERSION >wikiformat_.c

wikiformat.o:	wikiformat_.c wikiformat.h  $(SRCDIR)/config.h
	$(XTCC) -o wikiformat.o -c wikiformat_.c

wikiformat.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

xfer_.c:	$(SRCDIR)/xfer.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/xfer.c | sed -f $(SRCDIR)/VERSION >xfer_.c

xfer.o:	xfer_.c xfer.h  $(SRCDIR)/config.h
	$(XTCC) -o xfer.o -c xfer_.c

xfer.h:	makeheaders
	./makeheaders  add_.c:add.h blob_.c:blob.h cgi_.c:cgi.h checkin_.c:checkin.h checkout_.c:checkout.h clone_.c:clone.h comformat_.c:comformat.h content_.c:content.h db_.c:db.h delta_.c:delta.h deltacmd_.c:deltacmd.h descendents_.c:descendents.h diff_.c:diff.h diffcmd_.c:diffcmd.h encode_.c:encode.h file_.c:file.h http_.c:http.h info_.c:info.h login_.c:login.h main_.c:main.h manifest_.c:manifest.h md5_.c:md5.h merge_.c:merge.h merge3_.c:merge3.h name_.c:name.h pivot_.c:pivot.h printf_.c:printf.h rebuild_.c:rebuild.h schema_.c:schema.h setup_.c:setup.h sha1_.c:sha1.h style_.c:style.h sync_.c:sync.h timeline_.c:timeline.h update_.c:update.h url_.c:url.h user_.c:user.h verify_.c:verify.h vfile_.c:vfile.h wiki_.c:wiki.h wikiformat_.c:wikiformat.h xfer_.c:xfer.h $(SRCDIR)/sqlite3.h
	touch headers

sqlite3.o:	$(SRCDIR)/sqlite3.c
	$(XTCC) -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_PRIVATE= -DTHREADSAFE=0 -DSQLITE_DEFAULT_FILE_FORMAT=4 -c $(SRCDIR)/sqlite3.c -o sqlite3.o

