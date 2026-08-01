QT += core sql
CONFIG += console c++11
CONFIG -= app_bundle

TARGET = test_dm
TEMPLATE = app

INCLUDEPATH += ../../src

SOURCES += \
    test_dm.cpp \
    ../../src/core/datamanager.cpp

HEADERS += \
    ../../src/core/datamanager.h \
    ../../src/core/plateutil.h \
    ../../src/core/user.h \
    ../../src/core/vehicle.h
