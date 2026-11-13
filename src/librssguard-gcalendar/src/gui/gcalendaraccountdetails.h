// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GCALENDARACCOUNTDETAILS_H
#define GCALENDARACCOUNTDETAILS_H

#include "src/gcalendarnetworkfactory.h"

#include "ui_gcalendaraccountdetails.h"

#include <QFutureWatcher>
#include <QPointer>
#include <QWidget>

class GCalendarAccountDetails : public QWidget {
    Q_OBJECT

  public:
    explicit GCalendarAccountDetails(QWidget* parent = nullptr, GCalendarNetworkFactory::Transport transport = {});
    virtual ~GCalendarAccountDetails() override;

    void testSetup(const QNetworkProxy& proxy);
    bool isTesting() const;
    QString testedRefreshToken() const;
    Ui::GCalendarAccountDetails m_ui;

  public slots:
    void invalidateTest();

  signals:
    void setupTestFinished(bool succeeded);
    void testRunningChanged(bool running);

  protected:
    virtual void startAuthorization(OAuth2Service* oauth);

  private:
    struct TestResult {
        QString m_error;
    };
    void validateFields();
    void setTesting(bool testing);
    void testApi(const QString& access, const QString& refresh);
    void finishTest(bool succeeded, const QString& message);

    GCalendarNetworkFactory::Transport m_transport;
    GCalendarNetworkFactory::Context m_context;
    QPointer<OAuth2Service> m_oauth;
    QFutureWatcher<TestResult>* m_check = nullptr;
    QString m_testedId;
    QString m_testedSecret;
    QString m_refreshToken;
    QString m_candidateRefreshToken;
    bool m_testing = false;
};

#endif // GCALENDARACCOUNTDETAILS_H
