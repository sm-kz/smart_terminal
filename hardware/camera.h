#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>
#include <QImage>
#include <QTimer>

namespace cv {
class VideoCapture;
class Mat;
} // namespace cv

class Camera: public QObject {
    Q_OBJECT
public :
    explicit Camera(QObject *parent = nullptr);
    ~Camera();

    cv::VideoCapture *capture;
signals:
    void readyImage(QImage);

public slots:
    bool cameraProcess(bool);
    void selectCameraDevice(int);

private slots:
    void timerTimeout();

private:
    QTimer *timer;
    int cameraIndex;

    int openCameraDevice();
    void releaseCameraDevice();
    QImage matToQImage(const cv::Mat&);
};



#endif // CAMERA_H
