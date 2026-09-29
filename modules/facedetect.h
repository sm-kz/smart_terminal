#ifndef FACEDETECT_H
#define FACEDETECT_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLayout>
#include <QComboBox>
#include <opencv4/opencv2/objdetect.hpp>
#include <opencv4/opencv2/core.hpp>
#include <opencv4/opencv2/imgproc.hpp>

#include "hardware/camera.h"
#include "pageEvent.h"

class FaceDetectWorker: public QObject
{
    Q_OBJECT
public slots:
    // 检测图像，进行转灰度图，对象检测等
    void detect(const cv::Mat &img);

signals:
    // 检测完成信号，发送检测得到的检测框
    void detected(const std::vector<cv::Rect> &faces);

public:
    cv::CascadeClassifier faceCascade;
};


class FaceDetect: public QWidget
{
    Q_OBJECT
public:
    FaceDetect(QWidget *parent = nullptr);
    ~FaceDetect();

private:
    // 显示标签
    QLabel *displayLabel;

    // 照片标签
    QLabel *photoLabel;

    // 摄像头设备选项框
    QComboBox *comboBox;

    // 拍照、预览按钮
    QPushButton *captureButton;
    QPushButton *previewButton;

    // 人脸识别开启按钮
    QPushButton *faceDetectButton;

    // 摄像头
    Camera *camera;

    // 当前帧图像
    QImage currentFrame;

    // 侧边栏控件
    QWidget *sideWidget;

    // 布局
    QHBoxLayout *hMainLayout;
    QVBoxLayout *vSideLayout;

    void layoutInit();
    void scanCameraDevice();
    void faceDetectWorkerInit();

    // 检测工人对象
    FaceDetectWorker *worker;
    QThread *thread;

    // 检测记录
    bool detecting;
    std::vector<cv::Rect> lastRects;

protected:
    bool event(QEvent *event);

signals:
    void detectImage(const cv::Mat &img);

private slots:
    void showImage(const QImage&);
    void showFaceDetect(const QImage&);
    void updateFaces(const std::vector<cv::Rect> &faces);
    void previewButtonClicked(bool checked);
    void saveImageToLocal(bool);
    void faceHaarClicked(bool checked);
};

#endif // FACEDETECT_H
