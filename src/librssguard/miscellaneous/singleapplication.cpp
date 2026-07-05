// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/singleapplication.h"

#include <QDataStream>
#include <QLocalServer>
#include <QLocalSocket>

#include <memory>

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

  connect(sck, &QLocalSocket::disconnected, sck, &QLocalSocket::deleteLater);

  auto stream = std::make_shared<QDataStream>(sck);

  stream->setVersion(QDataStream::Version::Qt_5_5);

  const auto process_message = [this, sck, stream]() {
    quint32 block_size;
    QString message;

    stream->startTransaction();
    *stream >> block_size;
    *stream >> message;

    if (!stream->commitTransaction()) {
      return;
    }

    if (quint32(message.size()) != block_size) {
      sck->disconnectFromServer();
      return;
    }

    emit messageReceived(message);
    sck->disconnectFromServer();
  };

  connect(sck, &QLocalSocket::readyRead, this, process_message);
  process_message();
}
