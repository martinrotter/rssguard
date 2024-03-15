// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NETWORKEXCEPTION_H
#define NETWORKEXCEPTION_H

#include "exceptions/applicationexception.h"

#include <QNetworkReply>

class RSSGUARD_DLLSPEC NetworkException : public ApplicationException {
  public:
    explicit NetworkException(QNetworkReply::NetworkError error, const QString& message = QString());

    QNetworkReply::NetworkError networkError() const;

  private:
    QNetworkReply::NetworkError m_networkError;
};

#endif // NETWORKEXCEPTION_H
