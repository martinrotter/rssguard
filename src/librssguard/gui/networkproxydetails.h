// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NETWORKPROXYDETAILS_H
#define NETWORKPROXYDETAILS_H

#include <QWidget>

#include "ui_networkproxydetails.h"

class NetworkProxyDetails : public QWidget {
  Q_OBJECT

  public:
    explicit NetworkProxyDetails(QWidget* parent = nullptr);

  private slots:
    void displayProxyPassword(int state);
    void onProxyTypeChanged(int index);

  public:
    Ui::NetworkProxyDetails m_ui;
};

#endif // NETWORKPROXYDETAILS_H
