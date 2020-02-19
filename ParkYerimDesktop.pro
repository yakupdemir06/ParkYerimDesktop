QT       += core gui sql

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

INCLUDEPATH += $$PWD/include \
                $$PWD/opencv-build/include \


LIBS += -L$$PWD/opencv-build/x64/mingw/lib \
        -lopencv_calib3d420     \
        -lopencv_core420        \
        -lopencv_dnn420         \
        -lopencv_features2d420  \
        -lopencv_flann420       \
        -lopencv_gapi420        \
        -lopencv_highgui420     \
        -lopencv_imgcodecs420   \
        -lopencv_imgproc420     \
        -lopencv_ml420          \
        -lopencv_objdetect420   \
        -lopencv_photo420       \
        -lopencv_stitching420   \
        -lopencv_video420       \
        -lopencv_videoio420



HEADERS += \
    $$PWD/include/*.hpp


SOURCES += \
    $$PWD/src/*.cpp


FORMS += \
    $$PWD/forms/*.ui



RC_ICONS = ParkYerimDesktop.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


RESOURCES += \
    Assets.qrc


