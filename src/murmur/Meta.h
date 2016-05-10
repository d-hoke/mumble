// Copyright 2005-2016 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MURMUR_META_H_
#define MUMBLE_MURMUR_META_H_

#include <QtCore/QDir>
#include <QtCore/QList>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslKey>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "Timer.h"

class Server;
class QSettings;

class MetaParams {
public:
	QDir qdBasePath;

	QList<QHostAddress> qlBind;
	unsigned short usPort;
	int iTimeout;
	int iMaxBandwidth;
	int iMaxUsers;
	int iMaxUsersPerChannel;
	int iDefaultChan;
	bool bRememberChan;
	int iMaxTextMessageLength;
	int iMaxImageMessageLength;
	int iOpusThreshold;
	int iChannelNestingLimit;
	/// If true the old SHA1 password hashing is used instead of PBKDF2
	bool legacyPasswordHash;
	/// Contains the default number of PBKDF2 iterations to use
	/// when hashing passwords. If the value loaded from config
	/// is <= 0 the value is loaded from the database and if not
	/// available there yet found by a benchmark.
	int kdfIterations;
	bool bAllowHTML;
	QString qsPassword;
	QString qsWelcomeText;
	bool bCertRequired;
	bool bForceExternalAuth;

	int iBanTries;
	int iBanTimeframe;
	int iBanTime;

	QString qsDatabase;
	QString qsDBDriver;
	QString qsDBUserName;
	QString qsDBPassword;
	QString qsDBHostName;
	QString qsDBPrefix;
	QString qsDBOpts;
	int iDBPort;

	int iLogDays;

	int iObfuscate;
	bool bSendVersion;
	bool bAllowPing;

	QString qsDBus;
	QString qsDBusService;
	QString qsLogfile;
	QString qsPid;
	QString qsIceEndpoint;
	QString qsIceSecretRead, qsIceSecretWrite;

	QString qsGRPCAddress;
	QString qsGRPCCert;
	QString qsGRPCKey;

	QString qsRegName;
	QString qsRegPassword;
	QString qsRegHost;
	QString qsRegLocation;
	QUrl qurlRegWeb;
	bool bBonjour;

	QRegExp qrUserName;
	QRegExp qrChannelName;

	QSslCertificate qscCert;
	QSslKey qskKey;
	QByteArray qbaDHParams;
	QByteArray qbaPassPhrase;
	QString qsCiphers;

	QMap<QString, QString> qmConfig;

#ifdef Q_OS_UNIX
	unsigned int uiUid, uiGid;
	QString qsHome;
	QString qsName;
#endif

	QVariant qvSuggestVersion;
	QVariant qvSuggestPositional;
	QVariant qvSuggestPushToTalk;

	QSettings *qsSettings;

	MetaParams();
	~MetaParams();
	void read(QString fname = QString("murmur.ini"));

private:
	template <class T>
	T typeCheckedFromSettings(const QString &name, const T &variable);
};

class Meta : public QObject {
	private:
		Q_OBJECT;
		Q_DISABLE_COPY(Meta);
	public:
		static MetaParams mp;
		QHash<int, Server *> qhServers;
		QHash<QHostAddress, QList<Timer> > qhAttempts;
		QHash<QHostAddress, Timer> qhBans;
		QString qsOS, qsOSVersion;
		Timer tUptime;

#ifdef Q_OS_WIN
		static HANDLE hQoS;
#endif

		Meta();
		~Meta();
		void bootAll();
		bool boot(int);
		bool banCheck(const QHostAddress &);
		void kill(int);
		void killAll();
		void getOSInfo();
		void connectListener(QObject *);
		static void getVersion(int &major, int &minor, int &patch, QString &string);
	signals:
		void started(Server *);
		void stopped(Server *);
};

extern Meta *meta;

#endif
