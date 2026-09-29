#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QLayout>
#include <QStackedWidget>
#include <QListWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWidget;

struct WidgetItemData {
    QString name;
    typedef QWidget *(*createInsatnce_t)();
    createInsatnce_t createInstance;    // 特定控件的构造函数

    QWidget *widget = nullptr;
};

const QString styleFiles[] = {
    ":/style/mainWidget.qss",
    ":/style/banna.qss",
    ":/style/sqlClock.qss",
    ":/style/videoplayer.qss",
    ":/style/musicplayer.qss",
    ":/style/opencv.qss",
    ":/style/facedetect.qss",
    ":/style/videoMonitor.qss",
    ":/style/gestureDetect.qss"
};
}
QT_END_NAMESPACE

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget() override;

private:
    Ui::MainWidget *ui;

    QHBoxLayout *hBoxLayout;

    QStackedWidget *stackedWidget;

    QListWidget *listWidget;

    QList<Ui::WidgetItemData> naviItems;

private slots:
    void changeNaviItem(int index);
    void changePage(int index);
};
#endif // MAINWIDGET_H
