#ifndef COMPASS_H
#define COMPASS_H

#include <QWidget>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QPropertyAnimation>

class Compass: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int deg READ getDeg WRITE setDeg)

public:
    Compass(QWidget *parent = nullptr);

    int getDeg();
    void rotate(int deg);
    void rotateTo(int deg);

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    int deg;
    QPropertyAnimation *anim;

public slots:
    void setDeg(int);
    void clipDeg(int&);
};

#endif // COMPASS_H
