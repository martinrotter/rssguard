// For license of this file, see <object-root-folder>/LICENSE.md.

#include "services/gmail/gui/formaddeditemail.h"

#include "services/gmail/gmailserviceroot.h"

FormAddEditEmail::FormAddEditEmail(GmailServiceRoot* root, QWidget* parent) : QDialog(parent), m_root(root) {
  m_ui.setupUi(this);
}

void FormAddEditEmail::execForAdd() {
  exec();
}
