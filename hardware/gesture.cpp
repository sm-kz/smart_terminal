/*
 * 本程序实现的是手势、靠近、环境光分开检测
 * 驱动是完全可以实现三者同时检测的
 */

#include "hardware/gesture.h"
#include "linux/input.h"
#include <QDebug>
#include <linux/errno.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>

using namespace gesture;

Gesture::Gesture(QThread *parent):
    QThread(parent),
    mode(Mode::ModeNone)
{

}

inline int Gesture::changeMode(const Mode mode) {
    if (this->mode == mode)
        return 0;

    lock.lock();

    if (operatingFile.isOpen()) {
        operatingFile.close();
    }

    switch (mode) {
    case ModeALS:
    case ModeProximity:
        operatingFile.setFileName(GESTURE_DEV_PATH);
        break;
    case ModeGesture:
        operatingFile.setFileName(GESTURE_INPUT_PATH);
        break;
    case ModeNone:
    default:
        this->mode = ModeNone;
        lock.unlock();
        return -1;
        break;
    }

    this->mode = mode;
    if (!operatingFile.exists()) {
        qDebug() << "[Gesture] File doesn't exist:" << operatingFile.fileName();
        this->mode = ModeNone;
        lock.unlock();
        return -1;
    }

    operatingFile.open(QIODevice::ReadOnly);
    // QFile 打开文件默认以阻塞模式，有必要的话手动设置非阻塞模式
    int flags = fcntl(operatingFile.handle(), F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(operatingFile.handle(), F_SETFL, flags);
    lock.unlock();

    return 0;
}

// gesture驱动采用的 input 子系统，直接读会一直阻塞
int Gesture::gestureProcess() {
    int ret, n;
    struct input_event events[8];    // 最多两个方向，每个方向4条上报事件
    struct pollfd pfd;
    pfd.fd = operatingFile.handle();
    pfd.events = POLLIN;

    lock.lock();

    ret = poll(&pfd, 1, 300);

    if (ret == 0) {
        lock.unlock();
        return 0;
    }
    else if (ret < 0)
        goto fail;

    if (!(pfd.revents & POLLIN))
        goto fail;

    n = operatingFile.read((char *)events, sizeof(events));
    if (n < 0)
        goto fail;
    n /= sizeof(events[0]);

    for (int i = 0; i < n; ++i) {
        if (events[i].type == EV_KEY && events[i].value == 1) {
            switch(events[i].code) {
            case KEY_UP:
                emit gestureDetected(GestureUp);
                break;
            case KEY_DOWN:
                emit gestureDetected(GestureDown);
                break;
            case KEY_LEFT:
                emit gestureDetected(GestureLeft);
                break;
            case KEY_RIGHT:
                emit gestureDetected(GestureRight);
                break;
            default:
                break;
            }
        }
    }

    lock.unlock();
    return 0;
fail:
    lock.unlock();
    changeMode(ModeNone);
    return -1;
}

// ALS/Proximity 驱动采用 ioctl 条件阻塞
int Gesture::alsProcess() {
    struct ALSData als;
    int ret;

    // 上锁避免在等待过程中切换模式
    lock.lock();
    ret = ioctl(operatingFile.handle(), IOCTL_GET_ALS, &als);
    lock.unlock();

    if (ret < 0) {
        if (errno == ETIMEDOUT)
            return 0;

        qDebug() << "[Gesture] Getting ALS data failed with" << ret << strerror(errno);
        return -1;
    }

    emit alsDetected(als);
    return 0;
}

int Gesture::proximityProcess() {
    struct ProximityData proximity;
    int ret;

    // 上锁避免在等待过程中切换模式
    lock.lock();
    ret = ioctl(operatingFile.handle(), IOCTL_GET_PROXIMITY, &proximity);
    lock.unlock();

    if (ret < 0) {
        if (errno == ETIMEDOUT)
            return 0;

        qDebug() << "[Gesture] Getting proximity data failed with" << errno << strerror(errno);
        return -1;
    }

    emit proximityDetected(proximity);
    return 0;
}

void Gesture::startCalibration() {
    int ret;

    ret = ioctl(operatingFile.handle(), IOCTL_CALIBRATE);
    if (ret < 0) {
        qDebug() << "[Gesture] Calibration failed with" << ret << strerror(errno);
        return;
    }

    emit calibrationDone();
}

void Gesture::run() {
    int ret;

    while(!isInterruptionRequested()) {
        switch (mode) {
        case ModeGesture:
            ret = gestureProcess();
            break;
        case ModeALS:
            ret = alsProcess();
            break;
        case ModeProximity:
            ret = proximityProcess();
            break;
        default:
            msleep(100);
            continue;
            break;
        }

        if (ret < 0)
            changeMode(ModeNone);

        // 不要一直抢占 CPU，因为互斥锁不会先到先得
        // 如果一直运行几乎全流程上所的任务，其他线程就会抢不到锁
        // 造成长时间的线程饥饿
        // 睡眠时间也不宜太长，因为此目的仅仅是为了让出锁给等待线程
        // 程序中已有阻塞 IO 进行充足的等待
        msleep(1);
    }

    if (operatingFile.isOpen())
        operatingFile.close();
}