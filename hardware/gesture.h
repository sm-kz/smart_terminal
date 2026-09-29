#ifndef GESTURE_H
#define GESTURE_H

#include <QThread>
#include <QString>
#include <sys/ioctl.h>
#include <QFile>
#include <QMutex>

namespace gesture {

const QString GESTURE_DEV_PATH = "/dev/apds9960";
const QString GESTURE_INPUT_PATH = "/dev/input/event3";

struct ALSData {
    unsigned short c;
    unsigned short r;
    unsigned short g;
    unsigned short b;
};

struct ProximityData {
    unsigned char dist;
};

#define IOCTL_GET_ALS           _IOR('A', 0, struct ALSData)
#define IOCTL_GET_PROXIMITY     _IOR('A', 1, struct ProximityData)
#define IOCTL_CALIBRATE         _IO('A', 2)

enum GestureDirection {
    GestureUp = 0,
    GestureDown,
    GestureLeft,
    GestureRight
};

enum Mode {
    ModeNone,
    ModeGesture,
    ModeALS,
    ModeProximity
};

}   // namespace gesture


class Gesture: public QThread {
    Q_OBJECT
public:
    Gesture(QThread *parent = nullptr);
    int changeMode(const gesture::Mode mode);
    void startCalibration();

protected:
    virtual void run() final;

private:
    int gestureProcess();
    int alsProcess();
    int proximityProcess();

    gesture::Mode mode;
    bool state;
    QFile operatingFile;
    QMutex lock;

signals:
    void gestureDetected(gesture::GestureDirection);
    void alsDetected(gesture::ALSData);
    void proximityDetected(gesture::ProximityData);
    void calibrationDone();
};

#endif // GESTURE_H
