#ifndef WEBFACTORY_H
#define WEBFACTORY_H

#include <QObject>
#include <QPointer>


class WebFactory : public QObject {
    Q_OBJECT

  public:
    // Destructor.
    virtual ~WebFactory();

    // Singleton getter.
    static WebFactory *instance();

  signals:
    void javascriptSwitched(bool enabled);
    void pluginsSwitched(bool enabled);
    void imagesLoadingSwitched(bool enabled);

  private:
    // Constructor.
    explicit WebFactory(QObject *parent = 0);

    // Singleton.
    static QPointer<WebFactory> s_instance;
};

#endif // WEBFACTORY_H
