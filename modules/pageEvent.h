#ifndef PAGEEVENT_H
#define PAGEEVENT_H

#include <QEvent>

class PageEvent: public QEvent
{
public:
    static QEvent::Type leaveType() {
        static QEvent::Type type =
            static_cast<QEvent::Type>(registerEventType());
        return type;
    }

    static QEvent::Type enterType() {
        static QEvent::Type type =
            static_cast<QEvent::Type>(registerEventType());
        return type;
    }
};

#endif // PAGEEVENT_H
