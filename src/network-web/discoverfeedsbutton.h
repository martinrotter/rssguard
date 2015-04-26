#ifndef DISCOVERFEEDSBUTTON_H
#define DISCOVERFEEDSBUTTON_H

#include <QToolButton>


class DiscoverFeedsButton : public QToolButton {
    Q_OBJECT

  public:
    explicit DiscoverFeedsButton(QWidget *parent = 0);
    virtual ~DiscoverFeedsButton();

    void clearFeedAddresses();
    void setFeedAddresses(const QStringList &addresses);
};

#endif // DISCOVERFEEDSBUTTON_H
