// For license of this file, see <project-root-folder>/LICENSE.md.
//
// This code is heavily inspired by QXmlInputSource::fromRawData() function
// from Qt sources. All credit goes to its authors.

#pragma once
#include <QByteArray>
#include <QString>
#include <QtGlobal>

class RSSGUARD_DLLSPEC XmlEncodingDetector {
  public:
    XmlEncodingDetector() = delete;

    static QString detectXmlEncoding(const QByteArray& data);

  private:
    static QString extractXmlDeclEncoding(const QString& text);
    static QString detectBomEncoding(const QByteArray& data);
    static QString extractAsciiProbe(const QByteArray& data, const QString& provisional_enc);
};
