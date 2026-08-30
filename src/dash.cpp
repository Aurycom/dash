#include <QApplication>
#include <QElapsedTimer>
#include <QPainter>
#include <QStringList>
#include <QThread>
#include <QWindow>

#include "app/window.hpp"
#include "app/action.hpp"

int main(int argc, char *argv[])
{
    QApplication dash(argc, argv);

    dash.setOrganizationName("openDsh");
    dash.setApplicationName("dash");
    dash.installEventFilter(ActionEventFilter::get_instance());

    QSize size = dash.primaryScreen()->size();
    QPoint pos = dash.primaryScreen()->geometry().topLeft();
    bool fullscreen = true;

    QSettings settings;
    DASH_LOG(info) << "loaded config: " << settings.fileName().toStdString();

    QStringList args = dash.arguments();
    if (args.size() > 2) {
        size = QSize(args.at(1).toInt(), args.at(2).toInt());
        if (args.size() > 4)
            pos = QPoint(args.at(3).toInt(), args.at(4).toInt());
        fullscreen = false;
    }
    else {
        settings.beginGroup("Window");
        if (settings.contains("size")) {
            size = settings.value("size").toSize();
            if (settings.contains("pos"))
                pos = settings.value("pos").toPoint();
            fullscreen = false;
        }
    }

    // Wayland gives clients no control over their window's position (only
    // X11 does), so centering the splash on screen via QSplashScreen::move()
    // silently does nothing there. Bake the centered logo into a
    // screen-sized opaque pixmap instead, so centering comes from the
    // drawing itself rather than window placement - this also removes the
    // old transparency mask, whose transparent (unpainted) majority let
    // weston's black shell background show through as a flash before the
    // logo area was painted.
    QPixmap logo(QPixmap(":/splash.png").scaledToHeight(size.height() / 2));
    QPixmap pixmap(size);
    pixmap.fill(Qt::black);
    QPainter painter(&pixmap);
    painter.drawPixmap((size.width() - logo.width()) / 2, (size.height() - logo.height()) / 2, logo);
    painter.end();

    QSplashScreen splash(pixmap);
    splash.move(pos);
    splash.show();
    // A single processEvents() isn't enough on Wayland: the first frame
    // needs a full create-surface/attach/commit/configure round trip with
    // the compositor before it's actually mapped, and MainWindow's
    // constructor below does synchronous work (GStreamer/GL pipeline setup)
    // without yielding back to the event loop, so the compositor may never
    // get a chance to show this splash otherwise. Pump events until the
    // splash window is actually exposed, bounded so a slow/absent
    // compositor can't hang startup.
    QElapsedTimer splashTimer;
    splashTimer.start();
    while ((!splash.windowHandle() || !splash.windowHandle()->isExposed()) &&
           splashTimer.elapsed() < 500) {
        dash.processEvents();
        QThread::msleep(10);
    }

    MainWindow window(QRect(pos, size));
    window.setWindowIcon(QIcon(":/logo.png"));
    window.setWindowFlags(Qt::FramelessWindowHint);
    if (fullscreen)
        window.setWindowState(Qt::WindowFullScreen);

    window.show();
    splash.finish(&window);
    window.activateWindow();

    return dash.exec();
}
