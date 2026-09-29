#ifndef BANNA_H
#define BANNA_H

#include <QWidget>
#include <QTimer>
#include <QButtonGroup>
#include <QPushButton>
#include <QLabel>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QLayout>

namespace Ui {
class Banna;
} // namespace Ui

namespace BannaSpace {
constexpr int TIMER_INTERVAL = 3000;
const QByteArray ANIMATION_GEOMERTY = "geometry";
constexpr int ANIMATION_DURATION = 500;
const QEasingCurve ANIMATION_TYPE = QEasingCurve::BezierSpline;

const QString imagePaths[] = {
    ":/banna/image/ren.jpg",
    ":/banna/image/zelda-800x480.png",
    ":/banna/image/ren.jpg",
    ":/banna/image/zelda-800x480.png",
    ":/banna/image/ren.jpg",
};

struct BannaItem {
    QPushButton *pushButton;
    QLabel *currentImageLabel;
    QLabel *nextImageLabel;
    QLabel *prevImageLabel;
    QPropertyAnimation *propAnimation;
};
} // namespace BannaSpace

class Banna: public QWidget {
    Q_OBJECT

public:
    explicit Banna(QWidget *parent = nullptr);
    ~Banna();

    void updateAnimationDuration(int duration);

private:
    // 顶层图层索引
    int topIndex;

    // 手动切换banna变量
    bool manualChanging;
    int targetIndex;

    // 图片轮换定时器
    QTimer *changeImageTimer;

    // 按键组
    QButtonGroup buttonGroup;
    QPushButton *pushButtons[5];

    // 标签
    QLabel *labels[5];

    // 数据列表
    QList<BannaSpace::BannaItem> bannaList;

    // 动画列表
    QParallelAnimationGroup animationGroup;

    void layoutInit();
    void valueInit();
    QSize getLabelSize(int index);
    int getDiff(int base, int x, int N);
    void setNextAnimation();
    void setPrevAnimation();
    void sortGeometry(const bool &isNextFlag, const bool &isPrevFlag);

private slots:
    void changeImage();
    void indexedButtonClicked(int);
    void manualChangingProcess();
    void updateGeometry();

protected:
    void resizeEvent(QResizeEvent *event);
};

#endif // BANNA_H
