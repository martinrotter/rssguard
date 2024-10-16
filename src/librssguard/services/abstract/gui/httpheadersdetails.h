// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef HTTPHEADERSDETAILS_H
#define HTTPHEADERSDETAILS_H

#include "network-web/networkfactory.h"
#include "services/abstract/feed.h"

#include <QWidget>

namespace Ui {
  class HttpHeadersDetails;
}

class RSSGUARD_DLLSPEC HttpHeadersDetails : public QWidget {
    Q_OBJECT

  public:
    explicit HttpHeadersDetails(QWidget* parent = nullptr);
    virtual ~HttpHeadersDetails();

    void loadHttpHeaders(const QVariantHash& headers);
    QVariantHash httpHeaders() const;

  private:
    QScopedPointer<Ui::HttpHeadersDetails> m_ui;
};

#endif // HTTPHEADERSDETAILS_H
