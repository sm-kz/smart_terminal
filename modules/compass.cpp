#include "compass.h"
#include <QPainter>
#include <qmath.h>

const QColor backgroundColor(20, 20, 250);
const QColor tickColor(Qt::white);
const QColor pointerColor1(40, 40, 40);
const QColor pointerColor2(215, 215, 215);
const QColor pointerTipColor(235, 190, 45);
const QColor pivotColor(110, 110, 110);

Compass::Compass(QWidget *parent):
    QWidget(parent),
    deg(0)
{
    anim = new QPropertyAnimation(this, "deg");
    anim->setPropertyName("deg");
    anim->setDuration(1000);
    anim->setEasingCurve(QEasingCurve::InElastic);

    connect(anim, &QPropertyAnimation::finished,
            this, [this]() {
        clipDeg(this->deg);
    });
}

int Compass::getDeg() {
    return deg;
}

void Compass::rotate(int deg) {
    int target;

    clipDeg(deg);
    if (deg > 180)
        target = this->deg - (360 - deg);
    else
        target = this->deg + deg;

    anim->setStartValue(this->deg);
    anim->setEndValue(target);
    anim->start();
}

void Compass::rotateTo(int deg) {
    int diff = deg - this->deg;

    rotate(diff);
}

void Compass::setDeg(int deg) {
    this->deg = deg;
    update();
}

void Compass::clipDeg(int& deg) {
    if (deg >= 0)
        deg %= 360;
    else
        deg = -(-deg % 360) + 360;
}

// 保持正方形形状
void Compass::resizeEvent(QResizeEvent *event) {
    QSize size = event->size();

    if (size.width() != size.height()) {
        int target = qMin(size.width(), size.height());
        resize(target, target);
    }

    update();
    QWidget::resizeEvent(event);
}

void Compass::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    int _width = width();

    // 背景
    int radius = 0.4 * _width;
    painter.setBrush(QBrush(backgroundColor));
    painter.drawEllipse(0.1 * _width, 0.1 * _width, 2 * radius, 2 * radius);

    // 刻度
    painter.save();
    painter.setPen(QPen(tickColor, 3));
    painter.translate(_width / 2, _width / 2);

    const int tickLarge = 0.12 * radius;
    const int tickSmall = 0.06 * radius;
    for (int i = 0; i < 80; ++i) {
        int len = i % 10 == 0 ? tickLarge : tickSmall;
        painter.drawLine(
            radius, 0,
            radius - len, 0
            );

        painter.rotate(4.5);
    }

    painter.restore();

    // 指针和 pivot
    painter.save();
    painter.translate(_width / 2, _width / 2);

    const int pointerLong = 0.8 * radius;
    const int pointerShort = 0.2 * radius;
    // 旋转是逆时针，按坐标系的角度，应该反过来
    painter.rotate(-deg);

    painter.setBrush(QBrush(pointerColor1));
    painter.drawPolygon(QPolygon({
        {0, pointerShort},
        {0, -pointerShort},
        {pointerLong, 0},
        {-pointerLong, 0}
        }));

    painter.setBrush(QBrush(pointerColor2));
    painter.drawPolygon(QPolygon({
        {0, pointerShort},
        {0, -pointerShort},
        {-pointerLong, 0},
        {pointerLong, 0}
    }));

    painter.setBrush(QBrush(pointerTipColor));
    painter.drawPolygon(QPolygon({
        {0, pointerShort},
        {3 * pointerShort, 0},
        {0, -pointerShort},
        {pointerLong, 0},
    }));

    painter.setBrush(QBrush(pivotColor));
    painter.drawEllipse(
        QPoint(0, 0),
        radius / 20,
        radius / 20
    );

    painter.restore();

    QWidget::paintEvent(event);
}