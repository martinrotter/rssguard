// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/gui/httpheadersdetails.h"

#include "definitions/definitions.h"

#include "ui_httpheadersdetails.h"

#include <QRegularExpression>

HttpHeadersDetails::HttpHeadersDetails(QWidget* parent) : QWidget(parent), m_ui(new Ui::HttpHeadersDetails()) {
  m_ui->setupUi(this);
  m_ui->m_helpHttpHeaders
    ->setHelpText(tr("Enter each key/value HTTP header pair on separate line. Note that all spaces are significant "
                     "and that header names are case-sensitive. "
                     "Also, make sure to separate key from value with '=', like the example below:") +
                    QSL("<br/><br/><b>HeaderKey=HeaderValue</b>"),
                  false);
}

void HttpHeadersDetails::loadHttpHeaders(const QVariantHash& headers) {
  m_ui->m_txtHttpHeaders->clear();

  for (auto i = headers.cbegin(), end = headers.cend(); i != end; i++) {
    m_ui->m_txtHttpHeaders->appendPlainText(QSL("%1=%2").arg(i.key(), i.value().toString()));
  }
}

QVariantHash HttpHeadersDetails::httpHeaders() const {
  QVariantHash h;

  QRegularExpression exp(QSL("^([^=]+)=(.+)$"), QRegularExpression::PatternOption::MultilineOption);
  auto exp_match = exp.globalMatch(m_ui->m_txtHttpHeaders->toPlainText());

  while (exp_match.hasNext()) {
    auto match = exp_match.next();

    h.insert(match.captured(1).trimmed(), match.captured(2).trimmed());
  }

  return h;
}

HttpHeadersDetails::~HttpHeadersDetails() = default;
