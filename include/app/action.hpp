#pragma once

#include <QCloseEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QList>
#include <QObject>
#include <QShortcut>
#include <QSocketNotifier>
#include <QString>
#include <QShowEvent>
#include <QWidget>
#include <bits/stdc++.h>

#include <gpiod.hpp>

#include "app/widgets/dialog.hpp"

class Arbiter;

class GPIONotifier : public QObject {
    Q_OBJECT

   public:
    GPIONotifier();
    ~GPIONotifier();

    void enable();
    void disable();

   private:
    struct Watch {
        gpiod::line line;
        QSocketNotifier *notifier;
    };

    QList<Watch> watches;

   signals:
    void triggered(QString gpio);
};

class ActionDialog : public Dialog {
    Q_OBJECT

   public:
    ActionDialog(Arbiter &arbiter);

    QString key() { return label->text(); }

   protected:
    void keyPressEvent(QKeyEvent *event);
    void showEvent(QShowEvent *event);
    void closeEvent(QCloseEvent *event);

   private:
    GPIONotifier notifier;
    QLabel *label;
};

class Action : public QObject {
    Q_OBJECT

   public:
    enum ActionState
    {
        Activated,
        Deactivated,
        Triggered
    };
    Action(QString name, std::function<void(ActionState)> func, QWidget *parent);
    void set(QString key);

    QString key() const { return this->key_; }
    QString name() const { return this->name_; }
    std::function<void(ActionState)> func_;

   private:
    struct GPIO {
        gpiod::line line;
        QSocketNotifier *notifier;
        bool requested;

        GPIO();
        ~GPIO();
    };

    QShortcut shortcut;
    GPIO gpio;

    QString name_;
    QString key_;
};

class ActionEventFilter : public QObject
{
    Q_OBJECT

   public:
    ActionEventFilter(){};
    bool eventFilter(QObject* obj, QEvent* event);
    static ActionEventFilter *get_instance();
    QMap<int, Action*> eventFilterMap;

    void enable() { this->disabled = false; }
    void disable() { this->disabled = true; }

   private:
    std::mutex mutex_;
    bool disabled = false;
};


