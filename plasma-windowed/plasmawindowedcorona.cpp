/*
 * Copyright 2014  Bhushan Shah <bhush94@gmail.com>
 * Copyright 2014 Marco Martin <notmart@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "plasmawindowedcorona.h"
#include "plasmawindowedview.h"
#include "simpleshellcorona.h"
#include <QDebug>
#include <QAction>
#include <QQuickItem>

#include <KActionCollection>
#include <Plasma/Package>
#include <Plasma/PluginLoader>

PlasmaWindowedCorona::PlasmaWindowedCorona(QObject *parent)
    : Plasma::Corona(parent),
      m_containment(0)
{
    Plasma::Package package = Plasma::PluginLoader::self()->loadPackage("Plasma/Shell");
    package.setPath("org.kde.plasma.desktop");
    setPackage(package);
    //QMetaObject::invokeMethod(this, "load", Qt::QueuedConnection);
    load();
}

void PlasmaWindowedCorona::loadApplet(const QString &applet, const QVariantList &arguments)
{
    if (containments().isEmpty()) {
        return;
    }

    Plasma::Containment *cont = containments().first();

    //forbid more instances per applet (todo: activate the correpsponding already loaded applet)
    for (Plasma::Applet *a : cont->applets()) {
        if (a->pluginInfo().pluginName() == applet) {
            return;
        }
    }
    PlasmaWindowedView *v = new PlasmaWindowedView();
    v->show();

    KConfigGroup appletsGroup(KSharedConfig::openConfig(), "Applets");
    QString plugin;
    for (const QString &group : appletsGroup.groupList()) {
        KConfigGroup cg(&appletsGroup, group);
        plugin = cg.readEntry("plugin", QString());

        if (plugin == applet) {
            Plasma::Applet *a = Plasma::PluginLoader::self()->loadApplet(applet, group.toInt(), arguments);
            if (!a) {
                qWarning() << "Unable to load applet" << applet << "with arguments" <<arguments;
                v->deleteLater();
                return;
            }
            a->restore(cg);

            //Access a->config() before adding to containment
            //will cause applets to be saved in palsmawindowedrc
            //so applets will only be created on demand
            KConfigGroup cg2 = a->config();
            cont->addApplet(a);

            v->setApplet(a);
            return;
        }
    }

    Plasma::Applet *a = Plasma::PluginLoader::self()->loadApplet(applet, 0, arguments);
    if (!a) {
        qWarning() << "Unable to load applet" << applet << "with arguments" <<arguments;
        v->deleteLater();
        return;
    }

    //Access a->config() before adding to containment
    //will cause applets to be saved in palsmawindowedrc
    //so applets will only be created on demand
    KConfigGroup cg2 = a->config();
    cont->addApplet(a);

    v->setApplet(a);
}

void PlasmaWindowedCorona::activateRequested(const QStringList &arguments, const QString &workingDirectory)
{
    qDebug() << "Activate requested with arguments" << arguments;
    Q_UNUSED(workingDirectory)
    if (!arguments.count() > 1) {
        return;
    }

    QVariantList args;
    QStringList::const_iterator constIterator;
    constIterator = arguments.constBegin();
    ++constIterator;
    for (; constIterator != arguments.constEnd();
           ++constIterator) {
        args << (*constIterator);
    }

    loadApplet(arguments[1], args);
}

QRect PlasmaWindowedCorona::screenGeometry(int id) const
{
    Q_UNUSED(id);
    //TODO?
    return QRect();
}

void PlasmaWindowedCorona::load()
{
    /*this won't load applets, since applets are in plasmawindowedrc*/
    loadLayout("plasmawindowed-appletsrc");


    bool found = false;
    for (auto c : containments()) {
        if (c->containmentType() == Plasma::Types::DesktopContainment) {
            found = true;
            break;
        }
    }

    if (!found) {
        qDebug() << "Loading default layout";
        createContainment("empty");
        saveLayout("plasmawindowed-appletsrc");
    }

    for (auto c : containments()) {
        if (c->containmentType() == Plasma::Types::DesktopContainment) {
            m_containment = c;
            m_containment->setFormFactor(Plasma::Types::Application);
            QAction *removeAction = c->actions()->action("remove");
            if(removeAction) {
                removeAction->deleteLater();
            }
            break;
        }
    }
}

void PlasmaWindowedCorona::loadFullCorona(const QString &plugin)
{
    if (m_simpleShellCoronas.contains(plugin)) {
        return;
    }

    m_simpleShellCoronas[plugin] = new SimpleShellCorona(this);
}

#include "plasmawindowedcorona.moc"
