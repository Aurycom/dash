#pragma once

#include <QLabel>
#include <QWidget>

#include "app/pages/page.hpp"

class Arbiter;

// TEMPORARY Qt6 migration stub: the real implementation depends on Qt5GStreamer
// (QGst::), which has no known Qt6 port, plus the old Qt5 QCamera/QCameraViewfinder
// API removed in Qt6. It is disabled here so the rest of the app can be ported to
// Qt6 first; the backup camera feature will be reworked and restored afterwards.
class CameraPage : public QWidget, public Page {
    Q_OBJECT

   public:
    CameraPage(Arbiter &arbiter, QWidget *parent = nullptr);

    void init() override;

   private:
    QLabel *status;
};
