#pragma once

// FocusEventFilter.hpp
#include <QObject>
#include <QPushButton>
class FocusEventFilter : public QObject {
    Q_OBJECT
public:
    FocusEventFilter(QObject* parent_)
      :QObject(parent_) {
    }
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};
