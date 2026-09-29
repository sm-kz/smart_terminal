#include "sqlClock.h"
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QFont>
#include <QList>
#include <QTime>

using namespace sqlclock;

ItemObjectInfo::ItemObjectInfo(const QString &timeStr, bool active) {
    widget = new QWidget();
    time = new QLabel();
    switchButton = new SwitchButton();
    hInnerLayout = new QHBoxLayout();

    widget->setObjectName("alarm-item");
    time->setObjectName("alarm-time");

    time->setText(timeStr);
    switchButton->setMaximumWidth(80);
    switchButton->setChecked(active);
    hInnerLayout->addWidget(time);
    hInnerLayout->addWidget(switchButton);
    hInnerLayout->setAlignment(Qt::AlignVCenter);
    widget->setLayout(hInnerLayout);
}

SqlClock::SqlClock(QWidget *parent):
    QWidget(parent)
{
    // 获取本机可用数据库驱动
    // QStringList drivers = QSqlDatabase::drivers();

    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        qDebug() << "[SqlClock]Aborted: this machine dosen't support sqlite";
        return;
    }

    sqlDatabase = QSqlDatabase::addDatabase("QSQLITE");
    sqlDatabase.setDatabaseName("alarm.db");
    if (!sqlDatabase.open())
        qDebug() << "[SqlClock]Connection Error: "
                 << sqlDatabase.lastError();
    else
        qDebug() << "[SqlClock]Connect Succeed";

    QSqlQuery query(sqlDatabase);
    // query.exec("DROP TABLE alarm");
    // 创建表
    query.prepare("CREATE TABLE IF NOT EXISTS alarm("
                  "id INTEGER PRIMARY KEY,"
                  "comment VARCHAR(50),"
                  "time VARCHAR(15),"
                  "active BOOL DEFAULT false"
                  ")");
    query.exec();

    tableModel = new QSqlTableModel(this, sqlDatabase);
    // 设置表模型的名字，需要与数据库中表的名字一致
    tableModel->setTable("alarm");
    // 切换行之后，数据同步
    tableModel->setEditStrategy(QSqlTableModel::OnRowChange);

    hourPicker = new NumberPicker();
    minutePicker = new NumberPicker();

    hourPicker->setRange(0, 23);
    minutePicker->setRange(0, 59);

    QFont font;
    font.setBold(true);
    font.setPixelSize(200);

    for (int i = 0; i < 3; ++i) {
        labels[i] = new QLabel();
        labels[i]->setFont(font);
    }

    listWidget = new QListWidget();
    bottomWidget = new QWidget();
    alarmDialog = new QDialog(this);
    timeWidget = new QWidget();
    buttonWidget = new QWidget();
    addButton = new QPushButton();
    delButton = new QPushButton();
    yesButton = new QPushButton();
    noButton = new QPushButton();

    listWidget->setObjectName("alarm-list");
    bottomWidget->setObjectName("alarm-bottom");
    alarmDialog->setObjectName("alarm-dialog");
    addButton->setObjectName("alarm-add");
    delButton->setObjectName("alarm-delete");
    yesButton->setObjectName("alarm-confirm");
    noButton->setObjectName("alarm-cancel");

    addButton->setMinimumSize(84, 84);
    addButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    addButton->setText("Add");
    delButton->setMinimumSize(84, 84);
    delButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    delButton->setText("Delete");
    bottomWidget->setMinimumHeight(84);

    yesButton->setMinimumSize(100, 50);
    yesButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    yesButton->setText("Confirm");
    noButton->setMinimumSize(100, 50);
    noButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    noButton->setText("Cancel");
    buttonWidget->setMinimumHeight(70);

    alarmDialog->setMaximumSize(300, 300);
    alarmDialog->setModal(true);

    vMainLayout = new QVBoxLayout();
    hBottomLayout = new QHBoxLayout();
    hDialogLayout = new QHBoxLayout();
    vDialogLayout = new QVBoxLayout();
    hButtonLayout = new QHBoxLayout();

    // 主界面布局
    vMainLayout->addWidget(listWidget);
    vMainLayout->addWidget(bottomWidget);

    this->setLayout(vMainLayout);

    // 底部按钮布局
    hBottomLayout->addWidget(addButton, 1);
    hBottomLayout->addWidget(delButton, 1);
    bottomWidget->setLayout(hBottomLayout);

    // 对话框布局
    vDialogLayout->addWidget(timeWidget);
    vDialogLayout->addWidget(buttonWidget);
    alarmDialog->setLayout(vDialogLayout);

    // 标签用来占位
    hDialogLayout->addWidget(labels[0]);
    hDialogLayout->addWidget(hourPicker);
    hDialogLayout->addWidget(labels[1]);
    hDialogLayout->addWidget(minutePicker);
    hDialogLayout->addWidget(labels[2]);
    timeWidget->setLayout(hDialogLayout);

    hButtonLayout->addWidget(yesButton, 1);
    hButtonLayout->addWidget(noButton, 1);
    buttonWidget->setLayout(hButtonLayout);

    // 获取所有闹钟数据
    tableModel->select();

    // 打印所有闹钟
    showAllAlarm();

    // 在列表里添加闹钟开关
    for (int i = 0; i < tableModel->rowCount(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(listWidget);

        ItemObjectInfo info(tableModel->data(tableModel->index(i, timeField)).toString(),
                            tableModel->data(tableModel->index(i, activeField)).toBool()
                            );

        // 不使用item的默认高度
        item->setSizeHint(info.widget->sizeHint());
        listWidget->setItemWidget(item, info.widget);
        infos.append(info);

        connect(info.switchButton, SIGNAL(toggled(bool)),
                this, SLOT(switchButtonClicked(bool)));

        // 获取数据库中的闹钟状态
        QModelIndex mindex = tableModel->index(i, activeField);
        if (tableModel->data(mindex).toBool()) {
            info.switchButton->setChecked(true);
        }
    }

    // 让本控件接受listWidget的事件
    listWidget->viewport()->installEventFilter(this);

    connect(addButton, &QPushButton::clicked,
            this, &SqlClock::addButtonClicked);
    connect(delButton, &QPushButton::clicked,
            this, &SqlClock::delButtonClicked);
    connect(yesButton, &QPushButton::clicked,
            this, &SqlClock::yesButtonClicked);
    connect(noButton, &QPushButton::clicked,
            this, &SqlClock::noButtonClicked);

    connect(listWidget, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SLOT(listItemClicked(QListWidgetItem*)));
}

SqlClock::~SqlClock() {
    sqlDatabase.close();
}

void SqlClock::showAllAlarm() {
    if (tableModel->rowCount() == 0) {
        qDebug() << "[SqlClock]Alarm record is empty";
        return;
    }

    qDebug() << "******************* SQL Clock Record ********************";
    qDebug() << "| id |       comment         |       time      | active |";
    for (int i = 0; i < tableModel->rowCount(); ++i) {
        int id = tableModel->data(tableModel->index(i, idField)).toInt();
        QString comment = tableModel->data(tableModel->index(i, commentFiled)).toString();
        QString time = tableModel->data(tableModel->index(i, timeField)).toString();
        bool active = tableModel->data(tableModel->index(i, activeField)).toBool();

        qDebug() << "| " << id << " "
                 << "| " << comment << " "
                 << "| " << time << " "
                 << "| " << (active ? "true" : "false") << " |";
    }
    qDebug() << "*********************************************************";
}

void SqlClock::switchButtonClicked(bool checked) {
    SwitchButton *btn = static_cast<SwitchButton *>(sender());
    for (int i = 0; i < infos.size(); ++i) {
        if (btn == infos.at(i).switchButton) {
            if (checked) {
                tableModel->setData(tableModel->index(i, activeField), true);
                listWidget->item(i)->setForeground(QBrush(QColor(22, 22, 22, 225)));
            } else {
                tableModel->setData(tableModel->index(i, activeField), false);
                listWidget->item(i)->setForeground(QBrush(QColor(22, 22, 22, 60)));
            }

            tableModel->submit();
            break;
        }
    }
}

void SqlClock::addButtonClicked() {
    if (addButton->text() == "Add") {
        // 时间选项框默认为当前时间
        QTime now = QTime::currentTime();
        hourPicker->setValue(now.hour());
        minutePicker->setValue(now.minute());

        // 避免与其他行冲突
        listWidget->setCurrentRow(-1);
    } else {
        QStringList list = infos[listWidget->currentRow()].time->text().split(':');
        // 设置选择器的默认值
        hourPicker->setValue(list.at(0).toInt());
        minutePicker->setValue(list.at(1).toInt());
    }

    // 显示对话框
    alarmDialog->show();
}

void SqlClock::delButtonClicked() {
    // 删除并更新模型数据
    tableModel->removeRow(listWidget->currentRow());
    tableModel->submit();
    tableModel->select();
    infos.removeAt(listWidget->currentRow());

    // 更新显示
    listWidget->takeItem(listWidget->currentRow());

    // 确保所有数据同步
    if (tableModel->isDirty())
        tableModel->submitAll();
}

void SqlClock::yesButtonClicked() {
    QString hour;
    QString minute;

    if (hourPicker->getValue() < 10)
        hour = "0" + QString::number(hourPicker->getValue()) + ":";
    else
        hour = QString::number(hourPicker->getValue()) + ":";

    if (minutePicker->getValue() < 10)
        minute = "0" + QString::number(minutePicker->getValue());
    else
        minute = QString::number(minutePicker->getValue());

    // 如果修改的不是已存在的记录
    if (listWidget->currentRow() == -1) {
        // 更新数据库：插入一行新数据
        int row = tableModel->rowCount();
        tableModel->insertRow(row);
        tableModel->setData(tableModel->index(row, timeField), hour + minute);
        tableModel->setData(tableModel->index(row, activeField), true);
        tableModel->submit();

        // 更新表控件
        QListWidgetItem *item = new QListWidgetItem(listWidget);

        ItemObjectInfo info(hour + minute, true);

        // 不使用item的默认高度
        item->setSizeHint(info.widget->sizeHint());
        listWidget->setItemWidget(item, info.widget);
        infos.append(info);

        connect(info.switchButton, SIGNAL(toggled(bool)),
                this, SLOT(switchButtonClicked(bool)));
    } else {
        int row = listWidget->currentRow();
        tableModel->setData(tableModel->index(row, timeField), hour + minute);
        tableModel->submit();

        ItemObjectInfo info = infos.at(row);
        info.time->setText(hour + minute);
    }

    // 确保所有数据同步
    if (tableModel->isDirty())
        tableModel->submitAll();

    alarmDialog->close();
}

void SqlClock::noButtonClicked() {
    alarmDialog->close();
}

void SqlClock::listItemClicked(QListWidgetItem* item) {
    Q_UNUSED(item);
    if (addButton->text() == "Add") {
        addButton->setText("Set");
    } else if (addButton->text() == "Set") {
        // 选中状态下再次点击无效，点击空白处退出选择
    }
}

bool SqlClock::eventFilter(QObject *watched, QEvent *e) {
    if (watched == listWidget->viewport() &&
        e->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = static_cast<QMouseEvent *>(e);

        QListWidgetItem *item = listWidget->itemAt(me->pos());

        if (item == nullptr) {
            listWidget->setCurrentRow(-1);
            listWidget->clearSelection();
            addButton->setText("Add");
        }
    }

    return QWidget::eventFilter(watched, e);
}