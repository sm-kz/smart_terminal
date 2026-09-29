#ifndef PROXIMITYRADAR_H
#define PROXIMITYRADAR_H

#include <QWidget>
#include <QPropertyAnimation>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QColor>
#include <QTimer>

class ProximityRadar: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int dist READ getDist WRITE setDist)
public:
    ProximityRadar(QWidget *parent = nullptr);

    int getDist();
    void updateDist(int dist);
    void setBackgroundColor(QColor);
    void setForegroundColor(QColor);
protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    // 当前靠近距离
    int dist;

    // 背景和前景颜色
    QColor backgroundColor;
    QColor foregroundColor;

    // 波动相位
    qreal phase;

    // 波动定时器
    QTimer *waveTimer;

    QPropertyAnimation *anim;

    void setDist(int dist);
};

#endif // PROXIMITYRADAR_H
