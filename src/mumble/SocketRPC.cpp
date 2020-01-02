// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "mumble_pch.hpp"

#include "SocketRPC.h"

#include "Channel.h"
#include "ClientUser.h"
#include "MainWindow.h"
#include "ServerHandler.h"
#include "Log.h"
#include "ConnectDialog.h"

// We define a global macro called 'g'. This can lead to issues when included code uses 'g' as a type or parameter name (like protobuf 3.7 does). As such, for now, we have to make this our last include.
#include "Global.h"

SocketRPCClient::SocketRPCClient(QLocalSocket *s, QObject *p) : QObject(p), qlsSocket(s), qbBuffer(NULL) {

	qWarning() << MainWindow::tr("SocketRPCClient()");
	qlsSocket->setParent(this);

	connect(qlsSocket, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(qlsSocket, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(error(QLocalSocket::LocalSocketError)));
	connect(qlsSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));

	qxsrReader.setDevice(qlsSocket);
	qxswWriter.setAutoFormatting(true);

	qbBuffer = new QBuffer(&qbaOutput, this);
	qbBuffer->open(QIODevice::WriteOnly);
	qxswWriter.setDevice(qbBuffer);
}

void SocketRPCClient::disconnected() {
	deleteLater();
}

void SocketRPCClient::error(QLocalSocket::LocalSocketError) {
}

void SocketRPCClient::readyRead() {
	forever {
		switch (qxsrReader.readNext()) {
			case QXmlStreamReader::Invalid: {
					if (qxsrReader.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
						qWarning() << "Malformed" << qxsrReader.error();
						qlsSocket->abort();
					}
					return;
				}
				break;
			case QXmlStreamReader::EndDocument: {
					qxswWriter.writeCurrentToken(qxsrReader);

					processXml();

					qxsrReader.clear();
					qxsrReader.setDevice(qlsSocket);

					qxswWriter.setDevice(NULL);
					delete qbBuffer;
					qbaOutput = QByteArray();
					qbBuffer = new QBuffer(&qbaOutput, this);
					qbBuffer->open(QIODevice::WriteOnly);
					qxswWriter.setDevice(qbBuffer);
				}
				break;
			default:
				qxswWriter.writeCurrentToken(qxsrReader);
				break;
		}
	}
}

void SocketRPCClient::processXml() {
	qWarning() << QString::fromStdString(std::string("SocketRPCClient::processXml()"));
	QDomDocument qdd;
	qdd.setContent(qbaOutput, false);

	QDomElement request = qdd.firstChildElement();

	if (! request.isNull()) {
		qWarning() << QString::fromStdString(std::string("SocketRPCClient::processXml(), request NOT null"));
		bool ack = false;
		QMap<QString, QVariant> qmRequest;
		QMap<QString, QVariant> qmReply;
		QMap<QString, QVariant>::const_iterator iter;

		auto urlActivity = [&]() {
			if (g.sh && g.sh->isRunning() && g.uiSession) {
				QString host, user, pw;
				unsigned short port;
				QUrl u;

				g.sh->getConnectionInfo(host, port, user, pw);
				u.setScheme(QLatin1String("mumble"));
				u.setHost(host);
				u.setPort(port);
				u.setUserName(user);

#if QT_VERSION >= 0x050000
				QUrlQuery query;
				query.addQueryItem(QLatin1String("version"), QLatin1String("1.2.0"));
				u.setQuery(query);
#else
				u.addQueryItem(QLatin1String("version"), QLatin1String("1.2.0"));
#endif

				QStringList path;
				Channel *c = ClientUser::get(g.uiSession)->cChannel;
				while (c->cParent) {
					path.prepend(c->qsName);
					c = c->cParent;
				}
				u.setPath(path.join(QLatin1String("/")));
				qmReply.insert(QLatin1String("href"), u);
			}

#if 0
			iter = qmRequest.find(QLatin1String("href"));
			if (iter != qmRequest.constEnd()) {
				QUrl u = iter.value().toUrl();
				if (u.isValid() && u.scheme() == QLatin1String("mumble")) {
					OpenURLEvent *oue = new OpenURLEvent(u);
					qApp->postEvent(g.mw, oue);
					ack = true;
				}
			}
			else {
				ack = true;
			}
#endif

		};

		QDomNamedNodeMap attributes = request.attributes();
		for (int i=0;i<attributes.count();++i) {
			QDomAttr attr = attributes.item(i).toAttr();
			qmRequest.insert(attr.name(), attr.value());
		}
		QDomNodeList childNodes = request.childNodes();
		for (int i=0;i<childNodes.count();++i) {
			QDomElement child = childNodes.item(i).toElement();
			if (! child.isNull())
				qmRequest.insert(child.nodeName(), child.text());
		}

		iter = qmRequest.find(QLatin1String("reqid"));
		if (iter != qmRequest.constEnd())
			qmReply.insert(iter.key(), iter.value());

		qWarning() << QString::fromStdString(std::string("SocketRPCClient::processXml(), request NOT null, beginning nested if()-else-s"));
		if (request.nodeName() == QLatin1String("focus")) {
			qWarning() << QString::fromStdString(std::string("SocketRPCClient::processXml(), focus"));
			g.mw->show();
			g.mw->raise();
			g.mw->activateWindow();

			ack = true;
		} else if (request.nodeName() == QLatin1String("self")) {
			qWarning() << QString::fromStdString(std::string("SocketRPCClient::processXml(), self"));
			//TBD: Might multiple of these occur in same command, or are they (some, all) mutually exclusive (i.e. if-else could be/have-been used)?
			iter = qmRequest.find(QLatin1String("mute"));
			if (iter != qmRequest.constEnd()) {
				bool set = iter.value().toBool();
				if (set != g.s.bMute) {
					g.mw->qaAudioMute->setChecked(! set);
					g.mw->qaAudioMute->trigger();
				}
			}
			iter = qmRequest.find(QLatin1String("unmute"));
			if (iter != qmRequest.constEnd()) {
				bool set = iter.value().toBool();
				if (set == g.s.bMute) {
					g.mw->qaAudioMute->setChecked(set);
					g.mw->qaAudioMute->trigger();
				}
			}
			iter = qmRequest.find(QLatin1String("togglemute"));
			if (iter != qmRequest.constEnd()) {
				bool set = iter.value().toBool();
				if (set == g.s.bMute) {
					g.mw->qaAudioMute->setChecked(set);
					g.mw->qaAudioMute->trigger();
				} else {
					g.mw->qaAudioMute->setChecked(! set);
					g.mw->qaAudioMute->trigger();
				}
			}
			iter = qmRequest.find(QLatin1String("deaf"));
			if (iter != qmRequest.constEnd()) {
				bool set = iter.value().toBool();
				if (set != g.s.bDeaf) {
					g.mw->qaAudioDeaf->setChecked(! set);
					g.mw->qaAudioDeaf->trigger();
				}
			}
			iter = qmRequest.find(QLatin1String("undeaf"));
			if (iter != qmRequest.constEnd()) {
				bool set = iter.value().toBool();
				if (set == g.s.bDeaf) {
					g.mw->qaAudioDeaf->setChecked(set);
					g.mw->qaAudioDeaf->trigger();
				}
			}
			iter = qmRequest.find(QLatin1String("toggledeaf"));
			if (iter != qmRequest.constEnd()) {
				bool set = iter.value().toBool();
				if (set == g.s.bDeaf) {
					g.mw->qaAudioDeaf->setChecked(set);
					g.mw->qaAudioDeaf->trigger();
				} else {
					g.mw->qaAudioDeaf->setChecked(! set);
					g.mw->qaAudioDeaf->trigger();
				}
			}
			ack = true;
			if(1) //diagnostic
			{
#if 01
				QDomNamedNodeMap attributes = request.attributes();
				for (int i = 0; i<attributes.count(); ++i) {
					QDomAttr attr = attributes.item(i).toAttr();
					//qmRequest.insert(attr.name(), attr.value());
					//std::cout << "attr " << i << ": " << attributes.item(i).text() << std::endl;
					//std::cout << "attr " << i << ": " << attributes.item(i).text().toStdString() << std::endl;
					//std::cout << "attr " << i << ": " << attributes.item(i).name().toStdString() << std::endl;
					std::cout << "attr " << i << ": " << attr.name().toStdString() << std::endl;
				}
#endif
				QDomNodeList childNodes = request.childNodes();
				{
					std::stringstream strmmsg;
					strmmsg << "rpcContMode, command diag, " << childNodes.count() << " child nodes:";
					//QString rpcMsg = MainWindow::tr("rpcMode: command diag, :");
					QString rpcMsg = MainWindow::tr(strmmsg.str().c_str());
					qWarning() << rpcMsg;
				}
				for (int i = 0; i<childNodes.count(); ++i) {
					QDomElement child = childNodes.item(i).toElement();
					if (!child.isNull())
					{
						//qmRequest.insert(child.nodeName(), child.text());
						//std::cout << "child " << i << ": " << child.text() << std::endl;
						std::cout << "child " << i << ": " << child.text().toStdString() << std::endl;
						std::stringstream sstr;
						sstr << "child " << i << ": " << child.text().toStdString() << std::endl;
						//g.l->log(Log::Information, MainWindow.tr(toStdString(sstr.str())));
						//g.l->log(Log::Information, QString::fromStdString(sstr.str()));
						qWarning() << QString::fromStdString(sstr.str());
					}
				}
				std::string delimiter{ " " };
				//std::string delimiters{ " \n" };
				std::string delimiters{ " \r" };
				std::vector<std::string> tokenList;
				size_t last = 0;
				size_t next = 0;
				std::string sbuf = childNodes.item(0).toElement().text().toStdString();
				{
					std::stringstream msg;
					msg << "self --> " << sbuf << std::endl;
					OutputDebugStringA(msg.str().c_str());
				}
				//while ((next = sbuf.find(delimiter, last)) != std::string::npos) 
				while ((next = sbuf.find_first_of(delimiters, last)) != std::string::npos)
				{
					//cout << s.substr(last, next - last) << endl; 
					tokenList.emplace_back(sbuf.substr(last, next - last));
					//last = next + 1; 
					last = next += delimiter.length(); //TBD: technically incorrect, and code won't work if we wanted multi-char delimiters, have to reword to use find and list of delimiters, probably...
				}
				//What does this do if there are *no* tokens? yields an empty token...
				tokenList.emplace_back(sbuf.substr(last, next - last));
				for (auto tok : tokenList)
				{
					//std::cout << "(len " << tok.length() << ") " << tok << std::endl;
					std::stringstream msg;
					msg << "(len " << tok.length() << ") " << tok << std::endl;
					qWarning() << QString::fromStdString(msg.str());
				}
				if (tokenList[0] == "url") {
					OutputDebugStringA("url request\n");
					//mumble://[username[:password]@]<address>[:port]/[channelpath]?version=<serverversion>[&title=<servername>][&url=<serverurl>]
					//'url mumble://unip:upwd@192.168.1.121:64738/Root/ch2?version=1.2.0"
					//'url mumble://myun:mypwd@192.168.1.121:64738/ch2/?version=1.2.0' - didn't need/want the 'Root'!!!
					//'url mumble://myun:mypwd@192.168.1.121:64738/?version=1.2.0' - didn't need/want the 'Root'!!!
					urlActivity();
					//iter = qmRequest.find(QLatin1String("href"));
					//if (iter != qmRequest.constEnd()) {
						//QUrl u = iter.value().toUrl();
						QUrl u{ QString(g.mw->tr(tokenList[1].c_str())) };
						if (u.isValid() && u.scheme() == QLatin1String("mumble")) {
							OpenURLEvent *oue = new OpenURLEvent(u);
							qApp->postEvent(g.mw, oue);
							ack = true;
						}
					//}
					//else {
					//	ack = true;
					//}
				}
				else if (tokenList[0] == "connect") {
					//ip
					//(?port?)
					//user
					//password?
					QSharedPointer<ConnectDialog> spcd = QSharedPointer<ConnectDialog>(new ConnectDialog(g.mw,false), &QObject::deleteLater);
					//spcd->setqsServer("192.168.1.121");
					//spcd->setusPort(64738u);
					//spcd->setqsUsername("myip");
					//spcd->setqsPassword("");
					//spcd->qsServer = g.mw->tr("192.168.1.121");
					//"connect 192.168.1.121 port 64738 as nobody password nothing"
					spcd->qsServer = g.mw->tr(tokenList[1].c_str());
					//'port' (64738u is default for server)
					spcd->usPort = stoi(tokenList[3]); // 64738u;
					//spcd->qsUsername=g.mw->tr("myip");
					//'as'
					spcd->qsUsername = g.mw->tr(tokenList[5].c_str());
					//spcd->qsPassword=g.mw->tr("");
					//'password'
					spcd->qsPassword = g.mw->tr(tokenList[7].c_str());
					emit g.mw->qaServerConnect_signal(spcd);
					//g.mw->qaServerConnect->trigger(spcd);
					//seems that initial connect 'join's root channel by default, any way to over-ride that?  Apparently not, or at least mumble UI doesn't currently support!!!
					//hmm, there is a 'findDesiredChannel()' sposedly called after initial connect to connect to 'desired' channel on 'connect'...
					//used by/from openUrl()...
				}
				else if (tokenList[0] == "hide") {
					//g.mw->hideEvent.trigger();
				}
				else if (tokenList[0] == "unhide") {
					//g.mw->showEvent.trigger();
				}
				else if (tokenList[0] == "disconnect") {
					//current connection implied?
					g.mw->qaServerDisconnect->trigger();
				}
				else if (tokenList[0] == "link") { //later - multi-plane
					OutputDebugStringA("link request\n");
					//based on void MainWindow::on_qaChannelLink_triggered() 
					Channel *c = ClientUser::get(g.uiSession)->cChannel;
					Channel *l = nullptr; // getContextMenuChannel();
					if (!l)
						l = Channel::get(0);

					g.sh->addChannelLink(c->iId, l->iId);
				}
				else if (tokenList[0] == "unlink") { //later - multi-plane
					OutputDebugStringA("unlink request\n");
					//based on void MainWindow::on_qaChannelUnlink_triggered() 
					Channel *c = ClientUser::get(g.uiSession)->cChannel;
					Channel *l = nullptr; // getContextMenuChannel();
					if (!l)
						l = Channel::get(0);

					g.sh->removeChannelLink(c->iId, l->iId);
				}
				else if (tokenList[0] == "createchannel") {
					OutputDebugStringA("createchannel request\n");
					//void MainWindow::on_qaChannelAdd_triggered()  which invokes ACLEdit->show()... so ACLEdit() must invoke the actual create/add...
					//parent channel (to create under - 'root' for now, default)
					//sub-channel name 
					//TBD: We *may* just want to imply a create of a channel when connect is done above, with our channel being reated under root...???!!!
					//in ACLEditor::Accept(), ACLEdit->show() invoked from MainWindow, in ACLEdit have
					//if (bAddChannelMode) {
					//	g.sh->createChannel(iChannel, qleChannelName->text(), rteChannelDescription->text(), qsbChannelPosition->value(), qcbChannelTemporary->isChecked(), qsbChannelMaxUsers->value());
					//}
					//g.sh->createChannel(iChannel, qleChannelName->text(), rteChannelDescription->text(), qsbChannelPosition->value(), qcbChannelTemporary->isChecked(), qsbChannelMaxUsers->value());
					//using temporary == true channel *does* seem to go away when all creating/joined users become disconnected
#if 0
					g.sh->createChannel(0, g.mw->tr("myplane"), g.mw->tr("for plane <?>"), 0, true/*temporary*/, 9999 /*hmm, TBD*/);
#else
					g.sh->createChannel(0, g.mw->tr(tokenList[1].c_str()), g.mw->tr(""), 0, true/*temporary*/, 9999 /*hmm, TBD*/);
#endif
					//g.sh->createChannel(0, g.mw->tr("myplane"), g.mw->tr("nodescription"), 0, false/*temporary*/, 9999 /*hmm, TBD*/);

				}
				else if (tokenList[0] == "deletechannel") {
					//MainWindow::on_qaChannelRemove_triggered(), invokes g.sh->removeChannel(c->iId);
					//sub-channel name 
					//TBD: how to get the correct channel number?
					//g.sh->removeChannel(c->iId);
					//g.sh->removeChannel(0);
					Channel *c = nullptr; // getContextMenuChannel();
					//if (!c)
					//	return;

					//int id = c->iId;

					//ret = QMessageBox::question(this, QLatin1String("Mumble"), tr("Are you sure you want to delete %1 and all its sub-channels?").arg(Qt::escape(c->qsName)), QMessageBox::Yes, QMessageBox::No);

					c = Channel::get(0); // id);
					if (!c)
						return;
					if (g.uiSession) {
						c = ClientUser::get(g.uiSession)->cChannel;
					}
					//if (ret == QMessageBox::Yes) {
						g.sh->removeChannel(c->iId);
					//}
				}
			}
		} else if (request.nodeName() == QLatin1String("url")) {
			qWarning() << QString::fromStdString(std::string("SocketRPCClient::processXml(), url"));
#if 1
			urlActivity();
			iter = qmRequest.find(QLatin1String("href"));
			if (iter != qmRequest.constEnd()) {
				QUrl u = iter.value().toUrl();
				if (u.isValid() && u.scheme() == QLatin1String("mumble")) {
					OpenURLEvent *oue = new OpenURLEvent(u);
					qApp->postEvent(g.mw, oue);
					ack = true;
				}
			}
			else {
				ack = true;
			}
#else
			if (g.sh && g.sh->isRunning() && g.uiSession) {
				QString host, user, pw;
				unsigned short port;
				QUrl u;

				g.sh->getConnectionInfo(host, port, user, pw);
				u.setScheme(QLatin1String("mumble"));
				u.setHost(host);
				u.setPort(port);
				u.setUserName(user);

#if QT_VERSION >= 0x050000
				QUrlQuery query;
				query.addQueryItem(QLatin1String("version"), QLatin1String("1.2.0"));
				u.setQuery(query);
#else
				u.addQueryItem(QLatin1String("version"), QLatin1String("1.2.0"));
#endif

				QStringList path;
				Channel *c = ClientUser::get(g.uiSession)->cChannel;
				while (c->cParent) {
					path.prepend(c->qsName);
					c = c->cParent;
				}
				u.setPath(path.join(QLatin1String("/")));
				qmReply.insert(QLatin1String("href"), u);
			}

			iter = qmRequest.find(QLatin1String("href"));
			if (iter != qmRequest.constEnd()) {
				QUrl u = iter.value().toUrl();
				if (u.isValid() && u.scheme() == QLatin1String("mumble")) {
					OpenURLEvent *oue = new OpenURLEvent(u);
					qApp->postEvent(g.mw, oue);
					ack = true;
				}
			} else {
				ack = true;
			}
#endif
		}
		else { //unknown/unhandled command/request...
		
			qWarning() << QLatin1String("rpc unknown/unhandled command/request...");
			//__debugbreak(); //are we getting here? apparently *not*, yet...
#if 0
			QDomNamedNodeMap attributes = request.attributes();
			for (int i = 0; i<attributes.count(); ++i) {
				//QDomAttr attr = attributes.item(i).toAttr();
				//qmRequest.insert(attr.name(), attr.value());
				//std::cout << "attr " << i << ": " << attributes.item(i).text() << std::endl;
				//std::cout << "attr " << i << ": " << attributes.item(i).text().toStdString() << std::endl;
				//std::cout << "attr " << i << ": " << attributes.item(i).name().toStdString() << std::endl;
				std::cout << "attr " << i << ": " << attributes.item(i).toStdString() << std::endl;
			}
#endif
			QDomNodeList childNodes = request.childNodes();
			{
				std::stringstream strmmsg;
				strmmsg << "rpcContMode, command diag, " << childNodes.count() << " child nodes:";
				//QString rpcMsg = MainWindow::tr("rpcMode: command diag, :");
				QString rpcMsg = MainWindow::tr(strmmsg.str().c_str());
				qWarning() << rpcMsg;
			}
			for (int i = 0; i<childNodes.count(); ++i) {
				QDomElement child = childNodes.item(i).toElement();
				if (!child.isNull())
				{
					//qmRequest.insert(child.nodeName(), child.text());
					//std::cout << "child " << i << ": " << child.text() << std::endl;
					std::cout << "child " << i << ": " << child.text().toStdString() << std::endl;
					std::stringstream sstr;
					sstr << "child " << i << ": " << child.text().toStdString() << std::endl;
					//g.l->log(Log::Information, MainWindow.tr(toStdString(sstr.str())));
					//g.l->log(Log::Information, QString::fromStdString(sstr.str()));
					qWarning() << QString::fromStdString(sstr.str());
				}
			}
		}

		QDomDocument replydoc;
		QDomElement reply = replydoc.createElement(QLatin1String("reply"));

		qmReply.insert(QLatin1String("succeeded"), ack);

		for (iter = qmReply.constBegin(); iter != qmReply.constEnd(); ++iter) {
			QDomElement elem = replydoc.createElement(iter.key());
			QDomText text = replydoc.createTextNode(iter.value().toString());
			elem.appendChild(text);
			reply.appendChild(elem);
		}

		replydoc.appendChild(reply);

		qlsSocket->write(replydoc.toByteArray());
	}
	else {
		qWarning() << QString::fromStdString(std::string("SocketRPCClient::processXml(), hmm request was null !?"));
	}
}

SocketRPC::SocketRPC(const QString &basename, QObject *p) : QObject(p) {
	qlsServer = new QLocalServer(this);

	QString pipepath;

#ifdef Q_OS_WIN
	pipepath = basename;
#else
	{
		QString xdgRuntimePath = QProcessEnvironment::systemEnvironment().value(QLatin1String("XDG_RUNTIME_DIR"));
		QDir xdgRuntimeDir = QDir(xdgRuntimePath);

		if (! xdgRuntimePath.isNull() && xdgRuntimeDir.exists()) {
			pipepath = xdgRuntimeDir.absoluteFilePath(basename + QLatin1String("Socket"));
		} else {
			pipepath = QDir::home().absoluteFilePath(QLatin1String(".") + basename + QLatin1String("Socket"));
		}
	}

	{
		QFile f(pipepath);
		if (f.exists()) {
			qWarning() << "SocketRPC: Removing old socket on" << pipepath;
			f.remove();
		}
	}
#endif

	if (! qlsServer->listen(pipepath)) {
		qWarning() << "SocketRPC: Listen failed";
		delete qlsServer;
		qlsServer = NULL;
	} else {
		connect(qlsServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
	}
}

void SocketRPC::newConnection() {
	qWarning() << "SocketRPC::newConnection()!";
	while (true) {
		QLocalSocket *qls = qlsServer->nextPendingConnection();
		if (! qls)
			break;
		new SocketRPCClient(qls, this);
	}
}

bool SocketRPC::send(const QString &basename, const QString &request, const QMap<QString, QVariant> &param) {
	QString pipepath;

#ifdef Q_OS_WIN
	pipepath = basename;
#else
	{
		QString xdgRuntimePath = QProcessEnvironment::systemEnvironment().value(QLatin1String("XDG_RUNTIME_DIR"));
		QDir xdgRuntimeDir = QDir(xdgRuntimePath);

		if (! xdgRuntimePath.isNull() && xdgRuntimeDir.exists()) {
			pipepath = xdgRuntimeDir.absoluteFilePath(basename + QLatin1String("Socket"));
		} else {
			pipepath = QDir::home().absoluteFilePath(QLatin1String(".") + basename + QLatin1String("Socket"));
		}
	}
#endif

	QLocalSocket qls;
	qls.connectToServer(pipepath);
	if (! qls.waitForConnected(1000)) {
		return false;
	}

	QDomDocument requestdoc;
	QDomElement req = requestdoc.createElement(request);
	for (QMap<QString, QVariant>::const_iterator iter = param.constBegin(); iter != param.constEnd(); ++iter) {
		QDomElement elem = requestdoc.createElement(iter.key());
		QDomText text = requestdoc.createTextNode(iter.value().toString());
		elem.appendChild(text);
		req.appendChild(elem);
	}
	requestdoc.appendChild(req);

	qls.write(requestdoc.toByteArray());
	qls.flush();

	if (! qls.waitForReadyRead(2000)) {
		return false;
	}

	QByteArray qba = qls.readAll();

	QDomDocument replydoc;
	replydoc.setContent(qba);

	QDomElement succ = replydoc.firstChildElement(QLatin1String("reply"));
	succ = succ.firstChildElement(QLatin1String("succeeded"));
	if (succ.isNull())
		return false;

	return QVariant(succ.text()).toBool();
}
