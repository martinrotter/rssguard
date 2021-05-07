// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DISCOVERFEEDSBUTTON_H
#define DISCOVERFEEDSBUTTON_H

#include <QToolButton>

class DiscoverFeedsButton : public QToolButton {
  Q_OBJECT

  public:
    explicit DiscoverFeedsButton(QWidget* parent = nullptr);
    virtual ~DiscoverFeedsButton();

    void clearFeedAddresses();
    void setFeedAddresses(const QStringList& addresses);

  private slots:
    void linkTriggered(QAction* action);
    void fillMenu();

  private:
    QStringList m_addresses;
};

#endif // DISCOVERFEEDSBUTTON_H
