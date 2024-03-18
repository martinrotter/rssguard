// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NETWORKPROXYDETAILS_H
#define NETWORKPROXYDETAILS_H

#include <QWidget>

#include <QNetworkProxy>

namespace Ui {
  class NetworkProxyDetails;
}

class NetworkProxyDetails : public QWidget {
    Q_OBJECT

  public:
    explicit NetworkProxyDetails(QWidget* parent = nullptr);
    virtual ~NetworkProxyDetails();

    QNetworkProxy proxy() const;
    void setProxy(const QNetworkProxy& proxy);

  signals:
    void changed();

  private slots:
    void onProxyTypeChanged(int index);

  private:
    QScopedPointer<Ui::NetworkProxyDetails> m_ui;
};

#endif // NETWORKPROXYDETAILS_H
