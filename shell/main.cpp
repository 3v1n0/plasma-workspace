/*
 *  Copyright 2012 Marco Martin <mart@kde.org>
 *  Copyright 2013 Sebastian Kügler <sebas@kde.org>
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

#include <QApplication>
#include <QCommandLineParser>
#include <QQuickWindow>
#include <QSessionManager>
#include <QDebug>

#include <kdbusservice.h>
#include <klocalizedstring.h>

#include "shellmanager.h"
#include "config-plasma.h"

#if HAVE_WAYLAND
#  include "waylandregistry.h"
#endif

static const char description[] = "Plasma Shell";
static const char version[] = "4.96.0";

void noMessageOutput(QtMsgType type, const char *msg)
{
     Q_UNUSED(type);
     Q_UNUSED(msg);
}
int main(int argc, char** argv)
{
    QQuickWindow::setDefaultAlphaBuffer(true);

    KLocalizedString::setApplicationDomain("plasmashell");

    QApplication app(argc, argv);
    app.setApplicationName("plasmashell");
    app.setApplicationDisplayName(i18n("Plasma"));
    app.setOrganizationDomain("kde.org");
    app.setApplicationVersion(version);
    app.setQuitOnLastWindowClosed(false);
    app.setWindowIcon(QIcon::fromTheme("plasma"));
    KDBusService service(KDBusService::Unique);

    // Make sure we are using the right platform plugin
#if HAVE_X11
    if (QGuiApplication::platformName() != QStringLiteral("xcb")) {
        qWarning() << qPrintable(i18n("Please run plasma-shell with -platform xcb or "
                                      "set QT_QPA_PLATFORM_PLUGIN environment variable accordingly."));
        return 127;
    }
#endif
#if HAVE_WAYLAND
    if (!QGuiApplication::platformName().startsWith(QStringLiteral("wayland"))) {
        qWarning() << qPrintable(i18n("Please run plasma-shell with -platform wayland or "
                                      "set QT_QPA_PLATFORM_PLUGIN environment variable accordingly."));
        return 127;
    }
#endif

    QCommandLineParser cliOptions;
    cliOptions.setApplicationDescription(description);
    cliOptions.addVersionOption();
    cliOptions.addHelpOption();

    QCommandLineOption dbgOption(QStringList() << QStringLiteral("d") <<
                                 QStringLiteral("qmljsdebugger"),
                                 i18n("Enable QML Javascript debugger"));

    QCommandLineOption winOption(QStringList() << QStringLiteral("w") <<
                                 QStringLiteral("windowed"),
                                 i18n("Force a windowed view for testing purposes"));

    QCommandLineOption respawnOption(QStringList() << QStringLiteral("n") <<
                                     QStringLiteral("no-respawn"),
                                     i18n("Do not restart plasma-shell automatically after a crash"));

    QCommandLineOption crashOption(QStringList() << QStringLiteral("c") << QStringLiteral("crashes"),
                                   i18n("Recent number of crashes"),
                                   QStringLiteral("n"));

    QCommandLineOption shutupOption(QStringList() << QStringLiteral("s") << QStringLiteral("shut-up"),
                                    i18n("Shuts up the output"));

    QCommandLineOption shellPluginOption(QStringList() << QStringLiteral("p") << QStringLiteral("shell-plugin"),
                                         i18n("Force loading the given shell plugin"),
                                         QStringLiteral("plugin"));

    cliOptions.addOption(dbgOption);
    cliOptions.addOption(winOption);
    cliOptions.addOption(respawnOption);
    cliOptions.addOption(crashOption);
    cliOptions.addOption(shutupOption);
    cliOptions.addOption(shellPluginOption);

    cliOptions.process(app);

    if (cliOptions.isSet(shutupOption)) {
        qInstallMsgHandler(noMessageOutput);
    }

    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

#if HAVE_WAYLAND
    WaylandRegistry::instance();
#endif

    ShellManager::setCrashCount(cliOptions.value(crashOption).toInt());
    ShellManager::s_forceWindowed = cliOptions.isSet(winOption);
    ShellManager::s_noRespawn = cliOptions.isSet(respawnOption);
    ShellManager::s_fixedShell = cliOptions.value(shellPluginOption);

    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), ShellManager::instance(), SLOT(deleteLater()));

    return app.exec();
}
