#include "facedetect.h"
#include <QSpacerItem>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QThread>
#include <QCoreApplication>

using namespace cv;

FaceDetect::FaceDetect(QWidget *parent):
    QWidget(parent),
    detecting(false)
{
    layoutInit();

    scanCameraDevice();

    // 另外创建一个线程进行耗时的检测处理
    faceDetectWorkerInit();

    connect(camera, &Camera::readyImage,
            this, &FaceDetect::showImage);
    connect(previewButton, &QPushButton::clicked,
            this, &FaceDetect::previewButtonClicked);
    connect(captureButton, &QPushButton::clicked,
            this, &FaceDetect::saveImageToLocal);
    connect(faceDetectButton, &QPushButton::clicked,
            this, &FaceDetect::faceHaarClicked);
    connect(comboBox, SIGNAL(currentIndexChanged(int)),
            camera, SLOT(selectCameraDevice(int)));
}

FaceDetect::~FaceDetect() {
    if (thread) {
        thread->quit();   // 请求工作线程的事件循环退出
        thread->wait();   // 当前线程等待工作线程真正结束
    }
}

void FaceDetect::layoutInit() {
    // 控件初始化
    displayLabel = new QLabel();
    photoLabel = new QLabel();
    comboBox = new QComboBox();
    captureButton = new QPushButton("Capture");
    previewButton = new QPushButton("Preview");
    faceDetectButton = new QPushButton("Face Detect");
    sideWidget = new QWidget();
    camera = new Camera(this);

    displayLabel->setObjectName("face-display");
    photoLabel->setObjectName("face-photo");
    sideWidget->setObjectName("face-side");
    comboBox->setObjectName("camera-select");
    captureButton->setObjectName("capture-button");
    previewButton->setObjectName("preview-button");
    faceDetectButton->setObjectName("detect-button");

    displayLabel->setScaledContents(true);
    displayLabel->setMinimumSize(400, 300);
    displayLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    photoLabel->setScaledContents(true);
    photoLabel->setFixedSize(150, 150);
    displayLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    captureButton->setMaximumSize(120, 60);
    previewButton->setMaximumSize(120, 60);
    faceDetectButton->setMaximumSize(120, 60);
    previewButton->setCheckable(true);
    captureButton->setEnabled(false);
    previewButton->setEnabled(true);
    faceDetectButton->setCheckable(true);

    // 布局初始化
    hMainLayout = new QHBoxLayout();
    hMainLayout->addWidget(displayLabel);
    hMainLayout->addWidget(sideWidget);
    this->setLayout(hMainLayout);

    vSideLayout = new QVBoxLayout();
    vSideLayout->addWidget(photoLabel);
    vSideLayout->addWidget(faceDetectButton);
    QSpacerItem *spacer1 = new QSpacerItem(200, 50,
        QSizePolicy::Maximum, QSizePolicy::Expanding);
    vSideLayout->addSpacerItem(spacer1);
    vSideLayout->addWidget(comboBox);
    QSpacerItem *spacer2 = new QSpacerItem(200, 50,
        QSizePolicy::Maximum, QSizePolicy::Expanding);
    vSideLayout->addSpacerItem(spacer2);
    vSideLayout->addWidget(captureButton);
    vSideLayout->addWidget(previewButton);
    vSideLayout->setAlignment(Qt::AlignCenter);
    sideWidget->setLayout(vSideLayout);
}

void FaceDetect::faceDetectWorkerInit() {
    worker = new FaceDetectWorker;
    // 使用opencv源码提供的级联分类器模型文件
    // 可以理解为模型训练后得到的一系列特征、阈值、权重
    QString file = ":/haarcascades/haarcascade_frontalface_alt2.xml";
    QString temp = "/tmp/haarcascade_frontalface_alt2.xml";

    if (!QFile::exists(temp)) {
        QFile::copy(file, temp);
    }

    worker->faceCascade.load(temp.toStdString());

    thread = new QThread(this);

    // 非Qt的标准数据类型，作为信号或槽参数之前需要注册
    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<std::vector<cv::Rect>>("std::vector<cv::Rect>");
    connect(thread, &QThread::finished,
            worker, &QObject::deleteLater);
    connect(this, &FaceDetect::detectImage,
            worker, &FaceDetectWorker::detect);
    connect(worker, &FaceDetectWorker::detected,
            this, &FaceDetect::updateFaces);

    worker->moveToThread(thread);
    thread->start();
}

void FaceDetect::showImage(const QImage &img) {
    displayLabel->setPixmap(QPixmap::fromImage(img));

    currentFrame = img.copy();
    captureButton->setEnabled(true);
}

void FaceDetect::previewButtonClicked(bool checked) {
    if (camera->cameraProcess(checked) == false)
        return;

    if (checked) {
        previewButton->setText("close");
    } else {
        captureButton->setEnabled(false);
        previewButton->setText("preview");
    }
}

void FaceDetect::scanCameraDevice() {
    QFile file("/dev/video0");

    // 一般为pxp设备
    if (file.exists())
        comboBox->addItem("video0");

    // 一般是用户注册的摄像头
    file.setFileName("/dev/video1");
    if (file.exists())
        comboBox->addItem("video1");

    file.setFileName("/dev/video2");
    if (file.exists())
        comboBox->addItem("video2");

    comboBox->setCurrentIndex(1);
    camera->selectCameraDevice(comboBox->currentIndex());
}

void FaceDetect::saveImageToLocal(bool) {
    if (!currentFrame.isNull()) {
        QString fileName =
            QCoreApplication::applicationDirPath() +
            QString("/capture_%1").arg(QDateTime::currentMSecsSinceEpoch() / 1000);
        qDebug() << "[FaceDetect] Saving " << fileName << " ...";

        currentFrame.save(fileName, "PNG", -1);
        photoLabel->setPixmap(QPixmap::fromImage(QImage(fileName)));
        qDebug() << "[FaceDetect] save succeed";
    }
}

void FaceDetect::faceHaarClicked(bool checked) {
    if(checked) {
        disconnect(camera, &Camera::readyImage,
                   this, &FaceDetect::showImage);
        connect(camera, &Camera::readyImage,
                this, &FaceDetect::showFaceDetect);
    } else {
        disconnect(camera, &Camera::readyImage,
                   this, &FaceDetect::showFaceDetect);
        connect(camera, &Camera::readyImage,
                this, &FaceDetect::showImage);
    }

    lastRects.clear();
}

void FaceDetect::updateFaces(const std::vector<Rect> &faces) {
    lastRects = faces;
    detecting = false;
}

void FaceDetect::showFaceDetect(const QImage &qImg) {
    QImage qShow = qImg.copy();
    Mat img(
        qShow.height(),
        qShow.width(),
        CV_8UC3,
        qShow.bits(),
        qShow.bytesPerLine()
        );

    std::vector<Rect> faces;
    if (!detecting) {
        detecting = true;
        // 跨线程必须保证像素数据独立，避免提前释放
        emit detectImage(img.clone());
    }

    // 检测到人脸数大于0
    if (lastRects.size() > 0){
        for (int i = 0; i < static_cast<int>(lastRects.size()); ++i) {
            // 在图像上显示检测框
            rectangle(img, Point(lastRects[i].x, lastRects[i].y),
                      Point(lastRects[i].x + lastRects[i].width, lastRects[i].y + lastRects[i].height),
                      Scalar(0, 255, 0), 5, LINE_8);
        }
    }

    displayLabel->setPixmap(QPixmap::fromImage(qShow));
}

void FaceDetectWorker::detect(const Mat &img) {
    Mat imgGray;
    imgGray.create(img.rows, img.cols, CV_8UC1);
    cvtColor(img, imgGray, COLOR_RGB2GRAY);

    // 开始目标检测
    std::vector<Rect> faces;
    faceCascade.detectMultiScale(imgGray, faces,
                                 1.2, 6, 0,
                                 Size(0, 0));
    emit detected(faces);
}

bool FaceDetect::event(QEvent *event) {
    if (event->type() == PageEvent::leaveType()) {
        camera->cameraProcess(false);
        captureButton->setEnabled(false);
        previewButton->setChecked(false);
        previewButton->setText("preview");
    }

    return QWidget::event(event);
}