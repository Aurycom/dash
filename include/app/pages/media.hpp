#pragma once

#include <QFileInfo>
#include <QList>
#include <QMap>
#include <QMediaPlayer>
#include <QPluginLoader>
#include <QString>
#include <QUrl>
#include <QtWidgets>

#include "app/config.hpp"
#include "app/pages/page.hpp"
#include "app/widgets/selector.hpp"
#include "app/widgets/tuner.hpp"

class Arbiter;

// Minimal replacement for QMediaPlaylist, removed in Qt6. Only implements
// the always-looping behavior LocalPlayerTab relies on.
class MediaPlaylist : public QObject {
    Q_OBJECT

   public:
    MediaPlaylist(QMediaPlayer *player, QObject *parent = nullptr);

    bool addMedia(const QUrl &url);
    void clear();

    int currentIndex() const { return this->current_index; }
    void setCurrentIndex(int index);

    void next();
    void previous();

   signals:
    void currentIndexChanged(int index);

   private:
    QMediaPlayer *player;
    QList<QUrl> tracks;
    int current_index = -1;
};

class MediaPage : public QTabWidget, public Page {
    Q_OBJECT

   public:
    MediaPage(Arbiter &arbiter, QWidget *parent = nullptr);

    void init() override;
};

class BluetoothPlayerTab : public QWidget {
    Q_OBJECT

   public:
    BluetoothPlayerTab(Arbiter &arbiter, QWidget *parent = nullptr);

   private:
    Arbiter &arbiter;

    QWidget *track_widget();
    QWidget *controls_widget();
};

class RadioPlayerTab : public QWidget {
    Q_OBJECT

   public:
    RadioPlayerTab(Arbiter &arbiter, QWidget *parent = nullptr);
    ~RadioPlayerTab();

   private:
    static QMap<QString, QFileInfo> get_plugins();

    Arbiter &arbiter;
    Config *config;
    QMap<QString, QFileInfo> plugins;
    QPluginLoader loader;
    Tuner *tuner;
    Selector *plugin_selector;
    QPushButton *play_button;

    void load_plugin();
    QWidget *dialog_body();
    QWidget *tuner_widget();
    QWidget *controls_widget();

};

class LocalPlayerTab : public QWidget {
    Q_OBJECT

   public:
    LocalPlayerTab(Arbiter &arbiter, QWidget *parent = nullptr);

    static QString durationFmt(int total_ms);

   private:
    Arbiter &arbiter;

    QWidget *playlist_widget();
    QWidget *seek_widget();
    QWidget *controls_widget();
    void populate_dirs(QString path, QListWidget *dirs_widget);
    void populate_tracks(QString path, QListWidget *tracks_widget);

    Config *config;
    QMediaPlayer *player;
    MediaPlaylist *playlist;
    QLabel *path_label;
};
