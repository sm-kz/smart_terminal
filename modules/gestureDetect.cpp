#include "gestureDetect.h"
#include <QDebug>

// TODO: 传感器校准功能

using namespace gesture;

GestureDetect::GestureDetect(QWidget *parent):
    QWidget(parent),
    gestureDirs()
{
    layoutInit();

    gesture = new Gesture();
    gTimer = new QTimer(this);

    qRegisterMetaType<gesture::GestureDirection>("gesture::GestureDirection");
    qRegisterMetaType<gesture::ALSData>("gesture::ALSData");
    qRegisterMetaType<gesture::ProximityData>("gesture::ProximityData");

    // 手势
    connect(gestureButton, &QPushButton::clicked,
            this, &GestureDetect::gestureButtonClicked);
    connect(gesture, &Gesture::gestureDetected,
            this, &GestureDetect::gestureSave);
    connect(gTimer, &QTimer::timeout,
            this, &GestureDetect::gestureProcess);

    // 靠近
    connect(proximityButton, &QPushButton::clicked,
            this, &GestureDetect::proximityButtonClicked);
    connect(gesture, &Gesture::proximityDetected,
            this, &GestureDetect::proximityProcess);

    // 环境光
    connect(alsButton, &QPushButton::clicked,
            this, &GestureDetect::alsButtonClicked);
    connect(gesture, &Gesture::alsDetected,
            this, &GestureDetect::alsProcess);
}

void GestureDetect::layoutInit() {
    gestureButton = new QPushButton("Gesture");
    alsButton = new QPushButton("Ambidient");
    proximityButton = new QPushButton("Proximity");
    compass = new Compass();
    proximityRadar = new ProximityRadar();
    displayWidget = new QWidget();
    controlWidget = new QWidget();

    displayWidget->setObjectName("gesturedetect-display-widget");
    controlWidget->setObjectName("gesturedetect-control-widget");

    gestureButton->setCheckable(true);
    alsButton->setCheckable(true);
    proximityButton->setCheckable(true);

    // 主布局
    hMainLayout = new QHBoxLayout();
    hMainLayout->addWidget(displayWidget);
    hMainLayout->addWidget(controlWidget);
    hMainLayout->setAlignment(Qt::AlignCenter);
    hMainLayout->setSpacing(0);
    hMainLayout->setContentsMargins(0, 0, 0, 0);
    controlWidget->setMaximumWidth(140);
    controlWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
    compass->setMinimumWidth(300);
    compass->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    proximityRadar->setMinimumWidth(300);
    proximityRadar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setLayout(hMainLayout);

    // 显示布局
    displayLayout = new QStackedLayout();
    displayLayout->addWidget(compass);
    displayLayout->addWidget(proximityRadar);
    displayLayout->setCurrentIndex(1);
    displayLayout->setContentsMargins(0, 0, 0, 0);
    displayWidget->setLayout(displayLayout);

    // 控制栏布局
    gControlLayout = new QGridLayout();
    gControlLayout->addWidget(proximityButton, 0, 0, 1, 1);
    gControlLayout->addWidget(gestureButton, 1, 0, 1, 1);
    gControlLayout->addWidget(alsButton, 2, 0, 1, 1);
    proximityButton->setMaximumSize(120, 90);
    gestureButton->setMaximumSize(120, 90);
    alsButton->setMaximumSize(120, 90);
    gControlLayout->setSpacing(0);
    gControlLayout->setContentsMargins(0, 0, 0, 0);
    controlWidget->setLayout(gControlLayout);
}

void GestureDetect::changeMode(Mode mode) {
    switch (mode) {
    case ModeGesture:
        displayLayout->setCurrentIndex(0);
        proximityRadar->updateDist(0);
        compass->update();
        break;
    case ModeNone:
    case ModeALS:
    case ModeProximity:
        displayLayout->setCurrentIndex(1);
        compass->rotateTo(0);
        proximityRadar->update();
        break;
    default:
        return;
    }
    gesture->changeMode(mode);
}

void GestureDetect::gestureButtonClicked(bool checked) {
    if (!checked) {
        proximityButton->setEnabled(true);
        alsButton->setEnabled(true);
        changeMode(ModeNone);
        gesture->requestInterruption();
    } else {
        proximityButton->setEnabled(false);
        alsButton->setEnabled(false);
        changeMode(ModeGesture);
        gesture->start();
    }
}

void GestureDetect::gestureSave(GestureDirection dir) {
    // 可能会连续上传两个方向表示一个方位，100ms内保存两个方向
    if (gestureDirs.isEmpty()) {
        gTimer->setSingleShot(true);
        gTimer->start(100);
    }
    gestureDirs.push_back(dir);
}

int directionToDegree(GestureDirection dir) {
    switch(dir) {
    case GestureUp:
        return 90;
    case GestureDown:
        return 270;
    case GestureLeft:
        return 180;
    case GestureRight:
        return 0;
    default:
        return 0;
    }
}

void GestureDetect::gestureProcess() {
    GestureDirection dir1, dir2;
    int deg1 = 0, deg2 = 0;

    dir1 = gestureDirs.front();
    gestureDirs.pop_front();
    deg1 = directionToDegree(dir1);

    if (gestureDirs.empty()) {
        compass->rotateTo(deg1);
        return;
    }

    dir2 = gestureDirs.front();
    gestureDirs.pop_front();
    gestureDirs.clear();
    deg2 = directionToDegree(dir2);

    int diff = qAbs(deg1 - deg2);
    int target;
    if (diff == 0 || diff == 180)
        return;

    if (diff > 180)
        target = (deg1 + deg2 + 360) / 2;
    else
        target = (deg1 + deg2) / 2;

    compass->rotateTo(target);
}

void GestureDetect::proximityButtonClicked(bool checked) {
    if (!checked) {
        gestureButton->setEnabled(true);
        alsButton->setEnabled(true);
        changeMode(ModeNone);
        gesture->requestInterruption();
    } else {
        gestureButton->setEnabled(false);
        alsButton->setEnabled(false);
        changeMode(ModeProximity);
        gesture->start();
    }
}

void GestureDetect::proximityProcess(ProximityData proximity) {
    proximityRadar->updateDist(proximity.dist);
}

void GestureDetect::alsButtonClicked(bool checked) {
    if (!checked) {
        gestureButton->setEnabled(true);
        proximityButton->setEnabled(true);
        changeMode(ModeNone);
        gesture->requestInterruption();
    } else {
        gestureButton->setEnabled(false);
        proximityButton->setEnabled(false);
        changeMode(ModeALS);
        gesture->start();
    }
}

void GestureDetect::alsProcess(ALSData als) {
    int maxRGB = qMax(als.r, qMax(als.g, als.b));

    if (maxRGB <= 0)
        return;

    // RGB只决定色调
    int r = als.r * 255 / maxRGB;
    int g = als.g * 255 / maxRGB;
    int b = als.b * 255 / maxRGB;

    // C决定深浅
    const qreal lightMax = 5000.0;
    qreal light = qMin(als.c / lightMax, 1.0);

    // 光越强，越接近白色
    qreal white = light * 0.7;

    r += (255 - r) * white;
    g += (255 - g) * white;
    b += (255 - b) * white;

    proximityRadar->setBackgroundColor(QColor(r, g, b));
}