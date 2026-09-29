#include "banna.h"
#include <QScreen>
#include <QDebug>
#include <QtMath>

using namespace BannaSpace;

Banna::Banna(QWidget *parent):
    QWidget(parent),
    manualChanging(false),
    targetIndex(0)
{
    layoutInit();
    valueInit();
}

Banna::~Banna() {

}

void Banna::layoutInit() {
    this->setMinimumSize(300, 300);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 按键、标签初始化
    int buttonCount = sizeof(pushButtons) / sizeof(pushButtons[0]);
    topIndex = buttonCount / 2;

    for (int i = 0; i < buttonCount; ++i) {
        pushButtons[i] = new QPushButton(this);
        pushButtons[i]->setFixedSize(30, 30);
        pushButtons[i]->setCheckable(true);
        buttonGroup.addButton(pushButtons[i], i);

        labels[i] = new QLabel(this);
        labels[i]->setAlignment(Qt::AlignCenter);
        labels[i]->setScaledContents(true);

        QSize size = getLabelSize(i);
        if (i < int(sizeof(imagePaths) / sizeof(imagePaths[0]))) {
            QImage image(imagePaths[i]);
            image.scaled(size.width(), size.height(),
                         Qt::KeepAspectRatio, Qt::SmoothTransformation);
            labels[i]->setPixmap(QPixmap::fromImage(image));
        }
    }

    pushButtons[topIndex]->setChecked(true);

    connect(&buttonGroup, &QButtonGroup::idClicked,
            this, &Banna::indexedButtonClicked);
    connect(&animationGroup, &QParallelAnimationGroup::finished,
            this, &Banna::manualChangingProcess);

    updateGeometry();
}

void Banna::valueInit() {
    changeImageTimer = new QTimer(this);

    int buttonCount = sizeof(pushButtons) / sizeof(pushButtons[0]);

    // 初始化banna列表
    for (int i = 0; i < buttonCount; ++i)
    {
        BannaItem bannaItem;
        bannaItem.pushButton = pushButtons[i];
        bannaItem.currentImageLabel = labels[i];
        bannaItem.nextImageLabel = labels[(i + 1 + buttonCount) % buttonCount];
        bannaItem.prevImageLabel = labels[(i - 1 + buttonCount) % buttonCount];
        bannaItem.propAnimation =
            new QPropertyAnimation(bannaItem.currentImageLabel, ANIMATION_GEOMERTY);
        bannaList.append(bannaItem);

        // 标签处理事件前，先经过本模块处理
        bannaItem.currentImageLabel->installEventFilter(this);
        // 设置动画
        bannaItem.propAnimation->setEasingCurve(ANIMATION_TYPE);

        animationGroup.addAnimation(bannaItem.propAnimation);
    }
    // 统一设置时间
    updateAnimationDuration(ANIMATION_DURATION);

    connect(changeImageTimer, &QTimer::timeout,
            this, &Banna::changeImage);

    sortGeometry(false, false);
    changeImageTimer->start(TIMER_INTERVAL);
}

void Banna::updateAnimationDuration(int duration) {
    for (BannaItem& bannaItem : bannaList)
    {
        bannaItem.propAnimation->setDuration(duration);
    }
}

// 计算x与base的N进制循环距离
int Banna::getDiff(int base, int x, int N) {
    int d = x - base;
    int half = N / 2;

    if (d > half)
        d -= N;
    else if (d < -half)
        d += N;

    return d;
}

QSize Banna::getLabelSize(int index) {
    // 页面最多显示5个图片
    int buttonCount = sizeof(pushButtons) / sizeof(pushButtons[0]);
    const qreal base = 0.5, alpha = 0.8;
    int frameW = this->width();
    int frameH = this->height();
    qreal factor = qPow(alpha, qAbs(getDiff(topIndex, index, buttonCount)));

    return QSize(
        qMin((int)(frameW * base * factor), frameH),
        qMin((int)(frameH * base * factor), frameW)
        );
}

void Banna::updateGeometry() {
    int frameW = this->width();
    int frameH = this->height();

    int bannaCount = bannaList.size();
    // 获取顶层图层尺寸
    QSize topSize = getLabelSize(topIndex);
    // 底层图层需要露出的宽度
    int gap = (frameW - topSize.width()) / (bannaCount - 1);
    // 按键的间隔
    int spacing = pushButtons[0]->width() / 4;
    int buttonPeri = pushButtons[0]->width();

    for (int i = 0; i < bannaCount; ++i) {
        int diff = getDiff(topIndex, i, bannaCount);
        QSize size = getLabelSize(i);
        QPoint labelPos, buttonPos;

        if (diff > 0) {
            labelPos.setX((frameW + topSize.width()) / 2 + diff * gap - size.width());
        }
        else if (diff < 0) {
            labelPos.setX((frameW - topSize.width()) / 2 + diff * gap);
        }
        else {
            labelPos.setX((frameW - size.width()) / 2);
        }

        labelPos.setY((frameH - size.height()) / 2 - 0.1 * topSize.height());
        buttonPos.setX(
            (frameW - buttonPeri) / 2 +
            diff * (buttonPeri + spacing)
            );
        buttonPos.setY((frameH + topSize.height()) / 2 + 0.1 * topSize.height());

        // 设置图片位置
        labels[i]->setGeometry(labelPos.x(), labelPos.y(),
                               size.width(), size.height());

        // 设置按键位置
        pushButtons[i]->move(buttonPos);
    }
}

void Banna::resizeEvent(QResizeEvent *event) {
    (void)event;
    updateGeometry();
}

void Banna::changeImage() {
    if (animationGroup.state() != QAbstractAnimation::Running) {
        this->setNextAnimation();
    }
}

void Banna::indexedButtonClicked(int id) {
    // 当前没有动画，且没有切换的必要
    if (id == topIndex &&
        animationGroup.state() != QAbstractAnimation::Running)
        return;

    manualChanging = true;
    targetIndex = id;

    changeImageTimer->stop();

    // 还有动画在执行，正在前往topIndex
    if (animationGroup.state() == QAbstractAnimation::Running)
        return;

    updateAnimationDuration(ANIMATION_DURATION / 5);

    int diff = getDiff(topIndex, targetIndex, bannaList.size());

    if (diff > 0)
        this->setNextAnimation();
    else if (diff < 0)
        this->setPrevAnimation();
    // else {
    //     manualChanging = false;
    //     updateAnimationDuration(ANIMATION_DURATION);
    //     changeImageTimer->start();
    // }
}

void Banna::manualChangingProcess() {
    if (!manualChanging)
        return;

    int diff = getDiff(topIndex, targetIndex, bannaList.size());

    if (diff == 0) {
        manualChanging = false;

        updateAnimationDuration(ANIMATION_DURATION);
        changeImageTimer->start();
        return;
    }

    updateAnimationDuration(ANIMATION_DURATION / 4);

    if (diff > 0)
        setNextAnimation();
    else
        setPrevAnimation();
}

// 设置向后移动动画
void Banna::setNextAnimation() {
    for (const BannaItem &bannaItem : bannaList) {
        bannaItem.propAnimation->setStartValue(
            bannaItem.currentImageLabel->geometry());
        bannaItem.propAnimation->setEndValue(
            bannaItem.prevImageLabel->geometry());
    }
    animationGroup.start();     // 默认保留运行完的动画
    this->sortGeometry(true, false);
}

// 设置向前移动动画
void Banna::setPrevAnimation() {
    for (const BannaItem &bannaItem : bannaList) {
        bannaItem.propAnimation->setStartValue(
            bannaItem.currentImageLabel->geometry());
        bannaItem.propAnimation->setEndValue(
            bannaItem.nextImageLabel->geometry());
    }
    animationGroup.start();     // 默认保存运行完的动画
    this->sortGeometry(false, true);
}

/*
 * 图层排序
 */
void Banna::sortGeometry(const bool &isNextFlag, const bool &isPrevFlag) {
    int bannaCount = bannaList.size();

    pushButtons[topIndex]->setChecked(false);

    if (!(isNextFlag ^ isPrevFlag))
        topIndex = topIndex;
    else if (isNextFlag)
        topIndex = (topIndex + 1) % bannaCount;
    else
        topIndex = (topIndex - 1 + bannaCount) % bannaCount;

    pushButtons[topIndex]->setChecked(true);

    for (int i = 1; i <= bannaCount / 2; ++i) {
        bannaList[(topIndex + i + bannaCount) % bannaCount].currentImageLabel->lower();
        bannaList[(topIndex - i + bannaCount) % bannaCount].currentImageLabel->lower();
    }
}