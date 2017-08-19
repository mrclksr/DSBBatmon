isEmpty(PREFIX) {  
	PREFIX="/usr/local"
}

isEmpty(DATADIR) {  
	DATADIR=$${PREFIX}/share/$${PROGRAM}                                    
}                   

PROGRAM	     = dsbbatmon
TARGET	     = $${PROGRAM}
APPSDIR	     = $${PREFIX}/share/applications
INSTALLS     = target desktopfile locales
TRANSLATIONS = locale/$${PROGRAM}_de.ts
LANGUAGES    = de
TEMPLATE     = app
QT	    += widgets
INCLUDEPATH += . lib src
DEFINES     += PROGRAM=\\\"$${PROGRAM}\\\" LOCALE_PATH=\\\"$${DATADIR}\\\"
QMAKE_POST_LINK = $(STRIP) $(TARGET)
QMAKE_EXTRA_TARGETS += distclean cleanqm

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

qtPrepareTool(LRELEASE, lrelease)
for(a, LANGUAGES) {
	in  = locale/$${PROGRAM}_$${a}.ts
	out = locale/$${PROGRAM}_$${a}.qm
	locales.files += $$out
	cmd = $$LRELEASE $$in -qm $$out
	system($$cmd)
}
cleanqm.commands  = rm -f $${locales.files}
distclean.depends = cleanqm

