#include "videoMonitor.h"

#include <QDebug>
#include <QUdpSocket>
#include <QBuffer>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>


VideoMonitorWorker::VideoMonitorWorker(QObject *parent):
    QObject(parent)
{

}

VideoMonitor::VideoMonitor(QWidget *parent):
    QWidget(parent),
    videoFd(-1)
{
    layoutInit();

    videoMonitorWorkerInit();

    connect(startButton, &QPushButton::clicked,
            this, &VideoMonitor::startButtonClicked);
}

VideoMonitor::~VideoMonitor(){
    if (thread) {
        thread->quit();   // 请求工作线程的事件循环退出
        thread->wait();   // 当前线程等待工作线程真正结束
    }

    if (videoFd < 0)
        return;

    for (int i = 0; i < VIDEO_BUF_COUNT; ++i) {
        munmap(buffers[i].start, buffers[i].len);
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(videoFd, VIDIOC_STREAMOFF, &type);

    ::close(videoFd);
}

void VideoMonitor::layoutInit() {
    videoLabel = new QLabel("No Data", this);
    localCheckBox = new QCheckBox("Local", this);
    broadcastCheckBox = new QCheckBox("Broadcast", this);
    startButton = new QPushButton("Start", this);

    videoLabel->setObjectName("video-display");
    localCheckBox->setObjectName("local-checkbox");
    broadcastCheckBox->setObjectName("broadcast-checkbox");
    startButton->setObjectName("start-button");

    videoLabel->setAlignment(Qt::AlignCenter);

    localCheckBox->setFixedSize(80, 50);
    broadcastCheckBox->setFixedSize(80, 50);

    startButton->setCheckable(true);
    startButton->setFixedSize(140, 80);

    setMinimumSize(400, 300);
}

// C标准库，V4L2操作摄像头
int VideoMonitor::cameraInit() {
    videoFd = open(VIDEO_DEV, O_RDWR);
    if (videoFd < 0) {
        qDebug() << "[VideoMonitor] Error: No such video device: "
                 << VIDEO_DEV;
        return -1;
    }

    // 设置数据格式
    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = VIDEO_WIDTH;
    fmt.fmt.pix.height = VIDEO_HEIGHT;
    fmt.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
    if (ioctl(videoFd, VIDIOC_S_FMT, &fmt) < 0) {
        qDebug() << "[VideoMonitor] Failed: VIDIOC_S_FMT";
        ::close(videoFd);
        return -1;
    }

    // 帧缓冲申请
    struct v4l2_requestbuffers reqbufs;
    reqbufs.count = VIDEO_BUF_COUNT;
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    if (ioctl(videoFd, VIDIOC_REQBUFS, &reqbufs) < 0) {
        qDebug() << "[VideoMonitor] Failed: VIDIOC_REQBUFS";
        ::close(videoFd);
        return -1;
    }

    // 内存映射
    struct v4l2_buffer buf;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    for (int i = 0; i < VIDEO_BUF_COUNT; ++i) {
        buf.index = i;
        if (ioctl(videoFd, VIDIOC_QUERYBUF, &buf)) {
            qDebug() << "[VideoMonitor] Failed: VIDIOC_QUERYBUF";
            ::close(videoFd);
            return -1;
        }

        buffers[i].len = buf.length;
        buffers[i].start = mmap(NULL, buf.length,
                                PROT_READ | PROT_WRITE, MAP_SHARED,
                                videoFd, buf.m.offset);
        if (buffers[i].start == MAP_FAILED) {
            qDebug() << "[VideoMonitor] Error: mmap to "
                     << buffers[i].start
                     << ", with length of "
                     << buffers[i].len;
            ::close(videoFd);
            return -1;
        }
    }

    // 设置定时器定时检测视频帧是否就绪，以30帧为标准
    timer = new QTimer(this);
    timer->setInterval(33);

    connect(timer, &QTimer::timeout,
            this, &VideoMonitor::checkBuffer);

    return 0;
}

void VideoMonitor::cameraDeinit() {
    if (videoFd < 0)
        return;

    for (int i = 0; i < VIDEO_BUF_COUNT; ++i) {
        munmap(buffers[i].start, buffers[i].len);
        buffers[i].start = nullptr;
        buffers[i].len = 0;
    }

    ::close(videoFd);
    videoFd = -1;

    if (timer) {
        delete timer;
        timer = nullptr;
    }
}

void VideoMonitor::videoMonitorWorkerInit() {
    worker = new VideoMonitorWorker;
    thread = new QThread(this);

    connect(thread, &QThread::finished,
            worker, &QObject::deleteLater);
    connect(this, &VideoMonitor::bufferReady,
            worker, &VideoMonitorWorker::bufferProcess);
    connect(worker, &VideoMonitorWorker::imageReady,
            this, &VideoMonitor::showImage);
    worker->moveToThread(thread);
    thread->start();
}

inline bool VideoMonitor::isLocalPlayed() {
    return localCheckBox->isChecked();
}

inline bool VideoMonitor::isBroadcastPlayed() {
    return broadcastCheckBox->isChecked();
}

bool VideoMonitor::event(QEvent *event) {
    if (event->type() == PageEvent::leaveType()) {
        streamStop();
        startButton->setChecked(false);
        startButton->setText("Start");

        if (videoFd > 0) {
            cameraDeinit();
        }

        return true;
    }
    return QWidget::event(event);
}

void VideoMonitor::resizeEvent(QResizeEvent *e) {
    Q_UNUSED(e);
    int _width = width();
    int _height = height();

    videoLabel->setGeometry(30, 30,
                            _width - 60, _height - 60);
    startButton->move((_width - startButton->width()) / 2,
                      _height - startButton->height() - 10);
    localCheckBox->move(_width - localCheckBox->width() - 10,
                        0.5 * _height - localCheckBox->height() - 40);
    broadcastCheckBox->move(_width - broadcastCheckBox->width() - 10,
                        0.5 * _height - broadcastCheckBox->height() + 40);
}

int VideoMonitor::streamStart() {
    struct v4l2_buffer buf = {};

    for (int i = 0; i < VIDEO_BUF_COUNT; ++i) {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(videoFd, VIDIOC_QBUF, &buf) < 0) {
            qDebug() << "[VideoMonitor] Failed: VIDIOC_QBUF";
            ::close(videoFd);
            return -1;
        }
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(videoFd, VIDIOC_STREAMON, &type) < 0) {
        qDebug() << "[VideoMonitor] Failed: VIDIOC_STREAMON";
        return -1;
    }

    timer->start();
    return 0;
}

void VideoMonitor::streamStop() {
    if (videoFd < 0)
        return;

    timer->stop();
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(videoFd, VIDIOC_STREAMOFF, &type) < 0)
        qDebug() << "[VideoMonitor] Failed: VIDIOC_STREAMOFF";
}

void VideoMonitor::startButtonClicked(bool checked) {
    if (checked) {
        if (videoFd == -1) {
            if (cameraInit() < 0)
                return;
        }

        if (streamStart() < 0) {
            cameraDeinit();
        }
        startButton->setText("Stop");
    }
    else {
        streamStop();
        startButton->setText("Start");
    }
}

void VideoMonitor::showImage(QImage img) {
    videoLabel->setPixmap(QPixmap::fromImage(img));
}

void VideoMonitor::checkBuffer() {
    struct pollfd pfd = {videoFd, POLLIN, 0};

    if (poll(&pfd, 1, 0) < 0)
        return;

    if (pfd.revents & POLLIN)
        emit bufferReady();
}

void VideoMonitorWorker::bufferProcess() {
    VideoMonitor *vm = static_cast<VideoMonitor *>(sender());

    struct v4l2_buffer buf;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    // 出队
    if (ioctl(vm->videoFd, VIDIOC_DQBUF, &buf)) {
        qDebug() << "[VideoMonitor] Failed: VIDIOC_DQBUF";
        return;
    }

    int index = buf.index;

    QImage img(static_cast<unsigned char*>(vm->buffers[index].start),
               VIDEO_WIDTH, VIDEO_HEIGHT, QImage::Format_RGB16);

    if (vm->isLocalPlayed())
        emit imageReady(img);

    if (vm->isBroadcastPlayed()) {
        QUdpSocket udpSocket;
        QByteArray byteArr;
        // 创建用于IO读写的缓冲区
        QBuffer imgbuf(&byteArr);
        img.save(&imgbuf, "JPEG", -1);

        QByteArray byteArr64 = byteArr.toBase64();
        // 以广播发送，端口号为8888
        udpSocket.writeDatagram(byteArr64.data(), byteArr64.size(),
                                QHostAddress::Broadcast, 8888);
    }

    // 入队
    if (ioctl(vm->videoFd, VIDIOC_QBUF, &buf)) {
        qDebug() << "[VideoMonitor] Failed: VIDIOC_DQBUF";
        return;
    }
}