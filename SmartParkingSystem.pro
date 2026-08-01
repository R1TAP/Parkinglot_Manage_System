QT += core gui widgets sql

CONFIG += c++11

TARGET = SmartParkingSystem
TEMPLATE = app

INCLUDEPATH += src
INCLUDEPATH += EasyPRLib/include

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    src/main.cpp \
    src/gui/logindialog.cpp \
    src/gui/mainwindow.cpp \
    src/gui/registerdialog.cpp \
    src/gui/vehicledialog.cpp \
    src/core/datamanager.cpp \
    src/gui/userdialog.cpp \
    src/gui/bindvehicledialog.cpp \
    src/gui/changepassworddialog.cpp \
    src/gui/monthpassdialog.cpp \
    # EasyPR sources
    EasyPRLib/src/core/chars_identify.cpp \
    EasyPRLib/src/core/chars_recognise.cpp \
    EasyPRLib/src/core/chars_segment.cpp \
    EasyPRLib/src/core/core_func.cpp \
    EasyPRLib/src/core/feature.cpp \
    EasyPRLib/src/core/params.cpp \
    EasyPRLib/src/core/plate_detect.cpp \
    EasyPRLib/src/core/plate_judge.cpp \
    EasyPRLib/src/core/plate_locate.cpp \
    EasyPRLib/src/core/plate_recognize.cpp \
    EasyPRLib/src/util/kv.cpp \
    EasyPRLib/src/util/util.cpp \
    EasyPRLib/thirdparty/LBP/helper.cpp \
    EasyPRLib/thirdparty/LBP/lbp.cpp \
    EasyPRLib/thirdparty/mser/mser2.cpp \
    EasyPRLib/thirdparty/svm/corrected_svm.cpp \
    EasyPRLib/thirdparty/textDetect/erfilter.cpp \
    EasyPRLib/thirdparty/xmlParser/xmlParser.cpp

HEADERS += \
    src/gui/logindialog.h \
    src/gui/mainwindow.h \
    src/gui/registerdialog.h \
    src/gui/vehicledialog.h \
    src/core/datamanager.h \
    src/core/plateutil.h \
    src/core/user.h \
    src/core/vehicle.h \
    src/gui/userdialog.h \
    src/gui/bindvehicledialog.h \
    src/gui/changepassworddialog.h \
    src/gui/monthpassdialog.h

FORMS += \
    forms/logindialog.ui \
    forms/mainwindow.ui \
    forms/registerdialog.ui \
    forms/vehicledialog.ui \
    forms/userdialog.ui \
    forms/bindvehicledialog.ui \
    forms/changepassworddialog.ui \
    forms/monthpassdialog.ui

RESOURCES += resources.qrc

ICON = resources/appicon.ico

# OpenCV 3.4.5（MinGW 构建）
# 通过环境变量 OPENCV_DIR 指定本机 OpenCV 构建目录；
# 未设置时回退到项目历史路径（本机仍可用的默认值）。
isEmpty(OPENCV_DIR): OPENCV_DIR = C:/0Datas/Program/Zero/SmartParkingSystem/opencv-3.4.5/build
INCLUDEPATH += $$OPENCV_DIR/install/include
LIBS += -L$$OPENCV_DIR/lib \
    -lopencv_core345 \
    -lopencv_highgui345 \
    -lopencv_imgcodecs345 \
    -lopencv_imgproc345 \
    -lopencv_features2d345 \
    -lopencv_ml345 \
    -lopencv_objdetect345

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
