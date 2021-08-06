// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NETWORKPROXYDETAILS_H
#define NETWORKPROXYDETAILS_H

#include <QWidget>

#include "ui_networkproxydetails.h"

#include <QNetworkProxy>

class NetworkProxyDetails : public QWidget {
  Q_OBJECT

  public:
    explicit NetworkProxyDetails(QWidget* parent = nullptr);

    QNetworkProxy proxy() const;
    void setProxy(const QNetworkProxy& proxy);

  signals:
    void changed();

  private slots:
    void onProxyTypeChanged(int index);

  private:
    Ui::NetworkProxyDetails m_ui;
};

#endif // NETWORKPROXYDETAILS_H
