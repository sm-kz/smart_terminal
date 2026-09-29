#include "switchButton.h"
#include <QPainter>
#include <QPainterPath>
#include <QBrush>

SwitchButton::SwitchButton(QWidget *parent):
    QWidget(parent),
    enabled(true),
    checked(false),
    background(Qt::gray),
    checkedColor(Qt::blue),
    uncheckedColor(Qt::darkGray),
    thumbColor(Qt::cyan),
    radius(12),
    margin(3)
{
    setMinimumSize(2 * (radius + margin), 2 * (radius + margin));
    anim = new QPropertyAnimation(this, "thumbPos");
    anim->setDuration(300);
    anim->setEasingCurve(QEasingCurve::InCubic);
    thumbPos = margin + radius;
    update();
}

SwitchButton::~SwitchButton() {

}

void SwitchButton::setPos(int newPos) {
    thumbPos = newPos;
    update();
}

int SwitchButton::getPos() {
    return thumbPos;
}

void SwitchButton::paintEvent(QPaintEvent *e) {
    Q_UNUSED(e);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 画滑轨
    drawRail(painter);

    // 画拖柄
    drawThumb(painter);
}

void SwitchButton::drawRail(QPainter &painter) {
    QRectF railRect(
        margin,
        margin,
        width() - 2 * margin,
        height() - 2 * margin
    );

    painter.save();

    qreal r = railRect.height() / 2.0;
    QPainterPath railPath;
    railPath.addRoundedRect(railRect, r, r);

    // 限制绘画范围
    painter.setClipPath(railPath);

    if (!enabled) {
        painter.setOpacity(0.2);
        painter.setBrush(uncheckedColor);
        painter.drawPath(railPath);
        painter.restore();
        return;
    }

    // 先画底色
    painter.setPen(QPen(Qt::black, 3));
    painter.setBrush(uncheckedColor);
    painter.setOpacity(0.6);
    painter.drawPath(railPath);

    // 再画checked部分
    painter.setPen(Qt::NoPen);
    painter.setBrush(checkedColor);
    painter.setOpacity(0.8);
    painter.fillRect(QRectF(
            railRect.left(),
            railRect.top(),
            thumbPos - railRect.left(),
            railRect.height()
        ), checkedColor);

    painter.restore();
}

void SwitchButton::drawThumb(QPainter &painter) {
    int opacity;
    if (!enabled) {
        opacity = 0.2;
        painter.setBrush(uncheckedColor);
    }
    else {
        opacity = 1;
        painter.setBrush(thumbColor);
    }

    painter.setPen(QPen(Qt::white, 3));
    painter.setOpacity(opacity);
    painter.drawEllipse(QRect(thumbPos - radius, margin,
                              2 * radius, 2 * radius));
}

void SwitchButton::mousePressEvent(QMouseEvent *e) {
    if (enabled) {
        e->accept();
    }
}

void SwitchButton::mouseReleaseEvent(QMouseEvent *e) {
    if (!enabled) {
        e->ignore();
        return;
    }

    checked = !checked;

    if (checked) {
        anim->setStartValue(margin + radius);
        anim->setEndValue(width() - radius - margin);
    } else {
        anim->setStartValue(width() - radius - margin);
        anim->setEndValue(margin + radius);
    }

    emit toggled(checked);
    anim->start();
    e->accept();
}

void SwitchButton::resizeEvent(QResizeEvent *e) {
    radius = height() / 2 - margin;

    if (checked)
        thumbPos = width() - margin - radius;
    else
        thumbPos = margin + radius;

    update();
    QWidget::resizeEvent(e);
}

void SwitchButton::setChecked(bool checked) {
    this->checked = checked;
    update();
}

bool SwitchButton::isChecked() {
    return checked;
}

void SwitchButton::setEnabled(bool en) {
    enabled = en;
    update();
}

bool SwitchButton::isEnabled() {
    return enabled;
}