#include "mainWidget.h"
#include "ui_mainWidget.h"
#include "banna.h"
#include "sqlClock.h"
#include "videoplayer.h"
#include "opencv.h"
#include "facedetect.h"
#include "videoMonitor.h"
#include "musicPlayer.h"
#include "gestureDetect.h"
#include "pageEvent.h"

#include <QScreen>
#include <QList>
#include <QFile>
#include <signal.h>
#include <QTimer>

static volatile sig_atomic_t sigintOccured = 0;

void sigintHandler(int sig) {
    Q_UNUSED(sig);
    sigintOccured = true;
}

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
{
    ui->setupUi(this);

#if __arm__
    QList<QScreen *> screens = QApplication::screens();
    this->resize(screens.at(0)->geometry().width(),
                   screens.at(0)->geometry().height());
#else
    this->resize(800, 480);
#endif

    /* 水平布局实例化 */
    hBoxLayout = new QHBoxLayout();

    /* 控件堆栈实例化 */
    stackedWidget = new QStackedWidget();

    /* 列表控件实例化 */
    listWidget = new QListWidget();

    // 定时检查是否有SIGINT产生，避免摄像头等设备因为中断而不被释放导致的锁死
    struct sigaction sa;
    sa.sa_handler = sigintHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout,
            this, [this](){
                if (sigintOccured) {
                    // 执行正常退出流程，释放控件资源
                    qApp->quit();
                }
            });
    timer->start(50);

    /* 页面导航栏 */
    naviItems = {
        {"Carousel", []()->QWidget *{ return new Banna(); }},
        {"SQL Clock", []()->QWidget *{ return new SqlClock(); }},
        {"VideoPlayer", []()->QWidget *{ return new VideoPlayer(); }},
        {"MusicPlayer", []()->QWidget *{ return new MusicPlayer(); }},
        {"OpenCV", []()->QWidget *{ return new Opencv(); }},
        {"Face Detect", []()->QWidget *{ return new FaceDetect(); }},
        {"Video Monitor", []()->QWidget *{ return new VideoMonitor(); }},
        {"Gesture Detect", []()->QWidget * { return new GestureDetect(); }}
    };

    for (const auto &item : naviItems) {
        QListWidgetItem *listItem = new QListWidgetItem(item.name);
        listItem->setTextAlignment(Qt::AlignCenter);
        listItem->setSizeHint(QSize(100, 40));
        listWidget->addItem(listItem);

        // lazy 初始化
        stackedWidget->addWidget(new QWidget());

        // 一次性初始化
        // stackedWidget->addWidget(item.createInstance());
    }

    listWidget->setFixedWidth(150);
    listWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    hBoxLayout->addWidget(listWidget);
    hBoxLayout->addWidget(stackedWidget);
    hBoxLayout->setContentsMargins(0, 0, 0, 0);
    hBoxLayout->setSpacing(0);
    this->setLayout(hBoxLayout);

    // 加载样式文件
    QFile styleFile;
    QString styleSheet;
    for (const auto &fileName : Ui::styleFiles) {
        styleFile.setFileName(fileName);
        if (!styleFile.exists())
            continue;

        styleFile.open(QIODevice::ReadOnly);
        styleSheet += QString(styleFile.readAll());
        qApp->setStyleSheet(styleSheet);
        styleFile.close();
    }

    connect(listWidget, SIGNAL(currentRowChanged(int)),
            this, SLOT(changeNaviItem(int)));
    // connect(listWidget, &QListWidget::currentRowChanged,
    //         this, &MainWidget::changePage);

    if (listWidget->count() > 0)
        listWidget->setCurrentRow(0);
}

MainWidget::~MainWidget()
{
    delete ui;
}

// lazy加载：只有点击后才初始化控件
void MainWidget::changeNaviItem(int index) {
    if (index < 0 || index > naviItems.size())
        return;

    int oldIndex = stackedWidget->currentIndex();

    if (oldIndex > 0 && oldIndex != index) {
        QWidget *oldWidget = stackedWidget->widget(oldIndex);
        if (oldWidget) {
            QEvent event(PageEvent::leaveType());
            QApplication::sendEvent(oldWidget, &event);
        }
    }

    Ui::WidgetItemData &newItem = naviItems[index];
    if (!newItem.widget) {
        QWidget *temp = stackedWidget->widget(index);

        newItem.widget = newItem.createInstance();
        stackedWidget->removeWidget(temp);
        stackedWidget->insertWidget(index, newItem.widget);

        delete temp;
    }

    stackedWidget->setCurrentIndex(index);

    // 通知新控件
    QEvent event(PageEvent::enterType());
    QApplication::sendEvent(newItem.widget, &event);
}

// 全量加载：切换页面后发送页面切换事件，让占用硬件的控件及时释放
void MainWidget::changePage(int index) {
    int oldIndex = stackedWidget->currentIndex();

    if (oldIndex == index)
        return;

    QWidget *oldWidget = stackedWidget->widget(oldIndex);
    if (oldWidget) {
        QEvent event(PageEvent::leaveType());
        QApplication::sendEvent(oldWidget, &event);
    }

    stackedWidget->setCurrentIndex(index);

    QWidget *curWidget = stackedWidget->widget(index);
    if (curWidget) {
        QEvent event(PageEvent::enterType());
        QApplication::sendEvent(oldWidget, &event);
    }
}