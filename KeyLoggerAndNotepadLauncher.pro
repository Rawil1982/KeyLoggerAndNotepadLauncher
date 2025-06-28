QT += core
CONFIG += c++11
TARGET = KeyLoggerAndNotepadLauncher
SOURCES += main.cpp

win32 {
    LIBS += -luser32 -lkernel32

    CONFIG -= console
    QMAKE_LFLAGS += -Wl,-subsystem,windows

    msvc: QMAKE_LFLAGS += /SUBSYSTEM:WINDOWS
}
