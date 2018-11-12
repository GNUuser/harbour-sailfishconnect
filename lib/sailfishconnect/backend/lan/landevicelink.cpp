/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "landevicelink.h"

#include <QTimer>

#include "../../kdeconnectconfig.h"
#include "../linkprovider.h"
#include "socketlinereader.h"
#include "lanlinkprovider.h"
#include "../../corelogging.h"
#include "lanuploadjob.h"
#include "landownloadjob.h"
#include <sailfishconnect/helper/cpphelper.h>
#include <sailfishconnect/io/jobmanager.h>

using namespace SailfishConnect;

LanDeviceLink::LanDeviceLink(const QString& deviceId, LinkProvider* parent, QSslSocket* socket, ConnectionStarted connectionSource)
    : DeviceLink(deviceId, parent)
    , m_socketLineReader(nullptr)
    , m_debounceTimer(new QTimer(this))
{
    reset(socket, connectionSource);

    m_debounceTimer->setInterval(100);
    m_debounceTimer->setSingleShot(true);
    connect(m_debounceTimer, &QTimer::timeout,
            this, &LanDeviceLink::socketDisconnected);
}

void LanDeviceLink::reset(QSslSocket* socket, ConnectionStarted connectionSource)
{
    Q_ASSERT(socket->state() != QAbstractSocket::UnconnectedState);
    qCDebug(coreLogger) << "reseting device link";

    if (m_socketLineReader) {
        disconnect(m_socketLineReader);
        delete m_socketLineReader;
    }

    m_socketLineReader = new SocketLineReader(socket, this);

    connect(socket, &QAbstractSocket::disconnected,
            m_debounceTimer, Overload<>::of(&QTimer::start));
    connect(m_socketLineReader, &SocketLineReader::readyRead,
            this, &LanDeviceLink::dataReceived);

    //We take ownership of the socket.
    //When the link provider destroys us,
    //the socket (and the reader) will be
    //destroyed as well
    socket->setParent(m_socketLineReader);

    QString certString = KdeConnectConfig::instance()->getDeviceProperty(deviceId(), QStringLiteral("certificate"));
    DeviceLink::setPairStatus(certString.isEmpty()? PairStatus::NotPaired : PairStatus::Paired);
}

QHostAddress LanDeviceLink::hostAddress() const
{
    if (!m_socketLineReader) {
        return QHostAddress::Null;
    }
    QHostAddress addr = m_socketLineReader->m_socket->peerAddress();
    if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
        bool success;
        QHostAddress convertedAddr = QHostAddress(addr.toIPv4Address(&success));
        if (success) {
            qCDebug(coreLogger) << "Converting IPv6" << addr << "to IPv4" << convertedAddr;
            addr = convertedAddr;
        }
    }
    return addr;
}

QString LanDeviceLink::name()
{
    return QStringLiteral("LanLink"); // Should be same in both android and kde version
}

bool LanDeviceLink::sendPackage(NetworkPackage& np, JobManager* jobMgr)
{
    if (np.hasPayload()) {
        auto* uploadJob = sendPayload(np, jobMgr);
        if (uploadJob->isRunning()) {
            np.setPayloadTransferInfo(uploadJob->transferInfo());
        }
        // TODO: else return?
    }

    int written = m_socketLineReader->write(np.serialize());

    //Actually we can't detect if a package is received or not. We keep TCP
    //"ESTABLISHED" connections that look legit (return true when we use them),
    //but that are actually broken (until keepalive detects that they are down).
    return (written != -1);
}

LanUploadJob* LanDeviceLink::sendPayload(const NetworkPackage& np, JobManager* jobMgr)
{
    LanUploadJob* job = new LanUploadJob(np.payload(), deviceId());
    job->start();
    if (jobMgr) {
        jobMgr->addJob(job, deviceId());
    }

    return job;
}

void LanDeviceLink::dataReceived()
{
    if (m_socketLineReader->bytesAvailable() == 0) return;

    const QByteArray serializedPackage = m_socketLineReader->readLine();
    NetworkPackage package(QString::null);
    NetworkPackage::unserialize(serializedPackage, &package);

    qCDebug(coreLogger) << "LanDeviceLink dataReceived" << serializedPackage;

    if (package.type() == PACKAGE_TYPE_PAIR) {
        //TODO: Handle pair/unpair requests and forward them (to the pairing handler?)
        qobject_cast<LanLinkProvider*>(provider())->incomingPairPackage(this, package);
        return;
    }

    if (package.hasPayloadTransferInfo()) {
        qCDebug(coreLogger) << "HasPayloadTransferInfo";
        QVariantMap transferInfo = package.payloadTransferInfo();
        //FIXME: The next two lines shouldn't be needed! Why are they here?
        transferInfo.insert(QStringLiteral("useSsl"), true);
        transferInfo.insert(QStringLiteral("deviceId"), deviceId());
        LanDownloadJob* job = new LanDownloadJob(m_socketLineReader->peerAddress(), transferInfo);
        job->start();
        package.setPayload(job->getPayload(), package.payloadSize());
    }

    Q_EMIT receivedPackage(package);

    if (m_socketLineReader->bytesAvailable() > 0) {
        QMetaObject::invokeMethod(this, "dataReceived", Qt::QueuedConnection);
    }

}

void LanDeviceLink::socketDisconnected()
{
    // Maybe LanDeviceLink::reset was called
    qCDebug(coreLogger) << "socket has disconnected";
    if (m_socketLineReader->m_socket->state()
            == QAbstractSocket::UnconnectedState) {
        delete this;
    }
}

void LanDeviceLink::userRequestsPair()
{
    if (m_socketLineReader->peerCertificate().isNull()) {
        Q_EMIT pairingError(tr("This device cannot be paired because it is running an old version of KDE Connect."));
    } else {
        qobject_cast<LanLinkProvider*>(provider())->userRequestsPair(deviceId());
    }
}

void LanDeviceLink::userRequestsUnpair()
{
    qobject_cast<LanLinkProvider*>(provider())->userRequestsUnpair(deviceId());
}

void LanDeviceLink::setPairStatus(PairStatus status)
{
    if (status == Paired && m_socketLineReader->peerCertificate().isNull()) {
        Q_EMIT pairingError(tr("This device cannot be paired because it is running an old version of KDE Connect."));
        return;
    }

    DeviceLink::setPairStatus(status);
    if (status == Paired) {
        Q_ASSERT(KdeConnectConfig::instance()->trustedDevices().contains(deviceId()));
        Q_ASSERT(!m_socketLineReader->peerCertificate().isNull());
        KdeConnectConfig::instance()->setDeviceProperty(deviceId(), QStringLiteral("certificate"), m_socketLineReader->peerCertificate().toPem());
    }
}

bool LanDeviceLink::linkShouldBeKeptAlive() {

    return true;     //FIXME: Current implementation is broken, so for now we will keep links always established

    //We keep the remotely initiated connections, since the remotes require them if they want to request
    //pairing to us, or connections that are already paired. TODO: Keep connections in the process of pairing
    //return (mConnectionSource == ConnectionStarted::Remotely || pairStatus() == Paired);

}
