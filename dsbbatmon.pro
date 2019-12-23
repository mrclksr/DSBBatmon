PROGRAM = dsbbatmon

isEmpty(PREFIX) {  
	PREFIX="/usr/local"
}

isEmpty(DATADIR) {  
	DATADIR=$${PREFIX}/share/$${PROGRAM}                                    
}                   

!isEmpty(TEST) {
	DEFINES += TEST=1
}

TARGET	     = $${PROGRAM}
PATH_LOCK    = .$${PROGRAM}.lock
APPSDIR	     = $${PREFIX}/share/applications
INSTALLS     = target desktopfile locales
TRANSLATIONS = locale/$${PROGRAM}_de.ts \
               locale/$${PROGRAM}_fr.ts
TEMPLATE     = app
QT	    += widgets
INCLUDEPATH += . lib src
DEFINES     += PROGRAM=\\\"$${PROGRAM}\\\" LOCALE_PATH=\\\"$${DATADIR}\\\"
DEFINES	    += PATH_LOCK=\\\"$${PATH_LOCK}\\\"
QMAKE_POST_LINK = $(STRIP) $(TARGET)
QMAKE_EXTRA_TARGETS += distclean cleanqm readme readmemd

HEADERS += src/battindicator.h \
           src/preferences.h \
	   src/countdown.h \
           lib/dsbbatmon.h \
           lib/config.h \
           lib/dsbcfg/dsbcfg.h \
           lib/qt-helper/qt-helper.h
SOURCES += src/battindicator.cpp \
	   src/countdown.cpp \
           src/main.cpp \
           src/preferences.cpp \
           lib/dsbbatmon.c \
           lib/config.c \
           lib/dsbcfg/dsbcfg.c \
           lib/qt-helper/qt-helper.cpp

target.files      = $${PROGRAM}         
target.path       = $${PREFIX}/bin      

desktopfile.path  = $${APPSDIR}         
desktopfile.files = $${PROGRAM}.desktop 

locales.path = $${DATADIR}

readme.target = readme
readme.files = readme.mdoc
readme.commands = mandoc -mdoc readme.mdoc | perl -e \'foreach (<STDIN>) { \
		\$$_ =~ s/(.)\x08\1/\$$1/g; \$$_ =~ s/_\x08(.)/\$$1/g; \
		print \$$_ \
	}\' | sed \'1,1d; \$$,\$$d\' > README

readmemd.target = readmemd
readmemd.files = readme.mdoc
readmemd.commands = mandoc -mdoc -Tmarkdown readme.mdoc | \
			sed -e \'1,1d; \$$,\$$d\' > README.md

qtPrepareTool(LRELEASE, lrelease)
for(a, TRANSLATIONS) {
	cmd = $$LRELEASE $${a}
	system($$cmd)
}
locales.files += locale/*.qm

cleanqm.commands  = rm -f $${locales.files}
distclean.depends = cleanqm

