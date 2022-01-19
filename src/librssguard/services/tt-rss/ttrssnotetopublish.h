// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TTRSSNOTETOPUBLISH_H
#define TTRSSNOTETOPUBLISH_H

#include <QString>

struct TtRssNoteToPublish {
  public:
    QString m_title;
    QString m_url;
    QString m_content;
};

#endif // TTRSSNOTETOPUBLISH_H
