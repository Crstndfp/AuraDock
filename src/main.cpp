#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QIcon>
#include <LayerShellQt/Window>

#include "application/DockController.hpp"
#include "infrastructure/xdg/XdgDesktopRepository.hpp"
#include "infrastructure/xdg/ThemeIconProvider.hpp"
#include "infrastructure/trackers/KWinWaylandTracker.hpp"
#include "infrastructure/wayland/WaylandSurfaceMaskSynchronizer.hpp"

int main(int argc, char *argv[]) {
    // Enable Wayland integration
    qputenv("QT_QPA_PLATFORM", "wayland;xcb");

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("cachy-dock"));
    app.setOrganizationName(QStringLiteral("cachyos"));
    app.setOrganizationDomain(QStringLiteral("cachyos.org"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("preferences-system-windows")));

    auto dockController = std::make_unique<cachy_dock::application::DockController>();
    cachy_dock::infrastructure::xdg::XdgDesktopRepository xdgRepo;
    auto kwinTracker = std::make_unique<cachy_dock::infrastructure::trackers::KWinWaylandTracker>();

    // Populate initial items only if no configuration exists yet
    if (!dockController->hasSavedItems()) {
        auto initialItems = xdgRepo.getDefaultPinnedApps();
        dockController->setItems(std::move(initialItems));
    }

    // Connect and start native Wayland window tracking
    dockController->setWindowTracker(kwinTracker.get());
    kwinTracker->start();

    // Handle launcher activation
    QObject::connect(dockController.get(), &cachy_dock::application::DockController::itemActivated,
                     [&xdgRepo](const QString &id) {
        qDebug() << "[CachyDock] Launching item:" << id;
        xdgRepo.launchDesktopId(id.toStdString(), false);
    });

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("icon"), new cachy_dock::infrastructure::xdg::ThemeIconProvider());
    engine.rootContext()->setContextProperty(QStringLiteral("dockController"), dockController.get());

    // Locate MainDock.qml
    QString qmlPath = QStringLiteral(CACHY_DOCK_QML_DIR "/MainDock.qml");
    if (!QFileInfo::exists(qmlPath)) {
        qmlPath = QDir::current().filePath(QStringLiteral("src/ui/qml/MainDock.qml"));
    }

    qDebug() << "[CachyDock] Loading QML from:" << qmlPath;

    std::unique_ptr<cachy_dock::infrastructure::wayland::WaylandSurfaceMaskSynchronizer> maskSynchronizer;

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [&maskSynchronizer, dockController = dockController.get()](QObject *obj, const QUrl &objUrl) {
        if (!obj) {
            qCritical() << "[CachyDock] Failed to create QML root object from:" << objUrl;
            QCoreApplication::exit(-1);
            return;
        }

        auto *window = qobject_cast<QQuickWindow*>(obj);
        if (window) {
            auto *lWindow = LayerShellQt::Window::get(window);
            if (lWindow) {
                lWindow->setScope(QStringLiteral("cachy-dock"));
                lWindow->setLayer(LayerShellQt::Window::LayerTop);
                LayerShellQt::Window::Anchors anchors;
                anchors.setFlag(LayerShellQt::Window::AnchorBottom);
                anchors.setFlag(LayerShellQt::Window::AnchorLeft);
                anchors.setFlag(LayerShellQt::Window::AnchorRight);
                lWindow->setAnchors(anchors);
                lWindow->setExclusiveZone(static_cast<int32_t>(dockController->exclusiveZone()));
                lWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);

                QObject::connect(dockController, &cachy_dock::application::DockController::autoHideStateChanged,
                                 window, [lWindow, dockController]() {
                    lWindow->setExclusiveZone(static_cast<int32_t>(dockController->exclusiveZone()));
                });
                qDebug() << "[CachyDock] LayerShell window configured on LayerTop.";
            }

            maskSynchronizer = std::make_unique<cachy_dock::infrastructure::wayland::WaylandSurfaceMaskSynchronizer>(window);
            QObject::connect(dockController, &cachy_dock::application::DockController::maskUpdateRequired,
                             maskSynchronizer.get(), &cachy_dock::infrastructure::wayland::WaylandSurfaceMaskSynchronizer::updateSurfaceMasks);

            window->setVisible(true);
            window->show();
            qDebug() << "[CachyDock] Window shown. Geometry:" << window->geometry() << "Visible:" << window->isVisible();
        }
    }, Qt::QueuedConnection);

    engine.load(QUrl::fromLocalFile(qmlPath));

    if (engine.rootObjects().isEmpty() && !QFileInfo::exists(qmlPath)) {
        qCritical() << "[CachyDock] Error: could not find MainDock.qml";
        return -1;
    }

    return app.exec();
}
