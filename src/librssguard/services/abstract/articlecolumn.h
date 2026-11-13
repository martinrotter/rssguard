// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ARTICLECOLUMN_H
#define ARTICLECOLUMN_H

#include <QString>

struct RSSGUARD_DLLSPEC ArticleColumn {
    QString m_id;
    QString m_title;
    int m_existingColumn = -1;

    bool operator==(const ArticleColumn& other) const {
      return m_id == other.m_id && m_title == other.m_title && m_existingColumn == other.m_existingColumn;
    }
};

#endif // ARTICLECOLUMN_H
