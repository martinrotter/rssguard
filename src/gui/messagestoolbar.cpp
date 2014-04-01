#include "gui/messagestoolbar.h"

#include "definitions/definitions.h"
#include "gui/baselineedit.h"
#include "gui/formmain.h"


MessagesToolBar::MessagesToolBar(const QString &title, QWidget *parent)
  : BaseToolBar(title, parent),
    m_spacer(new QWidget(this)),
    m_txtFilter(new BaseLineEdit(this)) {
  m_spacer->setObjectName(SPACER_OBJECT_NAME);
  m_spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_txtFilter->setObjectName(FILTER_OBJECT_NAME);
  m_txtFilter->setFixedWidth(FILTER_WIDTH);
  m_txtFilter->setPlaceholderText(tr("Filter messages"));

  QMargins margins = contentsMargins();
  margins.setRight(margins.right() + FILTER_RIGHT_MARGIN);
  setContentsMargins(margins);
}

MessagesToolBar::~MessagesToolBar() {
}

void MessagesToolBar::loadChangeableActions() {

  // TODO: udelat dynamicky, nacitat z nastaveni
  // pouzit formmain::allActions treba a ukladat podle "objectname"
  // allactions ale nani qhash, tak pouzit treba
  // http://qt-project.org/doc/qt-4.8/qobject.html#findChild na hledani podle jmena

  addAction(FormMain::instance()->m_ui->m_actionMarkSelectedMessagesAsRead);
  addAction(FormMain::instance()->m_ui->m_actionMarkSelectedMessagesAsUnread);
  addAction(FormMain::instance()->m_ui->m_actionSwitchImportanceOfSelectedMessages);

  addWidget(m_spacer);
  addWidget(m_txtFilter);
}
