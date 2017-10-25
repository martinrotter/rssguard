// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DISCOVERFEEDSBUTTON_H
#define DISCOVERFEEDSBUTTON_H

#include <QToolButton>

class DiscoverFeedsButton : public QToolButton {
  Q_OBJECT

  public:

    // Constructors.
    explicit DiscoverFeedsButton(QWidget* parent = 0);
    virtual ~DiscoverFeedsButton();

    // Feed addresses manipulators.
    void clearFeedAddresses();
    void setFeedAddresses(const QStringList& addresses);

  private slots:

    // User chose any of addresses.
    void linkTriggered(QAction* action);
    void fillMenu();

  private:
    QStringList m_addresses;
};

#endif // DISCOVERFEEDSBUTTON_H
