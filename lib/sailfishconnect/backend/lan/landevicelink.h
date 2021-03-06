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

#ifndef LANDEVICELINK_H
#define LANDEVICELINK_H

#include <QObject>
#include <QString>
#include <QSslSocket>
#include <QSslCertificate>
#include <QHostAddress>

#include "../devicelink.h"

class SocketLineReader;
class QTimer;

namespace SailfishConnect {
class LanUploadJob;
class JobManager;
} // namespace SailfishConnect

class LanDeviceLink
    : public DeviceLink
{
    Q_OBJECT

public:
    enum ConnectionStarted : bool { Locally, Remotely };

    LanDeviceLink(const QString& deviceId, LinkProvider* parent, QSslSocket* socket, ConnectionStarted connectionSource);
    void reset(QSslSocket* socket, ConnectionStarted connectionSource);

    QString name() override;
    bool sendPackage(NetworkPackage& np, SailfishConnect::JobManager* jobMgr) override;
    SailfishConnect::LanUploadJob* sendPayload(const NetworkPackage& np, SailfishConnect::JobManager *jobMgr = nullptr);

    void userRequestsPair() override;
    void userRequestsUnpair() override;

    void setPairStatus(PairStatus status) override;

    bool linkShouldBeKeptAlive() override;

    QHostAddress hostAddress() const;

private Q_SLOTS:
    void dataReceived();
    void socketDisconnected();

private:
    SocketLineReader* m_socketLineReader;
    QHostAddress m_hostAddress;
    QTimer* m_debounceTimer;
};

#endif
