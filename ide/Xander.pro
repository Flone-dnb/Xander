QT += core gui
QT += printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../ext/qcustomplot/qcustomplot.cpp \
    ../src/Controller/controller.cpp \
    ../src/Model/AudioCore/audiocore.cpp \
    ../src/Model/AudioEngine/SAudioEngine/saudioengine.cpp \
    ../src/Model/AudioEngine/SSound/ssound.cpp \
    ../src/Model/AudioEngine/SSoundMix/ssoundmix.cpp \
    ../src/View/AboutQtWindow/aboutqtwindow.cpp \
    ../src/View/AboutWindow/aboutwindow.cpp \
    ../src/View/FXWindow/fxwindow.cpp \
    ../src/View/SearchWindow/searchwindow.cpp \
    ../src/View/TrackList/tracklist.cpp \
    ../src/View/TrackWidget/trackwidget.cpp \
    ../src/main.cpp \
    ../src/View/MainWindow/mainwindow.cpp

HEADERS += \
    ../ext/qcustomplot/qcustomplot.h \
    ../src/Controller/controller.h \
    ../src/Model/AudioCore/audiocore.h \
    ../src/Model/AudioEngine/SAudioEngine/saudioengine.h \
    ../src/Model/AudioEngine/SSound/ssound.h \
    ../src/Model/AudioEngine/SSoundMix/ssoundmix.h \
    ../src/Model/globals.h \
    ../src/View/AboutQtWindow/aboutqtwindow.h \
    ../src/View/AboutWindow/aboutwindow.h \
    ../src/View/FXWindow/fxwindow.h \
    ../src/View/MainWindow/mainwindow.h \
    ../src/View/SearchWindow/searchwindow.h \
    ../src/View/TrackList/tracklist.h \
    ../src/View/TrackWidget/trackwidget.h

FORMS += \
    ../src/View/AboutQtWindow/aboutqtwindow.ui \
    ../src/View/AboutWindow/aboutwindow.ui \
    ../src/View/FXWindow/fxwindow.ui \
    ../src/View/MainWindow/mainwindow.ui \
    ../src/View/SearchWindow/searchwindow.ui \
    ../src/View/TrackList/tracklist.ui \
    ../src/View/TrackWidget/trackwidget.ui

INCLUDEPATH += "../src"
INCLUDEPATH += "../src/Model"

win32
{
    RC_FILE += ../res/rec_file.rc
    OTHER_FILES += ../res/rec_file.rc
}

RESOURCES += ../res/qt_rec_file.qrc

QMAKE_CXXFLAGS *= /std:c++17

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
