#include "core/message.h"


Enclosure::Enclosure(const QString &url, const QString &mime) : m_url(url), m_mimeType(mime) {
}

QList<Enclosure> Enclosures::decodeEnclosuresFromString(const QString &enclosures_data) {
  QList<Enclosure> enclosures;

  foreach (const QString &single_enclosure, enclosures_data.split(ENCLOSURES_OUTER_SEPARATOR, QString::SkipEmptyParts)) {
    Enclosure enclosure;

    if (single_enclosure.contains(ECNLOSURES_INNER_SEPARATOR)) {
      QStringList mime_url = single_enclosure.split(ECNLOSURES_INNER_SEPARATOR);

      enclosure.m_mimeType = QByteArray::fromBase64(mime_url.at(0).toLocal8Bit());
      enclosure.m_url = QByteArray::fromBase64(mime_url.at(1).toLocal8Bit());
    }
    else {
      enclosure.m_url = QByteArray::fromBase64(single_enclosure.toLocal8Bit());
    }

    enclosures.append(enclosure);
  }

  return enclosures;
}

QString Enclosures::encodeEnclosuresToString(const QList<Enclosure> &enclosures) {
  QStringList enclosures_str;

  foreach (const Enclosure &enclosure, enclosures) {
    if (enclosure.m_mimeType.isEmpty()) {
      enclosures_str.append(enclosure.m_url.toLocal8Bit().toBase64());
    }
    else {
      enclosures_str.append(QString(enclosure.m_mimeType.toLocal8Bit().toBase64()) +
                            ECNLOSURES_INNER_SEPARATOR +
                            enclosure.m_url.toLocal8Bit().toBase64());
    }
  }

  return enclosures_str.join(QString(ENCLOSURES_OUTER_SEPARATOR));
}

Message::Message() {
  m_title = m_url = m_author = m_contents = "";
  m_feedId = 0;
  m_enclosures = QList<Enclosure>();
}
