#include <algorithm>
#include <functional>

#include <QHBoxLayout>
#include <QKeySequence>
#include <QPushButton>
#include <QTimer>

#include <QDebug>

#include "app/arbiter.hpp"

#include "app/action.hpp"

#include "DashLog.hpp"

namespace {

const char *GPIO_CONSUMER = "dash";

// Buttons are assumed to be wired to ground with the internal pull-up
// enabled, so a press shows up as a falling edge.
gpiod::line_settings gpio_button_settings()
{
    gpiod::line_settings settings;
    settings.set_direction(gpiod::line::direction::INPUT)
        .set_edge_detection(gpiod::line::edge::BOTH)
        .set_bias(gpiod::line::bias::PULL_UP);
    return settings;
}

std::shared_ptr<gpiod::line_request> request_gpio_line(const GpioLine &line)
{
    gpiod::chip chip(line.chip_path);
    gpiod::line_request request = chip.prepare_request()
        .set_consumer(GPIO_CONSUMER)
        .add_line_settings(line.offset, gpio_button_settings())
        .do_request();
    return std::make_shared<gpiod::line_request>(std::move(request));
}

// "gpioN" keys are matched against the RPi devicetree line names ("GPIOx")
// first so this keeps working across chips (e.g. the Pi 5's RP1), falling
// back to offset N on gpiochip0 for boards without named lines.
bool find_gpio_line(int number, GpioLine &out)
{
    QString target = QString("GPIO%1").arg(number);

    for (auto &entry : std::filesystem::directory_iterator("/dev")) {
        if (!gpiod::is_gpiochip_device(entry.path()))
            continue;

        try {
            gpiod::chip chip(entry.path());
            std::size_t num_lines = chip.get_info().num_lines();
            for (unsigned int offset = 0; offset < num_lines; offset++) {
                gpiod::line_info info = chip.get_line_info(offset);
                if (QString::fromStdString(info.name()).compare(target, Qt::CaseInsensitive) == 0) {
                    out = GpioLine{entry.path(), offset, QString::fromStdString(info.name())};
                    return true;
                }
            }
        } catch (const std::exception &) {
            continue;
        }
    }

    try {
        std::filesystem::path chip_path("/dev/gpiochip0");
        gpiod::chip chip(chip_path);
        out = GpioLine{chip_path, static_cast<unsigned int>(number), QString()};
        return true;
    } catch (const std::exception &) {
        return false;
    }
}

QString gpio_key_for_line(const GpioLine &line)
{
    if (line.name.startsWith("GPIO", Qt::CaseInsensitive)) {
        bool ok = false;
        int number = line.name.mid(4).toInt(&ok);
        if (ok)
            return QString("gpio%1").arg(number);
    }
    return QString("gpio%1").arg(line.offset);
}

} // namespace

GPIONotifier::GPIONotifier()
    : QObject()
{
}

GPIONotifier::~GPIONotifier()
{
    this->disable();
}

void GPIONotifier::enable()
{
    if (!this->watches.isEmpty())
        return;

    for (auto &entry : std::filesystem::directory_iterator("/dev")) {
        if (!gpiod::is_gpiochip_device(entry.path()))
            continue;

        try {
            gpiod::chip chip(entry.path());
            std::size_t num_lines = chip.get_info().num_lines();
            for (unsigned int offset = 0; offset < num_lines; offset++) {
                gpiod::line_info info = chip.get_line_info(offset);
                QString name = QString::fromStdString(info.name());
                if (!name.startsWith("GPIO", Qt::CaseInsensitive) || info.used())
                    continue;

                GpioLine line{entry.path(), offset, name};
                std::shared_ptr<gpiod::line_request> request;
                try {
                    request = request_gpio_line(line);
                } catch (const std::exception &) {
                    continue;
                }

                auto *notifier = new QSocketNotifier(request->fd(), QSocketNotifier::Read, this);
                QString key = gpio_key_for_line(line);
                connect(notifier, &QSocketNotifier::activated, this, [this, request, key](int) {
                    gpiod::edge_event_buffer buffer;
                    request->read_edge_events(buffer);
                    emit triggered(key);
                });

                this->watches.push_back({request, notifier});
            }
        } catch (const std::exception &) {
            continue;
        }
    }
}

void GPIONotifier::disable()
{
    for (auto &watch : this->watches) {
        delete watch.notifier;
    }
    // Dropping the requests (last shared_ptr reference) releases the GPIO lines.
    this->watches.clear();
}

ActionDialog::ActionDialog(Arbiter &arbiter)
    : Dialog(arbiter, true, arbiter.window())
    , notifier()
{
    this->label = new QLabel();
    this->label->setProperty("add_hint", true);
    this->label->setFont(arbiter.forge().font(14, true));
    this->label->setAlignment(Qt::AlignCenter);
    this->set_body(this->label);

    connect(&this->notifier, &GPIONotifier::triggered, [this](QString gpio){ this->label->setText(gpio); });
}

void ActionDialog::keyPressEvent(QKeyEvent *event)
{
    static const auto mod_keys = {Qt::Key_unknown, Qt::Key_Control, Qt::Key_Shift, Qt::Key_Alt, Qt::Key_Meta};

    const auto key = static_cast<Qt::Key>(event->key());
    if (std::any_of(mod_keys.begin(), mod_keys.end(), [key](Qt::Key mod_key){ return (key == mod_key); }))
        return;

    this->label->setText(QKeySequence(event->modifiers() + key).toString());
}

void ActionDialog::showEvent(QShowEvent *event)
{
    ActionEventFilter::get_instance()->disable();
    this->notifier.enable();
    this->label->setText(QString());

    Dialog::showEvent(event);

    this->label->setFocus();
}

void ActionDialog::closeEvent(QCloseEvent *event)
{
    ActionEventFilter::get_instance()->enable();
    this->notifier.disable();

    Dialog::closeEvent(event);
}

Action::GPIO::GPIO()
    : request()
    , notifier(nullptr)
{
}

Action::GPIO::~GPIO()
{
    if (notifier)
        delete notifier;
    // Dropping the request (if any) releases the GPIO line.
}

Action::Action(QString name, std::function<void(ActionState)> action, QWidget *parent)
    : QObject(parent)
    , shortcut(parent)
    , gpio()
    , name_(name)
    , key_()
    , func_(action)
{
    connect(&this->shortcut, &QShortcut::activated, [action]{ action(ActionState::Triggered); });
}


bool ActionEventFilter::eventFilter(QObject* obj, QEvent* event)
{
    if (this->disabled || (this->eventFilterMap.count() == 0))
        return false;

    Action::ActionState state = Action::ActionState::Triggered;
    switch (event->type()) {
        case(QEvent::KeyPress):
            state = Action::ActionState::Activated;
            break;
        case(QEvent::KeyRelease):
            state = Action::ActionState::Deactivated;
            break;
        default:
            return false;
    }
    QKeyEvent* key = static_cast<QKeyEvent*>(event);
    if (!key->isAutoRepeat())
    {
        auto it = this->eventFilterMap.find(key->key());
        if (it != this->eventFilterMap.end())
        {
            std::lock_guard<decltype(mutex_)> lock(mutex_);
            (*it)->func_(state);
            return true;
        }
    }
    return false;
}


ActionEventFilter *ActionEventFilter::get_instance()
{
    static ActionEventFilter actionEventFilter;
    return &actionEventFilter;
}

void Action::set(QString key)
{
    this->shortcut.setKey(QKeySequence());

    ActionEventFilter* actionEventFilter = ActionEventFilter::get_instance();
    int k = actionEventFilter->eventFilterMap.key(this, -1);
    if(k > -1)
        actionEventFilter->eventFilterMap.remove(k);

    if (this->gpio.notifier) {
        delete this->gpio.notifier;
        this->gpio.notifier = nullptr;
    }
    this->gpio.request.reset();

    this->key_ = key;
    if (this->key_.startsWith("gpio")) {
        QString debugStr;
        QDebug stream(&debugStr);
        stream << "[Action] " << this->key_ << ": setting action as gpio"; // temp
        DASH_LOG(info) << debugStr.toStdString();

        bool ok = false;
        int number = this->key_.mid(4).toInt(&ok);

        GpioLine line;
        if (ok && find_gpio_line(number, line)) {
            try {
                this->gpio.request = request_gpio_line(line);

                auto request = this->gpio.request;
                this->gpio.notifier = new QSocketNotifier(request->fd(), QSocketNotifier::Read, this);
                connect(this->gpio.notifier, &QSocketNotifier::activated, this, [this, request](int){
                    gpiod::edge_event_buffer buffer;
                    request->read_edge_events(buffer);
                    this->gpio.notifier->setEnabled(false);
                    this->func_(ActionState::Triggered);
                    QTimer::singleShot(300, this, [this]{
                        if (this->gpio.notifier)
                            this->gpio.notifier->setEnabled(true);
                    });
                });
            } catch (const std::exception &e) {
                QString debugStr;
                QDebug stream(&debugStr);
                stream << "[Action]" << this->key_ << ": failed to request gpio line -" << e.what(); // temp
                DASH_LOG(info) << debugStr.toStdString();
            }
        }
        else {
            QString debugStr;
            QDebug stream(&debugStr);
            stream << "[Action]" << this->key_ << ": gpio line not found"; // temp
            DASH_LOG(info) << debugStr.toStdString();
        }
    }
    else if (!this->key_.isNull()) {
        QString debugStr;
        QDebug stream(&debugStr);
        stream << "[Action]" << this->key_ << ": setting action as key"; // temp
        DASH_LOG(info) << debugStr.toStdString();
        // if + is present, it means sequence is multiple keys.
        // I'd have implemented this by looking at the .count() of the QKeySequence
        // but it doesn't consider modifiers (ctrl, shift, etc) to be a discrete key
        if(!this->key_.contains('+')) 
        {
            QString debugStr;
            QDebug stream(&debugStr);
            stream << "[Action]" << this->key_ << ": single key sequence, setting via eventFilter";
            DASH_LOG(info) << debugStr.toStdString();
            actionEventFilter->eventFilterMap.insert(QKeySequence::fromString(this->key_)[0], this);
        }
        else
        {
            this->shortcut.setKey(QKeySequence::fromString(this->key_));
        }
    }
}
