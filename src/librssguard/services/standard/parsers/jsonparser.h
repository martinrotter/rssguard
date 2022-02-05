// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef JSONPARSER_H
#define JSONPARSER_H

#include "core/message.h"

class JsonParser {
  public:
    explicit JsonParser(const QString& data);

    QList<Message> messages() const;

  private:
    QString m_jsonData;
};

#endif // JSONPARSER_H
