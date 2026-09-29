#ifndef SWITCHBUTTON_H
#define SWITCHBUTTON_H

#include <QWidget>
#include <QColor>
#include <QPropertyAnimation>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QResizeEvent>

class SwitchButton: public QWidget {
    Q_OBJECT
    Q_PROPERTY(int thumbPos READ getPos WRITE setPos)

public:
    SwitchButton(QWidget *parent = nullptr);
    ~SwitchButton();

    void setChecked(bool);
    bool isChecked();
    void setEnabled(bool);
    bool isEnabled();

private:
    // 按键使能
    bool enabled;

    // 按键状态
    bool checked;

    // 背景颜色
    QColor background;

    // checked 颜色
    QColor checkedColor;

    // unchecked 颜色
    QColor uncheckedColor;

    // 拖柄块颜色
    QColor thumbColor;

    // 拖柄半径
    int radius;

    // 边缘
    int margin;

    // 拖柄中心点位置
    int thumbPos;

    // 动画
    QPropertyAnimation *anim;

    void paintEvent(QPaintEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void resizeEvent(QResizeEvent *e);

    void drawRail(QPainter &painter);
    void drawThumb(QPainter &painter);

    void setPos(int newPos);
    int getPos();

signals:
    void toggled(bool);
};

#endif // SWITCHBUTTON_H
