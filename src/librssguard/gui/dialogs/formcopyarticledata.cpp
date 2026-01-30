// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formcopyarticledata.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include <QPushButton>
#include <QScrollBar>

FormCopyArticleData::FormCopyArticleData(QAbstractTableModel* model, QWidget* parent) : QDialog(parent) {
  m_ui.setupUi(this);
  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("edit-copy")), tr("Copy article data"));
  setWindowFlags(Qt::WindowType::WindowMinimizeButtonHint | windowFlags());

  auto help_text = helpText(model);

  m_ui.m_help->setHelpText(help_text, false, true);

  m_ui.m_txtPattern
    ->setPlainText(qApp->settings()->value(GROUP(Messages), SETTING(Messages::CopyArticlePattern)).toString());
  m_ui.m_cbEscapeCsv
    ->setChecked(qApp->settings()->value(GROUP(Messages), SETTING(Messages::CopyArticleEscapeCsv)).toBool());
}

QString FormCopyArticleData::helpText(QAbstractTableModel* model) const {
  QString help = tr("These placeholders are replaced by real article data of selected articles. Each article is placed "
                    "on its own line.") +
                 QSL("<br/><br/>");

  for (int clmn = 0; clmn < model->columnCount(); clmn++) {
    help.append(QSL("<b>%%1%</b> - %2<br/>")
                  .arg(QString::number(clmn),
                       model->headerData(clmn, Qt::Orientation::Horizontal, Qt::ItemDataRole::EditRole).toString()));
  }

  return help;
}

FormCopyArticleData::~FormCopyArticleData() {}

std::optional<PatternDecision> FormCopyArticleData::pattern() {
  auto res = exec();

  if (res == QDialog::DialogCode::Accepted) {
    qApp->settings()->setValue(GROUP(Messages), Messages::CopyArticlePattern, m_ui.m_txtPattern->toPlainText());
    qApp->settings()->setValue(GROUP(Messages), Messages::CopyArticleEscapeCsv, m_ui.m_cbEscapeCsv->isChecked());

    return PatternDecision{m_ui.m_txtPattern->toPlainText(), m_ui.m_cbEscapeCsv->isChecked()};
  }
  else {
    return std::nullopt;
  }
}

void FormCopyArticleData::closeEvent(QCloseEvent* event) {
  reject();
  QDialog::closeEvent(event);
}

void FormCopyArticleData::keyPressEvent(QKeyEvent* event) {
  if (event->matches(QKeySequence::StandardKey::Cancel)) {
    close();
  }
  else {
    QDialog::keyPressEvent(event);
  }
}
