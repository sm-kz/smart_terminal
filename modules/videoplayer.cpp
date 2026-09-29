#include "videoplayer.h"

#include <QPalette>
#include <QFont>
#include <QDebug>
#include <QDir>
#include <QTime>
#include <QCoreApplication>

VideoPlayer::VideoPlayer(QWidget *parent):
    QWidget(parent)
{
    // 布局初始化
    videoLayout();

    // 媒体初始化
    mediaInit();

    // 初始化信号槽连接
    connect(playButton, &QPushButton::toggled,
            this, &VideoPlayer::playButtonClicked);
    connect(nextButton, &QPushButton::clicked,
            this, &VideoPlayer::nextButtonClicked);
    connect(volumeDownButton, &QPushButton::clicked,
            this, &VideoPlayer::volumeDownButtonClicked);
    connect(volumeUpButton, &QPushButton::clicked,
            this, &VideoPlayer::volumeUpButtonClicked);
    connect(fullButton, &QPushButton::toggled,
            this, &VideoPlayer::fullButtonClicked);

    // 播放列表
    connect(listWidget, SIGNAL(currentRowChanged(int)),
            this, SLOT(changeMediaIndex(int)));
    // connect(mediaPlaylist, &QMediaPlaylist::currentIndexChanged,
    //         this, &VideoPlayer::playlistIndexChanged);

    // 进度条
    connect(mediaPlayer, SIGNAL(positionChanged(qint64)),
            this, SLOT(durationSliderUpdate(qint64)));
    connect(durationSlider, &QSlider::sliderMoved,
            this, &VideoPlayer::durationSliderMoved);
    connect(durationSlider, &QSlider::sliderReleased,
            this, &VideoPlayer::durationSliderReleased);
    // 播放列表切换后，mediaPlayer不会立刻更新总时长，需要监听durationChanged事件
    connect(mediaPlayer, &QMediaPlayer::durationChanged,
            this, [this](qint64 duration){
                durationSlider->setRange(0, duration);
                // 因为QTime的初始化时间有进位，不能直接将原始ms传入来初始化
                this->endTimeLabel->setText("/" +
                    QTime(0, 0, 0).QTime::addMSecs(duration).toString("mm:ss"));
            });

    // 音量条
    connect(volumeSlider, &QSlider::sliderMoved,
            mediaPlayer, &QMediaPlayer::setVolume);

    durationSlider->installEventFilter(this);
}

VideoPlayer::~VideoPlayer() {}

void VideoPlayer::videoLayout() {
    // 控件初始化
    displayWidget = new QWidget();
    videoWidget = new QVideoWidget();
    sideWidget = new QWidget();
    listWidget = new QListWidget();
    bottomWidget = new QWidget();
    durationSlider = new QSlider(Qt::Horizontal);
    currentTimeLabel = new QLabel();
    endTimeLabel = new QLabel();
    controlWidget = new QWidget();
    playButton = new QPushButton();
    nextButton = new QPushButton();
    volumeDownButton = new QPushButton();
    volumeUpButton = new QPushButton();
    fullButton = new QPushButton();
    volumeSlider = new QSlider(Qt::Horizontal);
    fullScreenWidget = new QWidget();

    setObjectName("videoplayer");
    displayWidget->setObjectName("videoplayer-display-widget");
    videoWidget->setObjectName("videoplayer-video-widget");
    sideWidget->setObjectName("videoplayer-side-widget");
    listWidget->setObjectName("videoplayer-list-widget");
    durationSlider->setObjectName("videoplayer-duration-slider");
    controlWidget->setObjectName("videoplayer-ctrl-widget");
    playButton->setObjectName("videoplayer-play-button");
    nextButton->setObjectName("videoplayer-next-button");
    volumeDownButton->setObjectName("videoplayer-volumedown-button");
    volumeUpButton->setObjectName("videoplayer-volumeup-button");
    fullButton->setObjectName("videoplayer-full-button");
    volumeSlider->setObjectName("videoplayer-volume-slider");
    fullScreenWidget->setObjectName("videoplayer-full-widget");

    QFont font;
    font.setPixelSize(20);
    QPalette pal;
    pal.setColor(QPalette::WindowText, Qt::white);
    currentTimeLabel->setFont(font);
    currentTimeLabel->setPalette(pal);
    currentTimeLabel->setText("00:00");
    endTimeLabel->setFont(font);
    endTimeLabel->setPalette(pal);
    endTimeLabel->setText("/00:00");

    playButton->setCheckable(true);
    playButton->setChecked(false);
    fullButton->setCheckable(true);
    fullButton->setChecked(false);

    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(50);

    listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 主界面布局
    vMainLayout = new QVBoxLayout();
    vMainLayout->addWidget(displayWidget);
    vMainLayout->addWidget(bottomWidget);
    vMainLayout->setContentsMargins(0, 0, 0, 0);
    vMainLayout->setSpacing(0);
    this->setLayout(vMainLayout);

    // 显示界面
    hDisplayLayout = new QHBoxLayout();
    hDisplayLayout->addWidget(videoWidget);
    hDisplayLayout->addWidget(sideWidget);
    videoWidget->setMinimumSize(400, 300);
    hDisplayLayout->setContentsMargins(0, 0, 0, 0);
    hDisplayLayout->setSpacing(0);
    displayWidget->setLayout(hDisplayLayout);

    // 侧边栏布局
    vSideLayout = new QVBoxLayout();
    vSideLayout->addWidget(listWidget);
    vSideLayout->setContentsMargins(0, 0, 0, 0);
    sideWidget->setLayout(vSideLayout);

    // 底部布局
    durationSlider->setMaximumHeight(10);
    controlWidget->setMaximumHeight(80);
    vBottomLayout = new QVBoxLayout();
    vBottomLayout->addWidget(durationSlider);
    vBottomLayout->addWidget(controlWidget);
    vBottomLayout->setAlignment(Qt::AlignCenter);
    bottomWidget->setLayout(vBottomLayout);

    // 控制控件布局
    hControlLayout = new QHBoxLayout();
    volumeSlider->setMaximumWidth(80);

    QSpacerItem *hSpacer0 = new QSpacerItem(200, 80,
        QSizePolicy::Expanding, QSizePolicy::Maximum);

    hControlLayout->addSpacing(20);
    hControlLayout->addWidget(playButton);
    hControlLayout->addSpacing(10);
    hControlLayout->addWidget(nextButton);
    hControlLayout->addSpacing(10);
    hControlLayout->addWidget(volumeDownButton);
    hControlLayout->addWidget(volumeSlider);
    hControlLayout->addWidget(volumeUpButton);
    hControlLayout->addWidget(currentTimeLabel);
    hControlLayout->addWidget(endTimeLabel);
    hControlLayout->addSpacerItem(hSpacer0);
    hControlLayout->addWidget(fullButton);
    hControlLayout->addSpacing(20);
    hControlLayout->setContentsMargins(0, 0, 0, 0);
    hControlLayout->setAlignment(Qt::AlignLeft);

    controlWidget->setLayout(hControlLayout);

    // 全屏控件布局
    fullScreenLayout = new QVBoxLayout();
    fullScreenLayout->setContentsMargins(0, 0, 0, 0);
    fullScreenLayout->setSpacing(0);
    fullScreenWidget->setLayout(fullScreenLayout);
}

void VideoPlayer::mediaInit() {
    mediaPlayer = new QMediaPlayer(this);
    mediaPlaylist = new QMediaPlaylist(this);
    // 设置播放列表
    mediaPlayer->setPlaylist(mediaPlaylist);
    // 媒体输出绑定到视频播放控件
    mediaPlayer->setVideoOutput(videoWidget);
    // 设置为循环播放
    mediaPlaylist->setPlaybackMode(QMediaPlaylist::Loop);
    // 设置播放器默认参数
    mediaPlayer->setVolume(50);

    // 搜索视频资源文件
    QDir dir(QCoreApplication::applicationDirPath() + "/video");
    QStringList filter;
    filter << "*.mp4" << "*.mkv" << "*.avi";
    dir.setNameFilters(filter);

    QStringList files = dir.entryList(QDir::Files);

    for (const auto &file : files) {
        // Url制定qrc文件必须指明"qrc"这个类型
        mediaPlaylist->addMedia(QMediaContent(
            QUrl("file:" + dir.absoluteFilePath(file))
            ));
        listWidget->addItem(file);
    }

    if (files.size() > 0)
        listWidget->setCurrentRow(0);
}

QSize VideoPlayer::sizeHint() {
    return QSize(450, 400);
}

void VideoPlayer::playButtonClicked(bool checked) {
    if (mediaPlaylist->isEmpty())
        return;

    if (!mediaPlayer->isAvailable())
        qDebug() << "[MediaPlayer] Current media is not available";

    if (checked)
        mediaPlayer->play();
    else
        mediaPlayer->pause();
}

void VideoPlayer::nextButtonClicked() {
    int nextIndex = mediaPlaylist->nextIndex();
    listWidget->setCurrentRow(nextIndex);
}

void VideoPlayer::volumeDownButtonClicked() {
    int val = volumeSlider->value();
    val -= 10;
    if (val < volumeSlider->minimum())
        val = volumeSlider->minimum();

    volumeSlider->setValue(val);
    mediaPlayer->setVolume(val);
}

void VideoPlayer::volumeUpButtonClicked() {
    int val = volumeSlider->value();
    val += 10;
    if (val > volumeSlider->maximum())
        val = volumeSlider->maximum();

    volumeSlider->setValue(val);
    mediaPlayer->setVolume(val);
}

void VideoPlayer::fullButtonClicked(bool checked) {
    if (checked) {
        fullScreenLayout->addWidget(videoWidget, 1);
        fullScreenLayout->addWidget(bottomWidget, 0);

        hide();
        fullScreenWidget->showFullScreen();

    } else {
        hDisplayLayout->insertWidget(0, videoWidget);
        vMainLayout->addWidget(bottomWidget);

        fullScreenWidget->hide();
        show();
    }
}

void VideoPlayer::durationSliderUpdate(qint64 position) {
    // 正在拖动slider时，不允许更新视频播放导致slider位置更新
    if (!durationSlider->isSliderDown()) {
        durationSlider->setValue(position);
        durationSlider->setValue(position);
        currentTimeLabel->setText(QTime(0, 0, 0).addMSecs(position).toString("mm:ss"));
    }
}

void VideoPlayer::changeMediaIndex(int index) {
    mediaPlaylist->setCurrentIndex(index);
    playButton->setChecked(false);

    // 更新进度条，总时长在durationChanged事件发生后更改
    durationSlider->setValue(0);
    currentTimeLabel->setText("00:00");
}

// void VideoPlayer::playlistIndexChanged(int index) {
//     // 屏蔽listWidget接收的信号，避免循环传递
//     QSignalBlocker blocker(listWidget);

//     listWidget->setCurrentRow(index);
//     durationSlider->setValue(0);
//     currentTimeLabel->setText("00:00");
// }

void VideoPlayer::durationSliderMoved(int value) {
    // 移动时只更新时间显示
    currentTimeLabel->setText(QTime(0, 0, 0).addMSecs(value).toString("mm:ss"));
}

void VideoPlayer::durationSliderReleased() {
    // 释放才更新视频进度
    mediaPlayer->setPosition(durationSlider->value());
}