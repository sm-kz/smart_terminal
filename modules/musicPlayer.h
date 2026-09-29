#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H
#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QListWidget>
#include <QLayout>
#include <QLabel>
#include <QMediaPlayer>
#include <QMediaPlaylist>

class MusicPlayer: public QWidget
{
    Q_OBJECT
public:
    MusicPlayer(QWidget *parent = nullptr);
    ~MusicPlayer();

private:
    // 内部维护状态
    bool sliderMoving;
    bool started;

    // 按钮
    QPushButton *prevButton;
    QPushButton *nextButton;
    QPushButton *playButton;
    QPushButton *favorButton;
    QPushButton *modeButton;
    QPushButton *listButton;
    QPushButton *volumeButton;

    // 标签
    QLabel *welcomeLabel;
    QLabel *coverLabel;
    QLabel *currentTime;
    QLabel *endTime;

    // 进度条
    QSlider *durationSlider;

    // 播放列表
    QListWidget *listWidget;

    // 布局
    QHBoxLayout *hMainLayout;
    QVBoxLayout *vLeftLayout;
    QVBoxLayout *vRightLayout;
    QHBoxLayout *hSelectLayout;
    QHBoxLayout *hDurationIndicatorLayout;
    QHBoxLayout *hFunctionLayout;

    QWidget *leftWidget;
    QWidget *rightWidget;
    QWidget *selectWidget;
    QWidget *durationIndicator;
    QWidget *functionWidget;

    // 媒体
    QMediaPlayer *player;
    QMediaPlaylist *playlist;

    void layoutInit();
    void mediaInit();

private slots:
    void setCurrentMedia(int index);
    void playButtonClicked(bool checked);
    void prevButtonClicked();
    void nextButtonClicked();
    void playlistUpdated();
    void updateDuration(qint64 pos);
};

#endif // MUSICPLAYER_H
