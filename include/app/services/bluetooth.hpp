#pragma once

#include <BluezQt/Adapter>
#include <BluezQt/Agent>
#include <BluezQt/Device>
#include <BluezQt/MediaPlayer>
#include <BluezQt/MediaPlayerTrack>
#include <QLabel>
#include <QList>
#include <QMainWindow>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QPushButton>
#include <QString>

#include "app/widgets/dialog.hpp"
#include "app/widgets/progress.hpp"

class Arbiter;

// Shows the pairing passkey and lets it be confirmed/cancelled with a tap,
// since there's a touchscreen but no keyboard to type a PIN on.
class BluetoothDialog : public Dialog {
    Q_OBJECT

   public:
    BluetoothDialog(Arbiter &arbiter);

    void set_passkey(QString device_name, QString passkey);

   protected:
    void closeEvent(QCloseEvent *event) override;

   private:
    QLabel *name_label;
    QLabel *label;
    bool confirmed_ = false;

   signals:
    void confirmed();
    void cancelled();
};

// Displays the passkey on screen for confirmation instead of auto-accepting,
// so pairing can't be silently forced from a phone with nothing shown here.
class BluetoothAgent : public BluezQt::Agent {
   public:
    explicit BluetoothAgent(Arbiter &arbiter, QObject *parent = nullptr);

    QDBusObjectPath objectPath() const override;
    Capability capability() const override;

    void requestConfirmation(BluezQt::DevicePtr device, const QString &passkey, const BluezQt::Request<> &request) override;
    void requestAuthorization(BluezQt::DevicePtr device, const BluezQt::Request<> &request) override;
    void authorizeService(BluezQt::DevicePtr device, const QString &uuid, const BluezQt::Request<> &request) override;

   private:
    Arbiter &arbiter;
};

class Bluetooth : public QObject {
    Q_OBJECT

   public:
    Bluetooth(Arbiter &arbiter);

    void start_scan();
    void stop_scan();
    void toggle_device(BluezQt::DevicePtr device) const;

    inline QList<BluezQt::DevicePtr> get_devices()
    {
        return this->has_adapter() ? this->adapter->devices() : QList<BluezQt::DevicePtr>();
    }
    inline QPair<QString, BluezQt::MediaPlayerPtr> get_media_player()
    {
        if (this->media_player_device != nullptr)
            return {this->media_player_device->name(), this->media_player_device->mediaPlayer()};

        return {QString(), QSharedPointer<BluezQt::MediaPlayer>(nullptr)};
    }
    inline bool has_adapter() { return this->adapter != nullptr; }

   private:
    void update_media_player(BluezQt::DevicePtr device);

    BluezQt::AdapterPtr adapter;
    BluezQt::DevicePtr media_player_device;
    QTimer *scan_timer;

   signals:
    void init();
    void device_added(BluezQt::DevicePtr);
    void device_changed(BluezQt::DevicePtr);
    void device_removed(BluezQt::DevicePtr);
    void media_player_changed(QString, BluezQt::MediaPlayerPtr);
    void media_player_status_changed(BluezQt::MediaPlayer::Status);
    void media_player_track_changed(BluezQt::MediaPlayerTrack);
    void scan_status(bool);
};

