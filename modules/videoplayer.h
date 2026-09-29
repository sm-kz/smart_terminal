#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QWidget>
#include <QLayout>
#include <QVideoWidget>
#include <QListWidget>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QMediaPlayer>
#include <QMediaPlaylist>

class VideoPlayer: public QWidget {
    Q_OBJECT

public:
    VideoPlayer(QWidget *parent = nullptr);
    ~VideoPlayer();

private:
    // 显示控件
    QWidget *displayWidget;

    // 视频播放控件
    QVideoWidget *videoWidget;

    // 侧边栏控件
    QWidget *sideWidget;

    // 播放列表
    QListWidget *listWidget;

    // 底部控件
    QWidget *bottomWidget;

    // 进度条控件
    QSlider *durationSlider;

    // 进度显示
    QLabel *currentTimeLabel;
    QLabel *endTimeLabel;

    // 控制栏
    QWidget *controlWidget;

    // 控制按键
    QPushButton *playButton;
    QPushButton *nextButton;
    QPushButton *volumeDownButton;
    QPushButton *volumeUpButton;
    QPushButton *fullButton;

    // 音量条
    QSlider *volumeSlider;

    // 主界面布局
    QVBoxLayout *vMainLayout;

    // 显示界面布局
    QHBoxLayout *hDisplayLayout;

    // 侧边栏布局
    QVBoxLayout *vSideLayout;

    // 底部布局
    QVBoxLayout *vBottomLayout;

    // 控制按键布局
    QHBoxLayout *hControlLayout;

    // 媒体播放器
    QMediaPlayer *mediaPlayer;

    // 播放列表
    QMediaPlaylist *mediaPlaylist;

    // 针对全屏播放时的控件和布局
    QWidget *fullScreenWidget;
    QVBoxLayout *fullScreenLayout;

    void videoLayout();
    void mediaInit();
    QSize sizeHint();

private slots:
    void playButtonClicked(bool checked);
    void nextButtonClicked();
    void volumeDownButtonClicked();
    void volumeUpButtonClicked();
    void fullButtonClicked(bool checked);
    void durationSliderUpdate(qint64 duration);
    void changeMediaIndex(int index);
    // void playlistIndexChanged(int index);
    void durationSliderMoved(int value);
    void durationSliderReleased();
};

#endif // VIDEOPLAYER_H
