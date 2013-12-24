#ifndef PARSINGFACTORY_H
#define PARSINGFACTORY_H

#include "core/messagesmodel.h"

#include <QList>


// This class contains methods to
// parse input Unicode textual data into
// another objects.
//
// NOTE: Each parsed message MUST CONTAINT THESE FIELDS.

class ParsingFactory {
  private:
    // Constructors and destructors.
    explicit ParsingFactory();

  public:
    // Parses input textual data into Message objects.
    // NOTE: Input is correctly encoded in Unicode.
    static QList<Message> parseAsRSS20(const QString &data);
};

#endif // PARSINGFACTORY_H
