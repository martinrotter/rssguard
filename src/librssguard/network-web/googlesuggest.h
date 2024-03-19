// For license of this file, see <project-root-folder>/LICENSE.md.

// You may use this file under the terms of the BSD license as follows:
//
// "Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in
//     the documentation and/or other materials provided with the
//     distribution.
//   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
//     of its contributors may be used to endorse or promote products derived
//     from this software without specific prior written permission.
//
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."

#ifndef GOOGLESUGGEST_H
#define GOOGLESUGGEST_H

#include "network-web/downloader.h"

#include <QListWidget>
#include <QNetworkReply>
#include <QObject>

class LocationLineEdit;
class QTimer;

class GoogleSuggest : public QObject {
    Q_OBJECT

  public:
    explicit GoogleSuggest(LocationLineEdit* editor, QObject* parent = nullptr);

    virtual bool eventFilter(QObject* object, QEvent* event);

  public slots:
    void showCompletion(const QStringList& choices);
    void doneCompletion();
    void preventSuggest();
    void autoSuggest();
    void handleNetworkData(const QUrl& url,
                           QNetworkReply::NetworkError status,
                           int http_code,
                           const QByteArray& contents);

  private:
    LocationLineEdit* m_editor;
    QScopedPointer<Downloader> m_downloader;
    QScopedPointer<QListWidget> m_popup;
    QTimer* m_timer;
    QString m_enteredText;
};

#endif // GOOGLESUGGEST_H
