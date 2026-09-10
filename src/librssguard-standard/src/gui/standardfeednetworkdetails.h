// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STANDARDFEEDNETWORKDETAILS_H
#define STANDARDFEEDNETWORKDETAILS_H

#include "ui_standardfeednetworkdetails.h"

#include <QNetworkProxy>
#include <QWidget>

class Category;
class RootItem;

class StandardFeedNetworkDetails : public QWidget {
    Q_OBJECT

    friend class FormStandardFeedDetails;
    friend class StandardFeedDetails;

  public:
    explicit StandardFeedNetworkDetails(QWidget* parent = nullptr);

    void loadHttpHeaders(const QVariantHash& headers);
    QVariantHash httpHeaders() const;

    void setHttp2Status(NetworkFactory::Http2Status status);
    NetworkFactory::Http2Status http2Status() const;

  private:
    Ui::StandardFeedNetworkDetails m_ui;
};

#endif // STANDARDFEEDEXPDETAILS_H
