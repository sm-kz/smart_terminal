#ifndef SPLASH_H
#define SPLASH_H

#include <QWidget>
#include <QGraphicsEffect>
#include <QPropertyAnimation>
#include <QLabel>
#include <QPaintEvent>

class Splash: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal m_opacity READ opacity WRITE setOpacity)
public:
    Splash(QWidget *parent = nullptr);

    qreal opacity() const {
        return m_opacity;
    }

    void setOpacity(qreal opacity) {
        m_opacity = opacity;
        update();
    }
    void start();
    void finish();

protected:
    void paintEvent(QPaintEvent *event);

signals:
    void inFinished();
    void outFinished();

private:
    qreal m_opacity;
    QPropertyAnimation *anim;
};

#endif // SPLASH_H
