// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/xmlencodingdetector.h"

#include "definitions/definitions.h"

QString XmlEncodingDetector::extractXmlDeclEncoding(const QString& text) {
  if (!text.trimmed().startsWith(QSL("<?xml"))) {
    return QString();
  }

  int end = text.indexOf(QL1C('>'));

  if (end == -1) {
    return QString();
  }

  int pos = text.indexOf(QSL("encoding"));

  if (pos == -1 || pos >= end) {
    return QString();
  }

  while (pos < end) {
    const QChar c = text.at(pos);
    if (c == QL1C('"') || c == QL1C('\'')) {
      break;
    }

    ++pos;
  }

  if (pos >= end) {
    return QString();
  }

  QChar quote = text.at(pos);
  ++pos;

  QString enc;
  while (pos < end) {
    QChar c = text.at(pos);

    if (c == quote) {
      break;
    }

    enc.append(c);
    ++pos;
  }

  return enc;
}

QString XmlEncodingDetector::detectBomEncoding(const QByteArray& data) {
  const uchar* d = reinterpret_cast<const uchar*>(data.constData());
  const int n = data.size();

  if (n >= 4) {
    if (d[0] == 0x00 && d[1] == 0x00 && d[2] == 0xFE && d[3] == 0xFF) {
      return QSL("UTF-32BE");
    }

    if (d[0] == 0xFF && d[1] == 0xFE && d[2] == 0x00 && d[3] == 0x00) {
      return QSL("UTF-32LE");
    }

    if (d[0] == 0x3C && d[1] == 0x00 && d[2] == 0x00 && d[3] == 0x00) {
      return QSL("UTF-32LE");
    }

    if (d[0] == 0x00 && d[1] == 0x00 && d[2] == 0x00 && d[3] == 0x3C) {
      return QSL("UTF-32BE");
    }
  }

  if (n >= 2) {
    if (d[0] == 0xFE && d[1] == 0xFF) {
      return QSL("UTF-16BE");
    }

    if (d[0] == 0xFF && d[1] == 0xFE) {
      return QSL("UTF-16LE");
    }

    if (d[0] == 0x3C && d[1] == 0x00) {
      return QSL("UTF-16LE");
    }

    if (d[0] == 0x00 && d[1] == 0x3C) {
      return QSL("UTF-16BE");
    }
  }

  return QString();
}

QString XmlEncodingDetector::extractAsciiProbe(const QByteArray& data, const QString& provisional_enc) {
  QString out;
  out.reserve(256);

  const uchar* d = reinterpret_cast<const uchar*>(data.constData());
  int n = data.size();

  if (provisional_enc == QL1S("UTF-8")) {
    // ASCII compatible — direct copy of bytes < 0x80
    for (int i = 0; i < n && out.size() < 256; ++i) {
      if (d[i] < 0x80) {
        out.append(QChar(d[i]));
      }
      else {
        break; // XML decl is ASCII, stop at non-ASCII
      }
    }
  }
  else if (provisional_enc == QL1S("UTF-16LE")) {
    for (int i = 0; i + 1 < n && out.size() < 256; i += 2) {
      out.append(QChar(d[i])); // low byte
      if (d[i] >= 0x80) {
        break;
      }
    }
  }
  else if (provisional_enc == QL1S("UTF-16BE")) {
    for (int i = 0; i + 1 < n && out.size() < 256; i += 2) {
      out.append(QChar(d[i + 1])); // low byte
      if (d[i + 1] >= 0x80) {
        break;
      }
    }
  }
  else if (provisional_enc == QL1S("UTF-32LE")) {
    for (int i = 0; i + 3 < n && out.size() < 256; i += 4) {
      out.append(QChar(d[i])); // lowest byte holds ASCII
      if (d[i] >= 0x80) {
        break;
      }
    }
  }
  else if (provisional_enc == QL1S("UTF-32BE")) {
    for (int i = 0; i + 3 < n && out.size() < 256; i += 4) {
      out.append(QChar(d[i + 3]));
      if (d[i + 3] >= 0x80) {
        break;
      }
    }
  }
  else {
    // Unknown → guess ASCII-compatible
    for (int i = 0; i < n && out.size() < 256; ++i) {
      if (d[i] < 0x80) {
        out.append(QChar(d[i]));
      }
      else {
        break;
      }
    }
  }

  return out;
}

QString XmlEncodingDetector::detectXmlEncoding(const QByteArray& data) {
  QByteArray trimmed_data = data.trimmed();

  if (trimmed_data.isEmpty()) {
    return QSL("UTF-8");
  }

  QString enc = detectBomEncoding(trimmed_data);

  if (enc.isEmpty()) {
    enc = QSL("UTF-8");
  }

  QString ascii_probe = extractAsciiProbe(trimmed_data, enc);
  QString decl_enc = extractXmlDeclEncoding(ascii_probe);

  if (!decl_enc.isEmpty()) {
    return decl_enc.toUpper();
  }

  return enc;
}
