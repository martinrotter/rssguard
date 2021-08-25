// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/singleapplication.h"

#include "definitions/definitions.h"

#include <QDataStream>
#include <QLocalServer>
#include <QLocalSocket>

SingleApplication::SingleApplication(const QString& id, int& argc, char** argv)
  : QApplication(argc, argv), m_id(id), m_server(new QLocalServer(this)) {}

SingleApplication::~SingleApplication() {
  finish();
}

void SingleApplication::finish() {
  if (m_server->isListening()) {
    m_server->close();
  }
}

bool SingleApplication::isOtherInstanceRunning(const QString& message) {
  QLocalSocket sck;

  sck.connectToServer(m_id);

  if (sck.waitForConnected(200)) {
    if (!message.isEmpty()) {
      QDataStream out(&sck);

      out.setVersion(QDataStream::Version::Qt_5_5);

      out << quint32(message.size());
      out << message;

      sck.flush();
      sck.waitForBytesWritten(200);
    }

    sck.disconnectFromServer();
    sck.abort();
    return true;
  }

  QLocalServer::removeServer(m_id);
  auto i_am_first = m_server->listen(m_id);

  if (i_am_first) {
    connect(m_server, &QLocalServer::newConnection, this, &SingleApplication::processMessageFromOtherInstance);
  }

  return !i_am_first;
}

bool SingleApplication::sendMessage(const QString& message) {
  return isOtherInstanceRunning(message);
}

void SingleApplication::processMessageFromOtherInstance() {
  auto* sck = m_server->nextPendingConnection();

  if (sck == nullptr) {
    return;
  }

  quint32 block_size = 0;
  QEventLoop loop;
  QDataStream in(sck);

  in.setVersion(QDataStream::Version::Qt_5_5);

  connect(sck, &QLocalSocket::disconnected, sck, &QLocalSocket::deleteLater);
  connect(sck, &QLocalSocket::readyRead, this, [this, sck, &loop, &in, &block_size]() {
    if (block_size == 0) {
      if (sck->bytesAvailable() < int(sizeof(quint32))) {
        return;
      }

      in >> block_size;
    }

    if (sck->bytesAvailable() < block_size || in.atEnd()) {
      return;
    }

    QString dat;
    in >> dat;

    emit messageReceived(dat);
    loop.exit();
  });

  sck->flush();
  loop.exec();
}
