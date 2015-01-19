/*
   Copyright (C) 2007 by Olivier Goffart <ogoffart at kde.org>
   Copyright (C) 2009 by Laurent Montel <montel@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.

 */
#include "notifybyktts.h"
#include <QtDBus/QtDBus>
#include <QHash>
#include <ktoolinvocation.h>
#include <kmessagebox.h>
#include <kmacroexpander.h>
#include <klocale.h>
#include <knotifyconfig.h>

NotifyByKTTS::NotifyByKTTS(QObject *parent) : KNotificationPlugin(parent),m_kspeech(0), tryToStartKttsd( false )
{
}


NotifyByKTTS::~NotifyByKTTS()
{
}

void NotifyByKTTS::setupKttsd()
{
    m_kspeech = new org::kde::KSpeech("org.kde.kttsd", "/KSpeech", QDBusConnection::sessionBus());
    m_kspeech->setParent(this);
    m_kspeech->setApplicationName("KNotify");

    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(this);
    watcher->setConnection(QDBusConnection::sessionBus());
    watcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    watcher->addWatchedService("org.kde.kttsd");
    connect(watcher, SIGNAL(serviceUnregistered( const QString & ) ), this, SLOT( removeSpeech() ));
}

void NotifyByKTTS::notify( int id, KNotifyConfig * config )
{
        if( !m_kspeech)
        {
            if (  tryToStartKttsd ) //don't try to restart it all the time.
                return;
            // If KTTSD not running, start it.
            if (!QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.kttsd"))
            {
                QString error;
                if (KToolInvocation::startServiceByDesktopName("kttsd", QStringList(), &error))
                {
                    KMessageBox::error(0, i18n( "Starting Jovie Text-to-Speech Service Failed"), error );
                    tryToStartKttsd = true;
                    return;
                }
            }
            setupKttsd();
        }

        QString say = config->readEntry( "KTTS" );

	if (!say.isEmpty()) {
		QHash<QChar,QString> subst;
		subst.insert( 'e', config->eventid );
		subst.insert( 'a', config->appname );
		subst.insert( 's', config->text );
		subst.insert( 'w', QString::number( (quintptr)config->winId ));
		subst.insert( 'i', QString::number( id ));
		subst.insert( 'm', config->text );
		say = KMacroExpander::expandMacrosShellQuote( say, subst );
	}

	if ( say.isEmpty() )
		say = config->text; // fallback

    m_kspeech->setApplicationName(config->appname);
	m_kspeech->call(QDBus::NoBlock, "say", say, 0);

	finished(id);
}

void NotifyByKTTS::removeSpeech()
{
    tryToStartKttsd = false;

    delete m_kspeech;
    m_kspeech = 0;
}

