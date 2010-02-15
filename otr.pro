TRANSLATIONS = qutim_otr_ru.ts
TARGET = otr
TEMPLATE = lib
HEADERS += otrcrypt.h \
    psi/psiotrclosure.h \
    psi/OtrMessaging.hpp \
    psi/OtrInternal.hpp \
    smpdialog.h \
    settingswidget.h \
    global.h
unix:INCLUDEPATH += /usr/include/libotr \
    /usr/include
windows:INCLUDEPATH += C:/otr/w32root/include
windows:QMAKE_LIBDIR += C:/otr/w32root/lib
SOURCES += otrcrypt.cpp \
    psi/psiotrclosure.cpp \
    psi/OtrMessaging.cpp \
    psi/OtrInternal.cpp \
    smpdialog.cpp \
    settingswidget.cpp
CONFIG += qt \
    plugin
QT += core \
    gui
LIBS += -lotr
windows:LIBS += -lgcrypt
RESOURCES += icons.qrc
FORMS += smpdialog.ui \
    settingswidget.ui
