/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "desktopview.h"
#include "containmentconfigview.h"
#include "shellcorona.h"
#include "shellmanager.h"
#include "krunner_interface.h"

#if HAVE_WAYLAND
#  include "waylandsurface.h"
#endif

#include <QQmlEngine>
#include <QQmlContext>
#include <QScreen>
#include <qopenglshaderprogram.h>

#include <kwindowsystem.h>
#include <klocalizedstring.h>

#include <Plasma/Package>

DesktopView::DesktopView(ShellCorona *corona)
    : PlasmaQuick::View(corona, 0),
      m_corona(corona),
      m_stayBehind(false),
      m_fillScreen(false),
      m_dashboardShown(false)
{
#if HAVE_WAYLAND
    m_stayBehind = true;
    m_fillScreen = true;

    m_plasmaSurface = new WaylandSurface(this);
    m_plasmaSurface->setRole(WaylandSurface::DesktopRole);
#else
    setTitle(i18n("Desktop"));
    setIcon(QIcon::fromTheme("user-desktop"));
#endif

    engine()->rootContext()->setContextProperty("desktop", this);
    setSource(QUrl::fromLocalFile(corona->package().filePath("views", "Desktop.qml")));

    //For some reason, if I connect the method directly it doesn't get called, I think it's for the lack of argument
    connect(this, &QWindow::screenChanged, this, [=](QScreen*) { adaptToScreen(); ensureStayBehind(); });

    QObject::connect(corona, &Plasma::Corona::packageChanged,
                     this, &DesktopView::coronaPackageChanged);

    connect(this, &DesktopView::sceneGraphInitialized, this,
        [this, corona]() {
            // check whether the GL Context supports OpenGL
            // Note: hasOpenGLShaderPrograms is broken, see QTBUG--39730
            if (!QOpenGLShaderProgram::hasOpenGLShaderPrograms(openglContext())) {
                qWarning() << "GLSL not available, Plasma won't be functional";
                QMetaObject::invokeMethod(corona, "showOpenGLNotCompatibleWarning", Qt::QueuedConnection);
            }
        }, Qt::DirectConnection);
}

DesktopView::~DesktopView()
{
#if HAVE_WAYLAND
    delete m_plasmaSurface;
#endif
}

bool DesktopView::stayBehind() const
{
    return m_stayBehind;
}

void DesktopView::setStayBehind(bool stayBehind)
{
#if HAVE_WAYLAND
    Q_UNUSED(stayBehind);
#else
    if (ShellManager::s_forceWindowed || stayBehind == m_stayBehind) {
        return;
    }

    m_stayBehind = stayBehind;
    ensureStayBehind();

    emit stayBehindChanged();
#endif
}

bool DesktopView::fillScreen() const
{
    return m_fillScreen;
}

void DesktopView::setFillScreen(bool fillScreen)
{
#if HAVE_WAYLAND
    Q_UNUSED(fillScreen);
#else
    if (ShellManager::s_forceWindowed || fillScreen == m_fillScreen) {
        return;
    }

    m_fillScreen = fillScreen;
    adaptToScreen();
    emit fillScreenChanged();
#endif
}

void DesktopView::showEvent(QShowEvent* e)
{
    adaptToScreen();
    QQuickWindow::showEvent(e);
}

void DesktopView::adaptToScreen()
{
    //This happens sometimes, when shutting down the process
    if (!screen() || m_oldScreen==screen())
        return;
//     qDebug() << "adapting to screen" << screen()->name() << this;
    if (m_fillScreen) {
        setGeometry(screen()->geometry());
        setMinimumSize(screen()->geometry().size());
        setMaximumSize(screen()->geometry().size());

        disconnect(m_oldScreen.data(), &QScreen::geometryChanged, this, static_cast<void (QWindow::*)(const QRect&)>(&QWindow::setGeometry));
        connect(screen(), &QScreen::geometryChanged, this, static_cast<void (QWindow::*)(const QRect&)>(&QWindow::setGeometry), Qt::UniqueConnection);

    } else {
        disconnect(screen(), &QScreen::geometryChanged, this, static_cast<void (QWindow::*)(const QRect&)>(&QWindow::setGeometry));
    }

    m_oldScreen = screen();
}

void DesktopView::ensureStayBehind()
{
#if !HAVE_WAYLAND
    //This happens sometimes, when shutting down the process
    if (!screen())
        return;
    if (m_stayBehind) {
        KWindowSystem::setType(winId(), NET::Desktop);
    } else {
        KWindowSystem::setType(winId(), NET::Normal);
    }
#endif
}

void DesktopView::setDashboardShown(bool shown)
{
#if HAVE_WAYLAND
    if (shown)
        m_plasmaSurface->setRole(WaylandSurface::DashboardRole);
    else
        m_plasmaSurface->setRole(WaylandSurface::DesktopRole);
#else
    if (shown) {
        if (m_stayBehind) {
            KWindowSystem::setType(winId(), NET::Normal);
            KWindowSystem::clearState(winId(), NET::KeepBelow);
            KWindowSystem::setState(winId(), NET::SkipTaskbar|NET::SkipPager);
        }
        setFlags(Qt::FramelessWindowHint | Qt::CustomizeWindowHint);

        raise();
        KWindowSystem::raiseWindow(winId());
        KWindowSystem::forceActiveWindow(winId());

    } else {
        if (m_stayBehind) {
            KWindowSystem::setType(winId(), NET::Desktop);
            KWindowSystem::setState(winId(), NET::SkipTaskbar|NET::SkipPager|NET::KeepBelow);
        }
        lower();
        KWindowSystem::lowerWindow(winId());

    }
#endif

    m_dashboardShown = shown;
    emit dashboardShownChanged();
}

bool DesktopView::event(QEvent *e)
{
    if (e->type() == QEvent::KeyRelease) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        if (m_dashboardShown && ke->key() == Qt::Key_Escape) {
            static_cast<ShellCorona *>(corona())->setDashboardShown(false);
        }
    } else if (e->type() == QEvent::Close) {
        //prevent ALT+F4 from killing the shell
        e->ignore();
        return true;
    } else if (e->type() == QEvent::FocusOut) {
        QObject *graphicObject = containment()->property("_plasma_graphicObject").value<QObject *>();
        if (graphicObject) {
            graphicObject->setProperty("focus", false);
        }
    }

    return PlasmaQuick::View::event(e);
}

void DesktopView::keyPressEvent(QKeyEvent *e)
{
    // When a key is pressed on desktop when nothing else is active forward the key to krunner
    if (!e->modifiers() && activeFocusItem() == contentItem()) {
        const QString text = e->text().trimmed();
        if (!text.isEmpty() && text[0].isPrint()) {
            const QString interface("org.kde.krunner");
            org::kde::krunner::App krunner(interface, "/App", QDBusConnection::sessionBus());
            krunner.query(text);
            e->accept();
        }
    }

    QQuickView::keyPressEvent(e);
}

bool DesktopView::isDashboardShown() const
{
    return m_dashboardShown;
}


void DesktopView::showConfigurationInterface(Plasma::Applet *applet)
{
    if (m_configView) {
        m_configView.data()->hide();
        m_configView.data()->deleteLater();
    }

    if (!applet || !applet->containment()) {
        return;
    }

    Plasma::Containment *cont = qobject_cast<Plasma::Containment *>(applet);

    if (cont && cont->isContainment()) {
        m_configView = new ContainmentConfigView(cont);
    } else {
        m_configView = new PlasmaQuick::ConfigView(applet);
    }
    m_configView.data()->init();
    m_configView.data()->show();
}

void DesktopView::coronaPackageChanged(const Plasma::Package &package)
{
    setContainment(0);
    setSource(QUrl::fromLocalFile(package.filePath("views", "Desktop.qml")));
}

void DesktopView::screenDestroyed(QObject* screen)
{
//     NOTE: this is overriding the screen destroyed slot, we need to do this because
//     otherwise Qt goes mental and starts moving our panels. See:
//     https://codereview.qt-project.org/#/c/88351/
    if (screen == this->screen()) {
        m_corona->remove(this);
    }
}

#include "moc_desktopview.cpp"
