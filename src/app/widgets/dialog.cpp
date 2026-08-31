#include <math.h>
#include <QFrame>
#include <QGridLayout>
#include <QKeyEvent>
#include <QPropertyAnimation>
#include <QRect>

#include "app/arbiter.hpp"
#include "app/config.hpp"
#include "app/widgets/dialog.hpp"

Dialog::Dialog(Arbiter &arbiter, bool fullscreen, QWidget *parent)
    : QWidget(arbiter.window())
    , arbiter(arbiter)
{
    this->setAttribute(Qt::WA_TranslucentBackground, true);
    // A child widget that's never been explicitly shown/hidden isn't
    // exempt from its parent's own show() cascading down to it - since
    // it's added directly under the main window rather than into a layout
    // that only gets populated once needed, it would otherwise pop up as
    // soon as MainWindow::show() runs, well before any open() call.
    this->hide();

    this->anchor = parent;
    this->fullscreen = fullscreen;
    this->backdrop = nullptr;
    if (this->fullscreen) {
        // Parented alongside this dialog (not as its child) so raising the
        // two of them independently keeps the backdrop behind the dialog's
        // own content while still being above the rest of the window.
        this->backdrop = new QWidget(this->arbiter.window());
        this->backdrop->setAttribute(Qt::WA_TranslucentBackground, true);
        this->backdrop->hide();
    }

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(this->content_widget());

    this->timer = new QTimer(this);
    this->timer->setSingleShot(true);
    connect(this->timer, &QTimer::timeout, [this]() { this->close(); });

    this->installEventFilter(this);
}

void Dialog::open(int timeout)
{
    // This widget isn't managed by a parent layout (it's a manually
    // positioned overlay), so its size has to be computed explicitly rather
    // than relying on the auto-resize-to-sizeHint a top-level QDialog would
    // otherwise get for free on show().
    this->adjustSize();

    if (this->backdrop) {
        this->backdrop->setGeometry(this->arbiter.window()->rect());
        this->backdrop->raise();
        this->backdrop->show();
    }

    this->show();
    this->raise();
    this->setFocus();

    if (timeout > 0)
        this->timer->start(timeout);
}

void Dialog::set_title(QString str)
{
    QLabel *label = new QLabel(str, this);
    QFont font(this->arbiter.forge().font(16));
    font.setBold(true);
    label->setFont(font);
    this->title->addWidget(label);
}

void Dialog::set_body(QWidget *widget)
{
    if (this->fullscreen) {
        QScrollArea *scroll_area = new QScrollArea(this);
        Session::Forge::to_touch_scroller(scroll_area);
        scroll_area->setWidgetResizable(true);
        scroll_area->setWidget(widget);

        this->body->addWidget(scroll_area);
    }
    else {
        this->body->addWidget(widget);
    }
}

void Dialog::set_button(QPushButton *button)
{
    if (this->buttons->count() == 0)
        this->add_cancel_button();
    button->setFlat(true);
    this->buttons->addWidget(button, 0, Qt::AlignRight);
    connect(button, &QPushButton::clicked, [this]() { this->close(); });
}

QWidget *Dialog::content_widget()
{
    QFrame *frame = new QFrame(this);
    QVBoxLayout *layout = new QVBoxLayout(frame);

    this->title = new QVBoxLayout();
    this->title->setContentsMargins(0, 0, 0, 0);
    this->title->setSpacing(0);
    layout->addLayout(this->title);

    this->body = new QVBoxLayout();
    this->body->setContentsMargins(0, 0, 0, 0);
    this->body->setSpacing(0);
    layout->addLayout(this->body);

    this->buttons = new QHBoxLayout();
    this->buttons->setContentsMargins(0, 0, 0, 0);
    this->buttons->setSpacing(0);
    if (this->fullscreen)
        this->add_cancel_button();
    layout->addLayout(this->buttons);

    return frame;
}

void Dialog::set_position()
{
    QWidget *window = this->arbiter.window();
    if (!this->anchor || !window)
        return;

    // This dialog's Qt parent is always the main window (see constructor),
    // so this is the coordinate space move() below actually needs.
    QPoint anchor_center = window->mapFromGlobal(this->anchor->mapToGlobal(this->anchor->rect().center()));

    QPoint point;
    if (this->fullscreen) {
        point = anchor_center - this->rect().center();
    }
    else {
        QPoint window_center = window->rect().center();

        int offset = std::ceil(4 * this->arbiter.layout().scale);

        QPoint pivot;
        if (anchor_center.y() > window_center.y()) {
            pivot = (anchor_center.x() > window_center.x()) ? this->rect().bottomRight() : this->rect().bottomLeft();
            pivot.ry() += (this->anchor->height() / 2) + offset;
        }
        else {
            pivot = (anchor_center.x() > window_center.x()) ? this->rect().topRight() : this->rect().topLeft();
            pivot.ry() -= (this->anchor->height() / 2) + offset;
        }
        if (anchor_center.x() > window_center.x())
            pivot.rx() -= this->width() / 2;
        else
            pivot.rx() += this->width() / 2;
        point = anchor_center - pivot;
    }
    this->move(point);
}

void Dialog::keyPressEvent(QKeyEvent *event)
{
    // Popovers (non-fullscreen) dismiss via their timeout/outside click
    // instead, not Escape.
    if (event->key() == Qt::Key_Escape) {
        if (this->fullscreen)
            this->close();
    }
    else {
        QWidget::keyPressEvent(event);
    }
}

void Dialog::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    if (this->fullscreen) {
        if (QWidget *window = this->arbiter.window()) {
            int margin = std::ceil(48 * this->arbiter.layout().scale) * 2;
            this->setFixedWidth(std::min(this->width(), window->width() - margin));
            this->setFixedHeight(std::min(this->height(), window->height() - margin));
        }
    }

    this->set_position();
}

void Dialog::closeEvent(QCloseEvent *)
{
    if (this->backdrop)
        this->backdrop->hide();

    // On Raspberry Pi's, sometimes LXPanel will grab focus after a dialog closes
    // Focus should be returned to main dash window instead, so that shortcuts work
    // and that dash remains fullscreen
    //
    // Tells the main window to grab focus
    this->arbiter.window()->activateWindow();
}

bool Dialog::eventFilter(QObject *object, QEvent *event)
{
    // restart timer on any event
    if (this->timer->isActive())
        this->timer->start(this->timer->interval());

    return QWidget::eventFilter(object, event);
}

SnackBar::SnackBar(Arbiter &arbiter)
    : Dialog(arbiter, false, this->get_ref())
{
    this->setFixedHeight(64 * this->arbiter.layout().scale);
}

void SnackBar::resizeEvent(QResizeEvent* event)
{
    // its possible the ref didnt exist when the anchor was originally set
    if (!this->anchor)
        this->anchor = this->get_ref();

    if (this->anchor)
        this->setFixedWidth(this->anchor->width() * (2 / 3.0));

    Dialog::resizeEvent(event);
}

void SnackBar::mousePressEvent(QMouseEvent *event)
{
    this->close();
}

QWidget *SnackBar::get_ref()
{
    for (QWidget *widget : qApp->allWidgets()) {
        if (widget->objectName() == "MsgRef")
            return widget;
    }
    return nullptr;
}
