#ifndef PARSINGFACTORY_H
#define PARSINGFACTORY_H

#include <QNetworkReply>

#include "core/messagesmodel.h"


class ParsingFactory {
  private:
    ParsingFactory();

  public:
    // Parses inpute textual data into Message objects.
    // NOTE: Input is correctly encoded in Unicode.
    static QList<Message> parseAsRSS20(const QString &data);
};

#endif // PARSINGFACTORY_H
