#include "mainWidget.h"
#include "splash.h"
#include <QApplication>
#include <QScreen>
#include <QTimer>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Splash splash;

#if __arm__
    QList<QScreen *> screens = QApplication::screens();
    splash.resize(screens.at(0)->geometry().width(),
                 screens.at(0)->geometry().height());
#else
    splash.resize(800, 480);
#endif

    splash.start();

    // 淡入完再加载控件
    QObject::connect(&splash, &Splash::inFinished, [&]() {
        MainWidget *w = new MainWidget;
        QObject::connect(&splash, &Splash::outFinished, [=]() {
            w->show();
        });
        splash.finish();
    });

    return QApplication::exec();
}
