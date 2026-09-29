#ifndef NUMBERPICKER_H
#define NUMBERPICKER_H
#include <QWidget>
#include <QPropertyAnimation>

// 一次显示的数字只能为奇数个
constexpr int ENTRY_COUNT = 3;

class NumberPicker: public QWidget {
    Q_OBJECT
    // 为动画提供属性及其方法
    Q_PROPERTY(int deviation READ getDeviation WRITE setDeviation)
public:
    NumberPicker(QWidget *parent = nullptr);
    ~NumberPicker();

    void    setRange(int min, int max);
    void    setStep(int step);
    int     getStep();
    void    setValue(int value);
    int     getValue();

private:
    // 可选择数值范围
    int rangeMin;
    int rangeMax;

    // 步长
    int step;

    // 当前值
    int current;

    // 偏移量
    int deviation;

    // 拖拽松开后，选项吸附动画
    QPropertyAnimation *propertyAnimation;

    // 拽动标志
    bool isDragging;

    // 点击屏幕时的y坐标
    int srcPos;

    // 私有成员函数
    int     stepTo(int from, int steps);
    void    startAnimation();
    void    setDeviation(int);
    int     getDeviation();
    void    paintRect(QPainter &painter);
    void    paintNumber(QPainter &painter, int num, int steps);

    // 事件处理
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void paintEvent(QPaintEvent *e);

signals:
    void numberPicked(int num);
};

#endif // NUMBERPICKER_H
