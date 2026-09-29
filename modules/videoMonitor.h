#ifndef VIDEOMONITOR_H
#define VIDEOMONITOR_H

#include <QWidget>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QThread>
#include <QTimer>

#include "pageEvent.h"

#define VIDEO_DEV           "/dev/video1"
#define VIDEO_BUF_COUNT     3
constexpr int VIDEO_WIDTH  = 800;
constexpr int VIDEO_HEIGHT = 480;

class VideoMonitorWorker: public QObject
{
    Q_OBJECT
public:
    VideoMonitorWorker(QObject *parent = nullptr);

signals:
    void imageReady(QImage img);

public slots:
    void bufferProcess();
};

struct VideoBuffer {
    void *start;
    int len;
};

class VideoMonitor: public QWidget
{
    Q_OBJECT
public:
    VideoMonitor(QWidget *parent = nullptr);
    ~VideoMonitor();

    // 视频设备文件描述符
    int videoFd;

    // 视频帧缓冲
    VideoBuffer buffers[VIDEO_BUF_COUNT];

    bool isLocalPlayed();
    bool isBroadcastPlayed();
private:
    // 界面控件
    QLabel *videoLabel;
    QCheckBox *localCheckBox;
    QCheckBox *broadcastCheckBox;
    QPushButton *startButton;

    // 视频帧处理线程
    VideoMonitorWorker *worker;
    QThread *thread;

    // 定时检测帧
    QTimer *timer;

    void layoutInit();
    int cameraInit();
    void cameraDeinit();
    void videoMonitorWorkerInit();
    int streamStart();
    void streamStop();

signals:
    void bufferReady();

private slots:
    void startButtonClicked(bool checked);
    void checkBuffer();

public slots:
    void showImage(QImage img);

protected:
    bool event(QEvent *event);
    void resizeEvent(QResizeEvent *e);
};

#endif // VIDEOMONITOR_H
