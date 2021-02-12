// For license of this file, see <project-root-folder>/LICENSE.md.

#include "exceptions/networkexception.h"

#include "network-web/networkfactory.h"

NetworkException::NetworkException(QNetworkReply::NetworkError error) : ApplicationException(NetworkFactory::networkErrorText(error)) {}

QNetworkReply::NetworkError NetworkException::networkError() const {
  return m_networkError;
}
