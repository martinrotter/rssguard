// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DOMDOCUMENT_H
#define DOMDOCUMENT_H

#include <QDomDocument>

class RSSGUARD_DLLSPEC DomDocument : public QDomDocument {
  public:
    explicit DomDocument();

    bool setContent(const QByteArray& text,
                    bool namespace_processing,
                    QString* error_msg = nullptr,
                    int* error_line = nullptr,
                    int* error_column = nullptr);
    bool setContent(const QString& text,
                    bool namespace_processing,
                    QString* error_msg = nullptr,
                    int* error_line = nullptr,
                    int* error_column = nullptr);

    static QString extractEncoding(const QByteArray& xml_data);
};

#endif // DOMDOCUMENT_H
