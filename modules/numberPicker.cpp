#include "numberPicker.h"
#include <QDebug>
#include <QPainter>
#include <QMouseEvent>
#include <QPaintEvent>

NumberPicker::NumberPicker(QWidget *parent):
    QWidget(parent),
    rangeMin(0),
    rangeMax(10),
    step(1),
    current(0),
    deviation(0),
    isDragging(false),
    srcPos(0)
{
    setMinimumSize(100, 300);
    propertyAnimation = new QPropertyAnimation(this, "deviation");
    propertyAnimation->setDuration(300);
    propertyAnimation->setEasingCurve(QEasingCurve::OutCubic);
}

NumberPicker::~NumberPicker() {

}

void NumberPicker::setRange(int min, int max) {
    if (min > max) {
        qDebug() << "[NumberPicker]" << "invalid range "
                 << "(" << min << ", " << "max";
        return;
    }

    rangeMin = min;
    rangeMax = max;

    // 对齐rangeMax
    int s = getStep();
    setStep(s);
}

void NumberPicker::setStep(int step) {
    if (step <= 0) {
        qDebug() << "[NumberPicker]step should be more than 0";
        return;
    }

    this->step = step;

    // 范围内不能被step均分
    if ((rangeMax - rangeMin) % step) {
        rangeMax = rangeMin + (rangeMax - rangeMin) / step * step;
    }
}

int NumberPicker::getStep() {
    return step;
}

void NumberPicker::mousePressEvent(QMouseEvent *e) {
    propertyAnimation->stop();
    isDragging = true;
    srcPos = e->y();
    QWidget::mousePressEvent(e);
}

void NumberPicker::mouseMoveEvent(QMouseEvent *e) {
    if (!isDragging)
        return;

    int delta = e->y() - srcPos;
    srcPos = e->y();

    deviation += delta;
    int frameHeight = height() / ENTRY_COUNT;

    // 每次移动事件最多移动一格
    if (deviation >= frameHeight) {
        deviation -= frameHeight;

        current += step;
        if (current > rangeMax)
            current = rangeMin;
    }
    else if (deviation <= -frameHeight) {
        deviation += frameHeight;

        current -= step;
        if (current < rangeMin)
            current = rangeMax;
    }
    update();
}

void NumberPicker::mouseReleaseEvent(QMouseEvent *e) {
    (void)e;
    if (isDragging) {
        isDragging = false;
        startAnimation();
    }
}

void NumberPicker::paintEvent(QPaintEvent *e) {
    (void)e;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    int _height = height();

    // 向下移动一格
    if (deviation >= _height / ENTRY_COUNT) {
        deviation -= _height / ENTRY_COUNT;
        current += step;
        if (current > rangeMax)
            current = rangeMin;
    }

    // 向上移动一格
    if (deviation <= -_height / ENTRY_COUNT) {
        deviation += _height / ENTRY_COUNT;
        current -= step;
        if (current < rangeMin)
            current = rangeMax;
    }

    // 绘制框
    paintRect(painter);

    // 绘制数字
    paintNumber(painter, current, 0);

    int before, after;
    for (int i = 1; i <= ENTRY_COUNT / 2; ++i) {
        before = stepTo(current, -i);
        after = stepTo(current, i);

        paintNumber(painter, before, -i);
        paintNumber(painter, after, i);
    }
}

int NumberPicker::stepTo(int from, int steps) {
    int target;
    // 这是两边闭区间，走完一个循环还需要+1
    int cycleSteps = (rangeMax - rangeMin) / step + 1;

    steps %= cycleSteps;
    target = from + steps * step;

    if (steps >= 0) {
        if (target <= rangeMax)
            return target;

        target = target - (rangeMax - rangeMin) - step;
        return target;
    } else {
        if (target >= rangeMin)
            return target;

        target = target + (rangeMax - rangeMin) + step;
        return target;
    }
}

void NumberPicker::paintRect(QPainter &painter) {
    int _width = width();
    int _height = height();
    int frameHeight = _height / ENTRY_COUNT;

    QBrush brush(QColor(230, 230, 230, 20));
    painter.setPen(Qt::NoPen);
    painter.setBrush(brush);
    painter.drawRect(QRect(0, _height / 2 - frameHeight / 4,
                        _width, frameHeight / 2));
}

void NumberPicker::paintNumber(QPainter &painter, int num, int steps) {
    int _width = width();
    int _height = height();
    int frameHeight = _height / ENTRY_COUNT;

    // 越偏离中心，字体越小
    int fontSize = frameHeight / (qAbs(steps) + 1) / 2;
    int transparency = 255 - 255 * qAbs(steps) / (qAbs(steps) + 1);
    int pos = _height / 2 - frameHeight / 2 - steps * frameHeight + deviation;

    QFont font;
    font.setPixelSize(fontSize);
    painter.setFont(font);
    painter.setPen(QColor(0, 0, 0, transparency));

    if (num < 10)
        painter.drawText(QRect(0, pos, _width, frameHeight),
                         Qt::AlignCenter,
                         "0" + QString::number(num));
    else
        painter.drawText(QRect(0, pos, _width, frameHeight),
                         Qt::AlignCenter,
                         QString::number(num));
}

void NumberPicker::startAnimation() {
    int _height = height();
    int frameHeight = _height / ENTRY_COUNT;

    if (qAbs(deviation) < frameHeight / 2) {
        propertyAnimation->setStartValue(deviation);
        propertyAnimation->setEndValue(0);
    } else if (deviation >= frameHeight / 2) {
        propertyAnimation->setStartValue(deviation);
        propertyAnimation->setEndValue(frameHeight);
    } else {
        propertyAnimation->setStartValue(deviation);
        propertyAnimation->setEndValue(-frameHeight);
    }

    // emit this->numberPicked(current);
    propertyAnimation->start();
}

void NumberPicker::setDeviation(int devi) {
    deviation = devi;
    update();
}

int NumberPicker::getDeviation() {
    return deviation;
}

void NumberPicker::setValue(int value) {
    if (value < rangeMin || value > rangeMax)
        return;

    int mid = (value - rangeMin) % step;
    if (mid) {
        int left = value - mid;
        int right = left + step;

        if (2 * mid >= step)
            current = right;
        else
            current = left;
    } else
        current = value;

    update();
}

int NumberPicker::getValue() {
    return current;
}
