#include "musicPlayer.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTime>
#include <QCoreApplication>

MusicPlayer::MusicPlayer(QWidget *parent):
    QWidget(parent),
    sliderMoving(false),
    started(false)
{
    // 如果要渲染控件本身的背景，必须加上这一行
    setAttribute(Qt::WA_StyledBackground, true);

    layoutInit();

    mediaInit();

    // 列表
    connect(listWidget, &QListWidget::currentRowChanged,
            this, &MusicPlayer::setCurrentMedia);

    // 按键与播放列表
    connect(playButton, &QPushButton::clicked,
            this, &MusicPlayer::playButtonClicked);
    connect(prevButton, &QPushButton::clicked,
            this, &MusicPlayer::prevButtonClicked);
    connect(nextButton, &QPushButton::clicked,
            this, &MusicPlayer::nextButtonClicked);
    connect(playlist, &QMediaPlaylist::currentIndexChanged,
            this, &MusicPlayer::playlistUpdated);

    // 进度条
    connect(player, &QMediaPlayer::positionChanged,
            this, &MusicPlayer::updateDuration);
    connect(durationSlider, &QSlider::sliderPressed,
            this, [this]() { sliderMoving = true; });
    connect(durationSlider, &QSlider::sliderReleased,
            this, [this]() {
        sliderMoving = false;
        // 未播放的视频还未加载，不能设置进度
        if (!started) {
            durationSlider->setValue(0);
            return;
        }

        player->setPosition(durationSlider->value());
    });
}

MusicPlayer::~MusicPlayer() {

}

void MusicPlayer::layoutInit() {
    // 按键
    prevButton = new QPushButton();
    nextButton = new QPushButton();
    playButton = new QPushButton();
    favorButton = new QPushButton();
    modeButton = new QPushButton();
    listButton = new QPushButton();
    volumeButton = new QPushButton();
    welcomeLabel = new QLabel("Enjoy the music!");
    coverLabel = new QLabel();
    currentTime = new QLabel("00:00");
    endTime = new QLabel("00:00");
    durationSlider = new QSlider(Qt::Horizontal);
    listWidget = new QListWidget();
    leftWidget = new QWidget();
    rightWidget = new QWidget();
    selectWidget = new QWidget();
    durationIndicator = new QWidget();
    functionWidget = new QWidget();

    playButton->setCheckable(true);
    favorButton->setCheckable(true);
    prevButton->setObjectName("musicplayer-prev-button");
    nextButton->setObjectName("musicplayer-next-button");
    playButton->setObjectName("musicplayer-play-button");
    favorButton->setObjectName("musicplayer-favor-button");
    modeButton->setObjectName("musicplayer-mode-button");
    listButton->setObjectName("musicplayer-list-button");
    volumeButton->setObjectName("musicplayer-volume-button");
    welcomeLabel->setObjectName("musicplayer-welcome-label");
    durationSlider->setObjectName("musicplayer-duration-slider");
    listWidget->setObjectName("musicplayer-listwidget");

    // 播放进度条
    durationSlider->setMinimumWidth(300);
    durationSlider->setMaximumHeight(20);

    // 音乐列表
    listWidget->resize(310, 265);
    listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 主界面布局
    hMainLayout = new QHBoxLayout();
    QSpacerItem *hspacer0 = new QSpacerItem(10, 0,
                                            QSizePolicy::Minimum, QSizePolicy::Minimum);
    QSpacerItem *hspacer1 = new QSpacerItem(10, 0,
                                            QSizePolicy::Minimum, QSizePolicy::Minimum);
    QSpacerItem *hspacer2 = new QSpacerItem(10, 0,
                                            QSizePolicy::Minimum, QSizePolicy::Minimum);
    hMainLayout->addSpacerItem(hspacer0);
    hMainLayout->addWidget(leftWidget);
    hMainLayout->addSpacerItem(hspacer1);
    hMainLayout->addWidget(rightWidget);
    hMainLayout->addSpacerItem(hspacer2);
    hMainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(hMainLayout);

    // 左边栏布局
    vLeftLayout = new QVBoxLayout();
    listWidget->setMinimumSize(270, 250);
    selectWidget->setMinimumSize(270, 80);
    selectWidget->setMaximumHeight(80);
    QSpacerItem *vspacer0 = new QSpacerItem(270, 10,
                                            QSizePolicy::Minimum, QSizePolicy::Maximum);
    QSpacerItem *vspacer1 = new QSpacerItem(270, 30,
                                            QSizePolicy::Minimum, QSizePolicy::Minimum);
    vLeftLayout->addWidget(welcomeLabel);
    vLeftLayout->addWidget(listWidget);
    vLeftLayout->addSpacerItem(vspacer0);
    vLeftLayout->addWidget(selectWidget);
    vLeftLayout->addSpacerItem(vspacer1);
    vLeftLayout->setContentsMargins(0, 0, 0, 0);
    leftWidget->setLayout(vLeftLayout);

    // 选择按钮布局
    hSelectLayout = new QHBoxLayout();
    prevButton->setFixedSize(80, 80);
    nextButton->setFixedSize(80, 80);
    playButton->setFixedSize(80, 80);
    QSpacerItem *hspacer3 = new QSpacerItem(10, 80,
                                            QSizePolicy::Expanding, QSizePolicy::Expanding);
    QSpacerItem *hspacer4 = new QSpacerItem(10, 80,
                                            QSizePolicy::Expanding, QSizePolicy::Expanding);
    hSelectLayout->addWidget(prevButton);
    hSelectLayout->addSpacerItem(hspacer3);
    hSelectLayout->addWidget(playButton);
    hSelectLayout->addSpacerItem(hspacer4);
    hSelectLayout->addWidget(nextButton);
    hSelectLayout->setContentsMargins(0, 0, 0, 0);
    selectWidget->setLayout(hSelectLayout);

    // 右边栏布局
    vRightLayout = new QVBoxLayout();
    coverLabel->setMinimumSize(320, 320);
    QSpacerItem *vspacer2 = new QSpacerItem(320, 40,
                                           QSizePolicy::Minimum, QSizePolicy::Maximum);
    QSpacerItem *vspacer3 = new QSpacerItem(320, 20,
                                           QSizePolicy::Minimum, QSizePolicy::Maximum);
    QSpacerItem *vspacer4 = new QSpacerItem(320, 30,
                                           QSizePolicy::Minimum, QSizePolicy::Minimum);
    QImage cover(":/musicplayer/image/cd.png");
    cover.scaled(320, 320,
                 Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    coverLabel->setPixmap(QPixmap::fromImage(cover));
    functionWidget->setMinimumSize(320, 80);
    functionWidget->setMaximumHeight(80);
    vRightLayout->addSpacerItem(vspacer2);
    vRightLayout->addWidget(coverLabel);
    vRightLayout->addSpacerItem(vspacer3);
    vRightLayout->addWidget(functionWidget);
    vRightLayout->addWidget(durationSlider);
    vRightLayout->addWidget(durationIndicator);
    vRightLayout->addSpacerItem(vspacer4);
    vRightLayout->setContentsMargins(0, 0, 0, 0);
    rightWidget->setLayout(vRightLayout);

    // 进度指示器布局
    hDurationIndicatorLayout = new QHBoxLayout();
    currentTime->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    endTime->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    currentTime->setAlignment(Qt::AlignLeft);
    endTime->setAlignment(Qt::AlignRight);
    hDurationIndicatorLayout->addWidget(currentTime);
    hDurationIndicatorLayout->addWidget(endTime);
    hDurationIndicatorLayout->setContentsMargins(0, 0, 0, 0);
    durationIndicator->setLayout(hDurationIndicatorLayout);

    // 功能按键布局
    hFunctionLayout = new QHBoxLayout();
    QSpacerItem *hspace5 = new QSpacerItem(0, 60,
                                           QSizePolicy::Minimum, QSizePolicy::Maximum);
    QSpacerItem *hspace6 = new QSpacerItem(60, 60,
                                           QSizePolicy::Minimum, QSizePolicy::Maximum);
    QSpacerItem *hspace7 = new QSpacerItem(60, 60,
                                           QSizePolicy::Minimum, QSizePolicy::Maximum);
    QSpacerItem *hspace8 = new QSpacerItem(60, 60,
                                           QSizePolicy::Minimum, QSizePolicy::Maximum);
    QSpacerItem *hspace9 = new QSpacerItem(0, 60,
                                           QSizePolicy::Minimum, QSizePolicy::Maximum);
    favorButton->setFixedSize(25, 25);
    modeButton->setFixedSize(25, 25);
    listButton->setFixedSize(25, 25);
    volumeButton->setFixedSize(25, 25);
    hFunctionLayout->addSpacerItem(hspace5);
    hFunctionLayout->addWidget(favorButton);
    hFunctionLayout->addSpacerItem(hspace6);
    hFunctionLayout->addWidget(modeButton);
    hFunctionLayout->addSpacerItem(hspace7);
    hFunctionLayout->addWidget(listButton);
    hFunctionLayout->addSpacerItem(hspace8);
    hFunctionLayout->addWidget(volumeButton);
    hFunctionLayout->addSpacerItem(hspace9);
    hFunctionLayout->setContentsMargins(0, 0, 0 ,0);
    functionWidget->setLayout(hFunctionLayout);
}

void MusicPlayer::mediaInit() {
    player = new QMediaPlayer(this);
    playlist = new QMediaPlaylist();
    player->setPlaylist(playlist);

    // 默认顺序循环播放
    playlist->setPlaybackMode(QMediaPlaylist::Loop);

    // 获取指定默认路径的音频文件
    QDir dir(QCoreApplication::applicationDirPath() + "/music");
    QStringList filter;
    filter << "*.mp3" << "*.wav";
    dir.setNameFilters(filter);

    const QStringList files = dir.entryList(QDir::Files, QDir::Name);
    for (const auto& file : files) {
        QString path = dir.absoluteFilePath(file);

        playlist->addMedia(QMediaContent(
            QUrl::fromLocalFile(path)
            ));
    }
    listWidget->addItems(files);

    // 视频变更后重新获取总时长
    connect(player, &QMediaPlayer::durationChanged,
            this, [this](qint64 duration) {
                durationSlider->setRange(0, player->duration());
                durationSlider->setValue(0);
                endTime->setText(
                    QTime(0, 0, 0).addMSecs(duration).toString("mm:ss")
                    );
            });

    if (playlist->mediaCount() > 0) {
        playlist->setCurrentIndex(0);
        listWidget->setCurrentRow(0);
    }
}

void MusicPlayer::setCurrentMedia(int index) {
    playlist->setCurrentIndex(index);
    if (playButton->isChecked())
        started = true;
    else
        started = false;
}

void MusicPlayer::playButtonClicked(bool checked) {
    if (checked) {
        started = true;
        player->play();
    }
    else
        player->pause();
}

void MusicPlayer::prevButtonClicked() {
    playlist->previous();
    if (playButton->isChecked())
        started = true;
    else
        started = false;
}

void MusicPlayer::nextButtonClicked() {
    playlist->next();
    if (playButton->isChecked())
        started = true;
    else
        started = false;
}

void MusicPlayer::playlistUpdated() {
    int index = playlist->currentIndex();
    QSignalBlocker blocker(listWidget);
    listWidget->setCurrentRow(index);
}

void MusicPlayer::updateDuration(qint64 pos) {
    if (sliderMoving)
        return;

    currentTime->setText(
        QTime(0, 0, 0).addMSecs(pos).toString("mm:ss"));
    durationSlider->setValue(pos);
}