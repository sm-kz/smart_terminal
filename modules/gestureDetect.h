#ifndef GESTUREDETECT_H
#define GESTUREDETECT_H

#include "hardware/gesture.h"
#include "compass.h"
#include "proximityRadar.h"
#include <QWidget>
#include <QLayout>
#include <QPushButton>
#include <QTimer>
#include <QQueue>
#include <QStackedLayout>

class GestureDetect: public QWidget
{
    Q_OBJECT
public:
    GestureDetect(QWidget *parent = nullptr);

private:
    Gesture *gesture;
    QTimer *gTimer;
    QQueue<gesture::GestureDirection> gestureDirs;

    // 控制按钮
    QPushButton *gestureButton;
    QPushButton *alsButton;
    QPushButton *proximityButton;

    QWidget *displayWidget;
    QWidget *controlWidget;

    // 指针轮盘
    Compass *compass;

    // 靠近雷达
    ProximityRadar *proximityRadar;

    // 主布局
    QHBoxLayout *hMainLayout;

    // 显示布局
    QStackedLayout *displayLayout;

    // 控制栏布局
    QGridLayout *gControlLayout;

    void layoutInit();
    void changeMode(gesture::Mode mode);

private slots:
    void gestureButtonClicked(bool checked);
    void gestureSave(gesture::GestureDirection);
    void gestureProcess();
    void proximityButtonClicked(bool checked);
    void proximityProcess(gesture::ProximityData);
    void alsButtonClicked(bool checked);
    void alsProcess(gesture::ALSData);
};

#endif // GESTUREDETECT_H
