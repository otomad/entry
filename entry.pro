#-------------------------------------------------
#
# Project created by QtCreator 2021-04-11T15:47:31
#
#-------------------------------------------------

QT += \
    core gui \
    sql \
    printsupport \
    serialport \
    network \
    webkitwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets \
					widgets printsupport

TARGET = entry
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += \
	c++11
#   console
#	release

#Win32:CONFIG(debug, debug|release) {
#    TARGET = debug_binary
#} else {
#    TARGET = release_binary
#}

SOURCES += \
	audioanalysis.cpp \
    chatmessage/qnchatmessage.cpp \
    main.cpp \
	loginwindow.cpp \
	nonlineardistortion.cpp \
	oscilloscope.cpp \
    serialportchat.cpp \
	socketlistmanager.cpp \
    sqlconnect.cpp \
    mainwindow.cpp \
    qcustomplot.cpp \
    serversetting.cpp \
    serialportassistant.cpp \
    signalgenerator.cpp \
    singlechip.cpp \
    mainwindow_plot.cpp

HEADERS += \
	audioanalysis.h \
    chatmessage/qnchatmessage.h \
	loginwindow.h \
	nonlineardistortion.h \
	oscilloscope.h \
	qhex.h \
    serialportchat.h \
	socketlistmanager.h \
    sqlconnect.h \
    mainwindow.h \
    qcustomplot.h \
    serversetting.h \
    smoothCurveGenerator.h \
    serialportassistant.h \
    common.h \
    signalgenerator.h \
    singlechip.h

FORMS += \
	audioanalysis.ui \
    loginwindow.ui \
	mainwindow.ui \
	nonlineardistortion.ui \
	oscilloscope.ui \
    serialportchat.ui \
    serversetting.ui \
    serialportassistant.ui \
    signalgenerator.ui \
    singlechip.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    QssPreview/qsspreview.qrc \
    chatmessage/chatmessage.qrc \
    led/led.qrc \
    qdarkstyle/darkstyle.qrc \
    qlightstyle/lightstyle.qrc \
    res.qrc \
    qmqrc/qm.qrc

OTHER_FILES += \
    application.rc

RC_FILE += \
    application.rc

OBJECTS_DIR = tmp
MOC_DIR = tmp

#LIBS += libgdi32

#win32:LIBS += Qt6SerialPort.dll

#DISTFILES += \
#	NonlinearDistortion.qml \
#	NonlinearDistortionWidget.ui.qml
