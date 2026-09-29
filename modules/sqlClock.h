#ifndef SQLCLOCK_H
#define SQLCLOCK_H
#include <QWidget>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QLabel>
#include <QListWidget>
#include <QDialog>
#include <QPushButton>
#include <QLayout>
#include <QMouseEvent>

#include "numberPicker.h"
#include "switchButton.h"

namespace sqlclock {
enum sqlField {
    idField = 0,
    commentFiled,
    timeField,
    activeField
};
} // namespace sqlclk

struct ItemObjectInfo {
    QWidget *widget;
    QLabel *time;
    SwitchButton *switchButton;
    QHBoxLayout *hInnerLayout;

    ItemObjectInfo(const QString &timeStr, bool active);
};

class SqlClock: public QWidget {
    Q_OBJECT
public:
    SqlClock(QWidget *parent = nullptr);
    ~SqlClock();

private:
    // 数据库实例
    QSqlDatabase sqlDatabase;

    // 数据库模型
    QSqlTableModel *tableModel;

    // 时钟选择器
    NumberPicker *hourPicker;
    NumberPicker *minutePicker;

    // 时分秒显示标签
    QLabel *labels[3];

    // 侧边列表栏
    QListWidget *listWidget;

    // 底部控件
    QWidget *bottomWidget;

    // 对话框
    QDialog *alarmDialog;

    // 时间选择控件
    QWidget *timeWidget;

    // 按键
    QWidget *buttonWidget;
    QPushButton *addButton;
    QPushButton *delButton;
    QPushButton *yesButton;
    QPushButton *noButton;

    // 布局
    QVBoxLayout *vMainLayout;
    QHBoxLayout *hBottomLayout;
    QHBoxLayout *hDialogLayout;
    QVBoxLayout *vDialogLayout;
    QHBoxLayout *hButtonLayout;

    // 项目对象信息列表
    QList<ItemObjectInfo> infos;

    void showAllAlarm();

    bool eventFilter(QObject *watched, QEvent *e);

private slots:
    void switchButtonClicked(bool checked);
    void addButtonClicked();
    void delButtonClicked();
    void yesButtonClicked();
    void noButtonClicked();
    void listItemClicked(QListWidgetItem*);
};

#endif // SQLCLOCK_H
