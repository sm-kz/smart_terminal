#include "proximityRadar.h"
#include <QPainter>
#include <qmath.h>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QDebug>

ProximityRadar::ProximityRadar(QWidget *parent):
    QWidget(parent),
    dist(0),
    backgroundColor(20, 20, 250),
    foregroundColor(Qt::white),
    phase(0)
{
    waveTimer = new QTimer(this);

    anim = new QPropertyAnimation(this, "dist");
    anim->setDuration(200);
    anim->setEasingCurve(QEasingCurve::Linear);

    connect(waveTimer, &QTimer::timeout,
            this, [this]() {
        phase += 0.1;
        update();
    });

    waveTimer->start(30);     // 大约 33 FPS
}

int ProximityRadar::getDist() {
    return dist;
}

void ProximityRadar::updateDist(int dist) {
    if (dist > 100)
        dist = 100;
    if (dist < 0)
        dist = 0;

    anim->setStartValue(this->dist);
    anim->setEndValue(dist);
    anim->start();
}

void ProximityRadar::setDist(int dist) {
    this->dist = dist;
    update();
}

void ProximityRadar::setBackgroundColor(QColor color) {
    backgroundColor = color;
}

void ProximityRadar::setForegroundColor(QColor color) {
    foregroundColor = color;
}

void ProximityRadar::resizeEvent(QResizeEvent *event) {
    QSize size = event->size();

    if (size.width() != size.height()) {
        int target = qMin(size.width(), size.height());
        resize(target, target);
    }

    update();
    QWidget::resizeEvent(event);
}

void ProximityRadar::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    painter.setRenderHint(QPainter::Antialiasing);
    int _width = width();

    // 背景
    int radius = 0.4 * _width;
    painter.setBrush(QBrush(backgroundColor));
    painter.drawEllipse(0.1 * _width, 0.1 * _width, 2 * radius, 2 * radius);

    // 距离指示波浪
    painter.setBrush(QBrush(foregroundColor));
    painter.translate(_width / 2, _width / 2);

    int tRadius = dist * radius / 100;
    const int count = 120;

    QPainterPath path;

    for (int i = 0; i <= count; ++i) {
        qreal angle = 2.0 * M_PI * i / count;

        qreal wave = (6.0 * qSin(angle * 3 + phase) +
                      3.0 * qCos(angle * 7 - phase * 0.7) +
                      1.8 * qSin(angle * 11 + phase * 0.4)) * dist / 100;

        qreal r = tRadius + wave;
        QPointF p(r * qCos(angle), r * qSin(angle));

        if (i == 0)
            path.moveTo(p);
        else
            path.lineTo(p);
    }

    path.closeSubpath();
    painter.drawPath(path);

    QWidget::paintEvent(event);
}
