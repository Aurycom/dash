#include <QVBoxLayout>

#include "app/pages/camera.hpp"

CameraPage::CameraPage(Arbiter &arbiter, QWidget *parent)
    : QWidget(parent)
    , Page(arbiter, "Camera", "camera", true, this)
{
}

void CameraPage::init()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    this->status = new QLabel("Backup camera is temporarily unavailable\n(pending Qt6 port)", this);
    this->status->setAlignment(Qt::AlignCenter);
    layout->addWidget(this->status);
}
