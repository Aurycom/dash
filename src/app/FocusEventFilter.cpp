// FocusEventFilter.cpp
#include <iostream>
#include "app/FocusEventFilter.hpp"
#include <QFocusEvent>
#include <QMouseEvent>
#include <QWidget>
#include <QStyle>
#include <QApplication>
#include <QDebug>

bool FocusEventFilter::eventFilter(QObject *obj, QEvent *event) {
    QWidget *widget = qobject_cast<QWidget*>(obj);
    if (widget) {
        // On positionne le autodefault
        if(event->type() == QEvent::ChildAdded){
            QChildEvent *ce = static_cast<QChildEvent*>(event);
            if(auto btn = qobject_cast<QPushButton*>(ce->child())){
                qDebug()<< "ajout bouton détecté";
                btn->setAutoDefault(true);
            }
        }else if (event->type() == QEvent::FocusIn) {
            qDebug()<< "event FocusIn";
            QFocusEvent *focusEvent = static_cast<QFocusEvent*>(event);
            if (focusEvent->reason() == Qt::TabFocusReason || focusEvent->reason() == Qt::BacktabFocusReason) {
                qDebug()<< "set focusByTab to true";
                widget->setProperty("focusByTab", true);
                widget->style()->polish(widget);
            }
        } else if (event->type() == QEvent::FocusOut) {
            widget->setProperty("focusByTab", false);
            widget->style()->polish(widget);
        } else if (event->type() == QEvent::MouseButtonPress) {
            widget->setProperty("focusByTab", false);
            widget->style()->polish(widget);
        }else if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

            QString readable = QKeySequence(keyEvent->key()).toString();
            QString rawText = keyEvent->text();
            qDebug() << "1 - Key code =" << keyEvent->key()
             << ", readable =" << readable
             << ", rawText =" << rawText ;
        }
    }else if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        QString readable = QKeySequence(keyEvent->key()).toString();
        QString rawText = keyEvent->text();
        qDebug() << "2 - Key code =" << keyEvent->key()
         << ", readable =" << readable
         << ", rawText =" << rawText ;
        qDebug() << "Enter pressed on "<<obj<<obj->metaObject()->className();
        QWidget *focused= QApplication::focusWidget();
        qDebug() << "Focused widget "<< focused;

        QWidget *w = QApplication::focusWidget();
if (w && (keyEvent->key()==Qt::Key_Return || keyEvent->key()==Qt::Key_Enter)) {
    // Crée un KeyPress et KeyRelease pour Enter
    QKeyEvent press(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
    QKeyEvent release(QEvent::KeyRelease, Qt::Key_Space, Qt::NoModifier);

    // Envoie l'événement au widget focalisé
    QCoreApplication::sendEvent(w, &press);
    QCoreApplication::sendEvent(w, &release);
}

    }
    return QObject::eventFilter(obj, event);
}
