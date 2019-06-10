// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef RDFPARSER_H
#define RDFPARSER_H

#include "core/message.h"

#include <QList>

class RdfParser {
  public:
    explicit RdfParser();
    virtual ~RdfParser();

    QList<Message> parseXmlData(const QString& data);
};

#endif // RDFPARSER_H
