#include "splash.h"
#include <QLayout>
#include <QPainter>

Splash::Splash(QWidget *parent):
    QWidget(parent),
    m_opacity(0)
{
    setStyleSheet(
        QString("background: black;"));

    // 淡入
    anim = new QPropertyAnimation(this, "m_opacity", this);
    anim->setDuration(600);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
}

void Splash::start() {
    connect(anim, &QPropertyAnimation::finished,
            this, [this]() {
                emit inFinished();
            });

    this->show();
    anim->start();
}

// 淡出，时机自己决定
void Splash::finish() {
    anim->stop();

    anim->setDuration(400);
    anim->setStartValue(m_opacity);
    anim->setEndValue(0.0);
    anim->setEasingCurve(QEasingCurve::InCubic);

    anim->disconnect(this);
    connect(anim, &QPropertyAnimation::finished,
            this, [this]() {
        emit outFinished();
    });

    anim->start();
}

void Splash::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 主标题
    QColor titleColor(Qt::white);
    titleColor.setAlphaF(m_opacity);
    painter.setPen(titleColor);

    QFont font = painter.font();
    font.setPixelSize(32);
    font.setBold(true);
    painter.setFont(font);

    QRect titleRect(0, height() / 2 - 40, width(), 50);
    painter.drawText(titleRect, Qt::AlignCenter, "SMART TERMINAL");

    // 副标题
    QColor subColor(143, 163, 184);
    subColor.setAlphaF(m_opacity);
    painter.setPen(subColor);
    font.setPixelSize(16);
    font.setBold(false);
    painter.setFont(font);

    QRect subRect(0, height() / 2 + 16, width(), 30);
    painter.drawText(subRect, Qt::AlignCenter, "made by sm-kz");

    QWidget::paintEvent(event);
}