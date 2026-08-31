#pragma once

#include <QApplication>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>


class Arbiter;

// A child widget overlaid on the main window, instead of a separate
// top-level window. Wayland gives clients no control over where their own
// top-level window is placed (only X11 does), so a movable/positionable
// QDialog silently ends up wherever the compositor feels like putting it.
// Positioning a child widget within its parent is plain Qt geometry, not a
// windowing-system operation, so it works the same on both. This also means
// it's never a separately draggable window - it always stays put, embedded
// in and clipped to the main window.
class Dialog : public QWidget {
    Q_OBJECT

   public:
    Dialog(Arbiter &arbiter, bool fullscreen, QWidget *parent = nullptr);
    void open(int timeout = 0);

    void set_title(QString str);
    void set_body(QWidget *widget);
    void set_button(QPushButton *button);

   protected:
    Arbiter &arbiter;
    // Widget this dialog is positioned relative to: centered on it when
    // fullscreen (modal), anchored beside it (like a popover) otherwise.
    // Not the dialog's actual Qt parent - that's always the main window, so
    // that raise() can stack this above every page/quick view regardless of
    // which widget it's anchored to.
    QWidget *anchor;

    void showEvent(QShowEvent *event);
    void keyPressEvent(QKeyEvent *event);
    bool eventFilter(QObject *object, QEvent *event);
    void closeEvent(QCloseEvent *event) override;

   private:
    QVBoxLayout *title;
    QVBoxLayout *body;
    QHBoxLayout *buttons;
    QTimer *timer;
    bool fullscreen;
    // Full-window click-catcher shown behind fullscreen (modal) dialogs, so
    // the rest of the main window can't be interacted with while one is
    // open - replaces the blocking a real modal top-level window used to
    // get from the window manager for free. Null for non-modal dialogs.
    QWidget *backdrop;

    QWidget *content_widget();
    void set_position();

    inline void add_cancel_button()
    {
        this->buttons->addStretch();

        QPushButton *button = new QPushButton("cancel", this);
        button->setFlat(true);
        connect(button, &QPushButton::clicked, [this]() { this->close(); });
        this->buttons->addWidget(button, 0, Qt::AlignRight);
    }
};

class SnackBar : public Dialog {
    Q_OBJECT

   public:
    SnackBar(Arbiter &arbiter);

    void resizeEvent(QResizeEvent* event);

   protected:
    void mousePressEvent(QMouseEvent *event) override;

   private:
    QWidget *get_ref();
};
