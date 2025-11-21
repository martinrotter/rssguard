// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/emailpreviewer.h"

#include "src/definitions.h"
#include "src/gmailnetworkfactory.h"
#include "src/gmailserviceroot.h"
#include "src/gui/formaddeditemail.h"

#include <librssguard/exceptions/networkexception.h>
#include <librssguard/gui/dialogs/filedialog.h>
#include <librssguard/gui/dialogs/formprogressworker.h>
#include <librssguard/gui/messagebox.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/network-web/oauth2service.h>

#include <QJsonObject>

EmailPreviewer::EmailPreviewer(GmailServiceRoot* account, QWidget* parent)
  : CustomMessagePreviewer(parent), m_account(account), m_webView(new WebBrowser(nullptr, this)) {
  m_ui.setupUi(this);

  m_tmrLoadExtraMessageData.setInterval(200);
  m_tmrLoadExtraMessageData.setSingleShot(true);

  m_ui.m_mainLayout->addWidget(dynamic_cast<QWidget*>(m_webView.data()), 3, 0, 1, -1);
  m_ui.m_btnAttachments->setIcon(qApp->icons()->fromTheme(QSL("mail-attachment")));
  m_ui.m_btnForward->setIcon(qApp->icons()->fromTheme(QSL("mail-forward")));
  m_ui.m_btnReply->setIcon(qApp->icons()->fromTheme(QSL("mail-reply-sender")));

  QMenu* menu_attachments = new QMenu(this);

  m_ui.m_btnAttachments->setMenu(menu_attachments);

  m_webView->setToolBarVisible(false);

  connect(menu_attachments, &QMenu::triggered, this, &EmailPreviewer::downloadAttachment);
  connect(m_ui.m_btnReply, &QToolButton::clicked, this, &EmailPreviewer::replyToEmail);
  connect(m_ui.m_btnForward, &QToolButton::clicked, this, &EmailPreviewer::forwardEmail);

  connect(&m_tmrLoadExtraMessageData, &QTimer::timeout, this, &EmailPreviewer::loadExtraMessageData);
}

EmailPreviewer::~EmailPreviewer() {
  qDebugNN << LOGSEC_GMAIL << "Email previewer destroyed.";
}

WebBrowser* EmailPreviewer::webBrowser() const {
  return m_webView.data();
}

void EmailPreviewer::clear() {
  m_tmrLoadExtraMessageData.stop();
  m_webView->clear(false);
}

void EmailPreviewer::loadMessage(const Message& msg, RootItem* selected_item) {
  m_message = msg;
  m_webView->loadMessage({msg}, selected_item);

  m_ui.m_tbFrom->setText(msg.m_author);
  m_ui.m_tbSubject->setText(msg.m_title);
  m_ui.m_tbTo->setText(QSL("-"));

  m_ui.m_btnAttachments->menu()->clear();

  for (const QSharedPointer<MessageEnclosure>& att : msg.m_enclosures) {
    const QStringList att_id_name = att->url().split(QSL(GMAIL_ATTACHMENT_SEP));

    m_ui.m_btnAttachments->menu()->addAction(att->mimeType())->setData(att_id_name);
  }

  m_ui.m_btnAttachments->setDisabled(m_ui.m_btnAttachments->menu()->isEmpty());

  m_tmrLoadExtraMessageData.start();
}

void EmailPreviewer::loadExtraMessageData() {
  try {
    m_ui.m_tbTo->setText(m_account->network()->getMessageMetadata(m_message.m_customId,
                                                                  {QSL("TO")},
                                                                  m_account->networkProxy())["To"]);
  }
  catch (const ApplicationException& ex) {
    qWarningNN << LOGSEC_GMAIL << "Cannot load extra article metadata:" << QUOTE_W_SPACE_DOT(ex.message());
  }
}

void EmailPreviewer::replyToEmail() {
  FormAddEditEmail(m_account, window()).show(FormAddEditEmail::Mode::Reply, &m_message);
}

void EmailPreviewer::forwardEmail() {
  FormAddEditEmail(m_account, window()).show(FormAddEditEmail::Mode::Forward, &m_message);
}

void EmailPreviewer::downloadAttachment(QAction* act) {
  const QString attachment_id = act->data().toStringList().at(1);
  const QString file_name = act->data().toStringList().at(0);

  const QString save_file_name = FileDialog::saveFileName(qApp->mainFormWidget(),
                                                          tr("Save attachment %1").arg(file_name),
                                                          qApp->documentsFolder(),
                                                          file_name,
                                                          {},
                                                          nullptr,
                                                          GENERAL_REMEMBERED_PATH);

  if (save_file_name.isEmpty()) {
    return;
  }

  try {
    FormProgressWorker wrkr(qApp->mainFormWidget());
    QByteArray data;

    wrkr.doSingleWork(
      tr("Download attachment"),
      false,
      [&](QFutureWatcher<void>& rprt) {
        Downloader dwnl;
        QEventLoop loop;
        QString target_url = QSL(GMAIL_API_GET_ATTACHMENT).arg(m_message.m_customId, attachment_id);
        QByteArray bearer = m_account->network()->oauth()->bearer().toLocal8Bit();

        /*
        if (bearer.isEmpty()) {
          throw NetworkException(QNetworkReply::NetworkError::AuthenticationRequiredError);
        }
        */

        emit rprt.progressRangeChanged(0, 0);

        QObject::connect(&dwnl, &Downloader::completed, &loop, &QEventLoop::quit);
        QObject::connect(&dwnl, &Downloader::progress, &rprt, [&](qint64 bytes_received, qint64 bytes_total) {
          emit rprt.progressValueChanged(bytes_received / 1000.0);
        });

        dwnl.appendRawHeader(QSL(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(), bearer);
        dwnl.downloadFile(target_url);
        loop.exec();

        data = dwnl.lastOutputData();
      },
      [](int progress) {
        return tr("Downloaded %1 kB...").arg(progress);
      });

    const QString dt = QJsonDocument::fromJson(data).object()[QSL("data")].toString();

    IOFactory::writeFile(save_file_name,
                         QByteArray::fromBase64(dt.toLocal8Bit(), QByteArray::Base64Option::Base64UrlEncoding));
  }
  catch (const NetworkException&) {
    MsgBox::show({},
                 QMessageBox::Icon::Critical,
                 tr("Cannot download attachment"),
                 tr("Attachment cannot be downloaded because you are not logged-in."));
  }
  catch (const ApplicationException& ex) {
    MsgBox::show({},
                 QMessageBox::Icon::Critical,
                 tr("Cannot download attachment"),
                 tr("Attachment cannot be downloaded because some general error happened."),
                 {},
                 ex.message());
  }
}
