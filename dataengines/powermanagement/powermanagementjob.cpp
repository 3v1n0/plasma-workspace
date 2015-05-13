/*
 * Copyright 2011 Sebastian Kügler <sebas@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 2 as
 * published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusPendingReply>

#include <KAuthorized>

// kde-workspace/libs
#include <kworkspace.h>

#include <krunner_interface.h>

#include "powermanagementjob.h"

#include <Solid/Power/PowerManagement>

PowerManagementJob::PowerManagementJob(const QString &operation, QMap<QString, QVariant> &parameters, QObject *parent) :
    ServiceJob(parent->objectName(), operation, parameters, parent)
{
}

PowerManagementJob::~PowerManagementJob()
{
}

void PowerManagementJob::start()
{
    const QString operation = operationName();
    //qDebug() << "starting operation  ... " << operation;

    if (operation == "lockScreen") {
        if (KAuthorized::authorizeKAction("lock_screen")) {
            const QString interface("org.freedesktop.ScreenSaver");
            QDBusInterface screensaver(interface, "/ScreenSaver");
            screensaver.asyncCall("Lock");
            setResult(true);
            return;
        }
        qDebug() << "operation denied " << operation;
        setResult(false);
        return;
    } else if (operation == "suspend" || operation == "suspendToRam") {
        Solid::PowerManagement::suspend();
        setResult(Solid::PowerManagement::canSuspend());
        return;
    } else if (operation == "suspendToDisk") {
        Solid::PowerManagement::hibernate();
        setResult(Solid::PowerManagement::canHibernate());
        return;
    } else if (operation == "suspendHybrid") {
        Solid::PowerManagement::hybridSleep();
        setResult(Solid::PowerManagement::canHybridSleep());
        return;
    } else if (operation == "requestShutDown") {
        requestShutDown();
        setResult(Solid::PowerManagement::canShutdown());
        return;
    } else if (operation == "switchUser") {
        // Taken from kickoff/core/itemhandlers.cpp
        org::kde::krunner::App krunner("org.kde.krunner", "/App", QDBusConnection::sessionBus());
        krunner.switchUser();
        setResult(true);
        return;
    } else if (operation == "beginSuppressingSleep") {
        setResult(Solid::PowerManagement::beginSuppressingSleep(parameters().value("reason").toString()));
        return;
    } else if (operation == "stopSuppressingSleep") {
        setResult(Solid::PowerManagement::stopSuppressingSleep(parameters().value("cookie").toInt()));
        return;
    } else if (operation == "beginSuppressingScreenPowerManagement") {
        setResult(Solid::PowerManagement::beginSuppressingScreenPowerManagement(parameters().value("reason").toString()));
        return;
    } else if (operation == "stopSuppressingScreenPowerManagement") {
        setResult(Solid::PowerManagement::stopSuppressingScreenPowerManagement(parameters().value("cookie").toInt()));
        return;
    } else if (operation == "setBrightness") {
        setScreenBrightness(parameters().value("brightness").toInt(), parameters().value("silent").toBool());
        setResult(true);
        return;
    } else if (operation == "setKeyboardBrightness") {
        setKeyboardBrightness(parameters().value("brightness").toInt(), parameters().value("silent").toBool());
        setResult(true);
        return;
    }

    qDebug() << "don't know what to do with " << operation;
    setResult(false);
}

void PowerManagementJob::setScreenBrightness(int value, bool silent)
{
    QDBusMessage msg = QDBusMessage::createMethodCall("org.kde.Solid.PowerManagement",
                                                      "/org/kde/Solid/PowerManagement/Actions/BrightnessControl",
                                                      "org.kde.Solid.PowerManagement.Actions.BrightnessControl",
                                                      silent ? "setBrightnessSilent" : "setBrightness");
    msg << value;
    QDBusConnection::sessionBus().asyncCall(msg);
}

void PowerManagementJob::setKeyboardBrightness(int value, bool silent)
{
    QDBusMessage msg = QDBusMessage::createMethodCall("org.kde.Solid.PowerManagement",
                                                      "/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl",
                                                      "org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl",
                                                      silent ? "setKeyboardBrightnessSilent" : "setKeyboardBrightness");
    msg << value;
    QDBusConnection::sessionBus().asyncCall(msg);
}

void PowerManagementJob::requestShutDown()
{
    KWorkSpace::requestShutDown();
}

