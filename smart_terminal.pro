QT += widgets sql multimediawidgets network

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    hardware/camera.cpp \
    hardware/gesture.cpp \
    modules/banna.cpp \
    modules/compass.cpp \
    modules/facedetect.cpp \
    modules/gestureDetect.cpp \
    modules/musicPlayer.cpp \
    modules/numberPicker.cpp \
    modules/opencv.cpp \
    modules/proximityRadar.cpp \
    modules/splash.cpp \
    modules/sqlClock.cpp \
    modules/switchButton.cpp \
    modules/videoMonitor.cpp \
    modules/videoplayer.cpp \
    main.cpp \
    mainWidget.cpp

HEADERS += \
    hardware/camera.h \
    hardware/gesture.h \
    modules/banna.h \
    modules/compass.h \
    modules/facedetect.h \
    modules/gestureDetect.h \
    modules/musicPlayer.h \
    modules/numberPicker.h \
    modules/opencv.h \
    modules/pageEvent.h \
    modules/proximityRadar.h \
    modules/splash.h \
    modules/sqlClock.h \
    modules/switchButton.h \
    modules/videoMonitor.h \
    modules/videoplayer.h \
    mainWidget.h

MODULES += \
    mainWidget.ui

INCLUDEPATH += $${_PRO_FILE_PWD_}/modules \
                /usr/local/include \
                /usr/local/include/opencv4

LIBS += -L/usr/local/lib/opencv \
        -lopencv_core \
        -lopencv_highgui \
        -lopencv_imgproc \
        -lopencv_videoio \
        -lopencv_imgcodecs \
        -lopencv_objdetect \
        -lopencv_dnn \
        -lopencv_calib3d \
        -lopencv_features2d \
        -lopencv_flann

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    rsc/banna/banna.qrc \
    rsc/musicplayer/musicplayer.qrc \
    rsc/opencv/opencv.qrc \
    rsc/videoplayer/videoplayer.qrc \
    style/style.qrc

DISTFILES +=
