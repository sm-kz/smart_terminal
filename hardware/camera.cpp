#include "camera.h"
#include "opencv4/opencv2/core/core.hpp"
#include "opencv4/opencv2/highgui/highgui.hpp"
#include <QDebug>

Camera::Camera(QObject *parent): QObject(parent) {
    capture = new cv::VideoCapture();
    timer = new QTimer(this);

    cameraIndex = 0;

    connect(timer, &QTimer::timeout, this, &Camera::timerTimeout);
}

Camera::~Camera() {
    capture->release();
    delete capture;
    capture = nullptr;
}

void Camera::selectCameraDevice(int index) {
    if (capture->isOpened())
        capture->release();

    cameraIndex = index;
}

int Camera::openCameraDevice() {
    if (capture->open(cameraIndex) == false) {
        qDebug() << "[Camera] Failed to Open camera" << cameraIndex;
        return -1;
    }
    capture->set(cv::VideoCaptureProperties::CAP_PROP_FOURCC,
                 cv::VideoWriter::fourcc('R', 'G', 'B', 'P'));
    capture->set(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH, 800);
    capture->set(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT, 480);
    capture->set(cv::VideoCaptureProperties::CAP_PROP_FPS, 60);
    capture->set(cv::VideoCaptureProperties::CAP_PROP_BUFFERSIZE, 3);

    return 0;
}

void Camera::releaseCameraDevice() {
    capture->release();
}

bool Camera::cameraProcess(bool checked) {
    if (checked) {
        if (openCameraDevice() < 0)
            return false;
        // 30帧，每帧周期为 1000 / 30 = 33ms
        timer->start(33);
    } else {
        releaseCameraDevice();
        timer->stop();
    }

    return capture->isOpened();
}

void Camera::timerTimeout() {
    if (!capture->isOpened()) {
        timer->stop();
        return;
    }

    static cv::Mat frame;
    *capture >> frame;
    if (frame.cols)
        emit readyImage(matToQImage(frame));
}

QImage Camera::matToQImage(const cv::Mat &img) {
    // opencv默认把帧数据当作RGB888数据，需要手动转换为QImage的RGB565，然后再转为RGB888
    if (img.type() == CV_8UC3) {
        const uchar *pimg = (const uchar*)img.data;

        QImage qimg(pimg, img.cols, img.rows, img.cols * 2, QImage::Format_RGB16);
        qimg.convertTo(QImage::Format_RGB888);
        return qimg;
    }

    return QImage();
}


