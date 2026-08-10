#include <unistd.h>
#include <BluezQt/Adapter>
#include <BluezQt/Device>
#include <BluezQt/InitManagerJob>
#include <BluezQt/Manager>
#include <BluezQt/PendingCall>
#include <BluezQt/MediaPlayer>
#include <BluezQt/MediaPlayerTrack>
#include <QAbstractSlider>
#include <QApplication>
#include <QBluetoothAddress>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QBluetoothLocalDevice>
#include <QBluetoothServiceDiscoveryAgent>
#include <QBluetoothServiceInfo>
#include <QTimer>

#include "DashLog.hpp"

#include "app/widgets/progress.hpp"
#include "app/arbiter.hpp"

#include "app/services/bluetooth.hpp"

BluetoothAgent::BluetoothAgent(QObject *parent)
    : BluezQt::Agent(parent)
{
}

QDBusObjectPath BluetoothAgent::objectPath() const
{
    return QDBusObjectPath(QStringLiteral("/dash/bluetooth/agent"));
}

BluetoothAgent::Capability BluetoothAgent::capability() const
{
    // No screen/keyboard prompt to confirm a passkey on, so pair using the
    // "just works" model and auto-accept authorization requests below.
    return BluezQt::Agent::NoInputNoOutput;
}

void BluetoothAgent::requestConfirmation(BluezQt::DevicePtr device, const QString &passkey, const BluezQt::Request<> &request)
{
    DASH_LOG(info) << "[Bluetooth] Confirming pairing with " << device->name().toStdString() << " (passkey " << passkey.toStdString() << ")";
    request.accept();
}

void BluetoothAgent::requestAuthorization(BluezQt::DevicePtr device, const BluezQt::Request<> &request)
{
    DASH_LOG(info) << "[Bluetooth] Authorizing pairing with " << device->name().toStdString();
    request.accept();
}

void BluetoothAgent::authorizeService(BluezQt::DevicePtr device, const QString &uuid, const BluezQt::Request<> &request)
{
    DASH_LOG(info) << "[Bluetooth] Authorizing service " << uuid.toStdString() << " for " << device->name().toStdString();
    request.accept();
}

Bluetooth::Bluetooth(Arbiter &arbiter)
    : QObject(qApp)
{
    DASH_LOG(info) << "[Bluetooth] Init";

    // Setup the scan timeout
    this->scan_timer = new QTimer(this);
    this->scan_timer->setSingleShot(true);
    connect(this->scan_timer, &QTimer::timeout, [this]{ this->stop_scan(); });

    BluezQt::Manager *manager = new BluezQt::Manager();
    BluezQt::InitManagerJob *job = manager->init();

    // Run the job with start() so we don't block this thread
    job->start();
    connect(job, &BluezQt::InitManagerJob::result, [this, manager]{
        DASH_LOG(info) << "[Bluetooth] Init complete!";

        // Without a registered agent, BlueZ has nothing to ask when a phone
        // pairs from the app UI and the request just times out - pairing
        // only appeared to work from `bluetoothctl` because it registers
        // its own agent for the duration of the CLI session.
        auto *agent = new BluetoothAgent(manager);
        manager->registerAgent(agent);
        manager->requestDefaultAgent(agent);

        this->adapter = manager->usableAdapter();
        if (this->has_adapter()) {
            for (auto device : this->get_devices()) {
                if (device->mediaPlayer() != nullptr) {
                    this->media_player_device = device;
                    break;
                }
            }

            connect(this->adapter.data(), &BluezQt::Adapter::deviceAdded, [this](BluezQt::DevicePtr device){
                emit device_added(device);
            });
            connect(this->adapter.data(), &BluezQt::Adapter::deviceChanged, [this](BluezQt::DevicePtr device) {
                emit device_changed(device);
                this->update_media_player(device);
            });
            connect(this->adapter.data(), &BluezQt::Adapter::deviceRemoved, [this](BluezQt::DevicePtr device){
                emit device_removed(device);
            });
        }

        DASH_LOG(info) << "[Bluetooth] Has Adapter: " << this->has_adapter() << ", Has Media Device: " << (this->media_player_device != nullptr);

        emit init();
    });
}

void Bluetooth::start_scan()
{
    if (this->has_adapter()) {
        if (!this->adapter->isDiscovering()) {
            emit scan_status(true);
            this->adapter->startDiscovery();
            this->scan_timer->start(15000);
        }
    }
}

void Bluetooth::stop_scan()
{
    if (this->has_adapter()) {
        if (this->adapter->isDiscovering()) {
            emit scan_status(false);
            this->adapter->stopDiscovery();
        }
    }
}

void Bluetooth::toggle_device(BluezQt::DevicePtr device) const
{
    // Calls are async and don't block the UI thread: pairing especially can
    // take several seconds, and blocking here (as with the old
    // waitForFinished() calls) froze the whole app for that duration.
    if (device->isConnected()) {
        BluezQt::PendingCall *call = device->disconnectFromDevice();
        connect(call, &BluezQt::PendingCall::finished, [device](BluezQt::PendingCall *call){
            if (call->error() != BluezQt::PendingCall::NoError)
                DASH_LOG(error) << "[Bluetooth] Failed to disconnect from " << device->name().toStdString() << ": " << call->errorText().toStdString();
        });
        return;
    }

    auto connect_to_device = [device]{
        BluezQt::PendingCall *call = device->connectToDevice();
        connect(call, &BluezQt::PendingCall::finished, [device](BluezQt::PendingCall *call){
            if (call->error() != BluezQt::PendingCall::NoError)
                DASH_LOG(error) << "[Bluetooth] Failed to connect to " << device->name().toStdString() << ": " << call->errorText().toStdString();
        });
    };

    if (device->isPaired()) {
        connect_to_device();
        return;
    }

    // Mirror the `bluetoothctl` flow (pair, then trust, then connect) rather
    // than relying on connectToDevice() to pair on its own.
    BluezQt::PendingCall *pair_call = device->pair();
    connect(pair_call, &BluezQt::PendingCall::finished, [device, connect_to_device](BluezQt::PendingCall *call){
        if (call->error() != BluezQt::PendingCall::NoError) {
            DASH_LOG(error) << "[Bluetooth] Failed to pair with " << device->name().toStdString() << ": " << call->errorText().toStdString();
            return;
        }

        if (!device->isTrusted())
            device->setTrusted(true);

        connect_to_device();
    });
}

void Bluetooth::update_media_player(BluezQt::DevicePtr device)
{
    if (device->mediaPlayer() != nullptr) {
        emit media_player_status_changed(device->mediaPlayer()->status());
        emit media_player_track_changed(device->mediaPlayer()->track());
        emit media_player_changed(device->name(), device->mediaPlayer());
        this->media_player_device = device;
    }
    else if (this->media_player_device == device) {
        emit media_player_status_changed(BluezQt::MediaPlayer::Status::Paused);
        emit media_player_track_changed(BluezQt::MediaPlayerTrack());
        emit media_player_changed(QString(), QSharedPointer<BluezQt::MediaPlayer>(nullptr));
    }
}
