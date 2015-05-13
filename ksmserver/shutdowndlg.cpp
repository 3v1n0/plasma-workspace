/*****************************************************************
ksmserver - the KDE session management server

Copyright 2000 Matthias Ettrich <ettrich@kde.org>
Copyright 2007 Urs Wolfer <uwolfer @ kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#include "shutdowndlg.h"
//include <ksmserver_debug.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QQuickItem>
#include <QTimer>
#include <QFile>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusPendingCall>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlPropertyMap>
#include <QPainter>
#include <QStandardPaths>
#include <QtX11Extras/qx11info_x11.h>
#include <QScreen>
#include <QStandardPaths>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include <KIconLoader>
#include <KLocalizedString>
#include <KUser>
#include <Solid/Power/PowerManagement>
#include <KWindowSystem>
#include <KDeclarative/KDeclarative>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KJob>

#include <stdio.h>
#include <netwm.h>

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <kdisplaymanager.h>

#include <config-workspace.h>

Q_DECLARE_METATYPE(Solid::PowerManagement::SleepState)

KSMShutdownDlg::KSMShutdownDlg( QWindow* parent,
                                bool maysd, bool choose, KWorkSpace::ShutdownType sdtype,
                                const QString& theme)
  : QQuickView(parent),
    m_result(false)
    // this is a WType_Popup on purpose. Do not change that! Not
    // having a popup here has severe side effects.
{
    // window stuff
    setClearBeforeRendering(true);
    setColor(QColor(Qt::transparent));
    setFlags(Qt::FramelessWindowHint | Qt::BypassWindowManagerHint);

    QPoint globalPosition(QCursor::pos());
    foreach (QScreen *s, QGuiApplication::screens()) {
        if (s->geometry().contains(globalPosition)) {
            setScreen(s);
            break;
        }
    }


    // Qt doesn't set this on unmanaged windows
    //FIXME: or does it?
    XChangeProperty( QX11Info::display(), winId(),
        XInternAtom( QX11Info::display(), "WM_WINDOW_ROLE", False ), XA_STRING, 8, PropModeReplace,
        (unsigned char *)"logoutdialog", strlen( "logoutdialog" ));


    //QQuickView *windowContainer = QQuickView::createWindowContainer(m_view, this);
    //windowContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QQmlContext *context = rootContext();
    context->setContextProperty(QStringLiteral("maysd"), maysd);
    context->setContextProperty(QStringLiteral("choose"), choose);
    context->setContextProperty(QStringLiteral("sdtype"), sdtype);

    QQmlPropertyMap *mapShutdownType = new QQmlPropertyMap(this);
    mapShutdownType->insert(QStringLiteral("ShutdownTypeDefault"), QVariant::fromValue<int>(KWorkSpace::ShutdownTypeDefault));
    mapShutdownType->insert(QStringLiteral("ShutdownTypeNone"), QVariant::fromValue<int>(KWorkSpace::ShutdownTypeNone));
    mapShutdownType->insert(QStringLiteral("ShutdownTypeReboot"), QVariant::fromValue<int>(KWorkSpace::ShutdownTypeReboot));
    mapShutdownType->insert(QStringLiteral("ShutdownTypeHalt"), QVariant::fromValue<int>(KWorkSpace::ShutdownTypeHalt));
    mapShutdownType->insert(QStringLiteral("ShutdownTypeLogout"), QVariant::fromValue<int>(KWorkSpace::ShutdownTypeLogout));
    context->setContextProperty(QStringLiteral("ShutdownType"), mapShutdownType);

    QQmlPropertyMap *mapSpdMethods = new QQmlPropertyMap(this);
    const QSet<Solid::PowerManagement::SleepState> spdMethods = Solid::PowerManagement::supportedSleepStates();
    mapSpdMethods->insert(QStringLiteral("StandbyState"), QVariant::fromValue(spdMethods.contains(Solid::PowerManagement::StandbyState)));
    mapSpdMethods->insert(QStringLiteral("SuspendState"), QVariant::fromValue(spdMethods.contains(Solid::PowerManagement::SuspendState)));
    mapSpdMethods->insert(QStringLiteral("HibernateState"), QVariant::fromValue(spdMethods.contains(Solid::PowerManagement::HibernateState)));
    context->setContextProperty(QStringLiteral("spdMethods"), mapSpdMethods);

    QString bootManager = KConfig(QStringLiteral(KDE_CONFDIR "/kdm/kdmrc"), KConfig::SimpleConfig)
                          .group("Shutdown")
                          .readEntry("BootManager", "None");
    context->setContextProperty(QStringLiteral("bootManager"), bootManager);

    int def, cur;
    if ( KDisplayManager().bootOptions( rebootOptions, def, cur ) ) {
        if ( cur > -1 ) {
            def = cur;
        }
    }
    QQmlPropertyMap *rebootOptionsMap = new QQmlPropertyMap(this);
    rebootOptionsMap->insert(QStringLiteral("options"), QVariant::fromValue(rebootOptions));
    rebootOptionsMap->insert(QStringLiteral("default"), QVariant::fromValue(def));
    context->setContextProperty(QStringLiteral("rebootOptions"), rebootOptionsMap);
    context->setContextProperty(QStringLiteral("screenGeometry"), screen()->geometry());

    setModality(Qt::ApplicationModal);

    // engine stuff
    KDeclarative::KDeclarative kdeclarative;
    kdeclarative.setDeclarativeEngine(engine());
    kdeclarative.initialize();
    kdeclarative.setupBindings();
//    windowContainer->installEventFilter(this);

    QString fileName;
    if(theme.isEmpty()) {
        KPackage::Package package = KPackage::PackageLoader::self()->loadPackage("Plasma/LookAndFeel");
        KConfigGroup cg(KSharedConfig::openConfig("kdeglobals"), "KDE");
        const QString packageName = cg.readEntry("LookAndFeelPackage", QString());
        if (!packageName.isEmpty()) {
            package.setPath(packageName);
        }

        fileName = package.filePath("logoutmainscript");
    } else
        fileName = theme;

    if (QFile::exists(fileName)) {
        //qCDebug(KSMSERVER) << "Using QML theme" << fileName;
        setSource(QUrl::fromLocalFile(fileName));
    } else {
        qWarning() << "Couldn't find a theme for the Shutdown dialog" << fileName;
        return;
    }

    setPosition(screen()->virtualGeometry().center().x() - width() / 2,
                screen()->virtualGeometry().center().y() - height() / 2);

    if(!errors().isEmpty()) {
        qWarning() << errors();
    }

    connect(rootObject(), SIGNAL(logoutRequested()), SLOT(slotLogout()));
    connect(rootObject(), SIGNAL(haltRequested()), SLOT(slotHalt()));
    connect(rootObject(), SIGNAL(suspendRequested(int)), SLOT(slotSuspend(int)) );
    connect(rootObject(), SIGNAL(rebootRequested()), SLOT(slotReboot()));
    connect(rootObject(), SIGNAL(rebootRequested2(int)), SLOT(slotReboot(int)) );
    connect(rootObject(), SIGNAL(cancelRequested()), SLOT(reject()));
    connect(rootObject(), SIGNAL(lockScreenRequested()), SLOT(slotLockScreen()));

    show();
    requestActivate();

    KWindowSystem::setState(winId(), NET::SkipTaskbar|NET::SkipPager);
}

void KSMShutdownDlg::resizeEvent(QResizeEvent *e)
{
    QQuickView::resizeEvent( e );

    if( KWindowSystem::compositingActive()) {
        //TODO: reenable window mask when we are without composite?
//        clearMask();
    } else {
//        setMask(m_view->mask());
    }

    setPosition(screen()->geometry().center().x() - width() / 2,
                screen()->geometry().center().y() - height() / 2);
}

void KSMShutdownDlg::slotLogout()
{
    m_shutdownType = KWorkSpace::ShutdownTypeNone;
    accept();
}

void KSMShutdownDlg::slotReboot()
{
    // no boot option selected -> current
    m_bootOption.clear();
    m_shutdownType = KWorkSpace::ShutdownTypeReboot;
    accept();
}

void KSMShutdownDlg::slotReboot(int opt)
{
    if (int(rebootOptions.size()) > opt)
        m_bootOption = rebootOptions[opt];
    m_shutdownType = KWorkSpace::ShutdownTypeReboot;
    accept();
}


void KSMShutdownDlg::slotLockScreen()
{
    m_bootOption.clear();
    QDBusMessage call = QDBusMessage::createMethodCall(QStringLiteral("org.kde.screensaver"),
                                                       QStringLiteral("/ScreenSaver"),
                                                       QStringLiteral("org.freedesktop.ScreenSaver"),
                                                       QStringLiteral("Lock"));
    QDBusConnection::sessionBus().asyncCall(call);
    reject();
}

void KSMShutdownDlg::slotHalt()
{
    m_bootOption.clear();
    m_shutdownType = KWorkSpace::ShutdownTypeHalt;
    accept();
}

void KSMShutdownDlg::slotSuspend(int spdMethod)
{
    m_bootOption.clear();
    switch (spdMethod) {
    case Solid::PowerManagement::StandbyState:
    case Solid::PowerManagement::SuspendState:
        Solid::PowerManagement::suspend();
        break;
    case Solid::PowerManagement::HibernateState:
        Solid::PowerManagement::hibernate();
        break;
    }
    reject();
}

void KSMShutdownDlg::accept()
{
    emit accepted();
}

void KSMShutdownDlg::reject()
{
    emit rejected();
}

bool KSMShutdownDlg::exec()
{
    QEventLoop loop;
    m_result = false;
    connect(this, &KSMShutdownDlg::accepted, &loop, &QEventLoop::quit);
    connect(this, &KSMShutdownDlg::accepted, [=]() { m_result = true; });

    connect(this, &KSMShutdownDlg::rejected, &loop, &QEventLoop::quit);
    loop.exec();
    return m_result;
}

bool KSMShutdownDlg::confirmShutdown(
        bool maysd, bool choose, KWorkSpace::ShutdownType& sdtype, QString& bootOption,
        const QString& theme)
{
    QScopedPointer<KSMShutdownDlg> l(new KSMShutdownDlg( 0, maysd, choose, sdtype, theme ));

    XClassHint classHint;
    classHint.res_name = const_cast<char*>("ksmserver");
    classHint.res_class = const_cast<char*>("ksmserver");
    XSetClassHint(QX11Info::display(), l->winId(), &classHint);

    bool result = l->exec();
    sdtype = l->m_shutdownType;
    bootOption = l->m_bootOption;

    return result;
}
