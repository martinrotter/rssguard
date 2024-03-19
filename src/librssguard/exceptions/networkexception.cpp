// For license of this file, see <project-root-folder>/LICENSE.md.

#include "exceptions/networkexception.h"

#include "network-web/networkfactory.h"

NetworkException::NetworkException(QNetworkReply::NetworkError error, const QString& message)
  : ApplicationException(message.simplified().isEmpty() ? NetworkFactory::networkErrorText(error) : message),
    m_networkError(error) {}

QNetworkReply::NetworkError NetworkException::networkError() const {
  return m_networkError;
}
