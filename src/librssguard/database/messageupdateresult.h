// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGEUPDATERESULT_H
#define MESSAGEUPDATERESULT_H

#include "core/message.h"
#include "exceptions/applicationexception.h"

#include <exception>

enum class MessageUpdateDetail {
  Summary,
  Detailed
};
enum class MessageSaveAction {
  Insert,
  Update,
  Skip
};
enum class MessageSaveReason {
  NewArticle,
  OrdinaryUpdate,
  ExplicitOverwrite,
  ForcedUpdate,
  NoUpdateTrigger,
  PermanentlyDeleted,
  MissingArticle,
  InvalidArticle
};

struct MessageSaveDecision {
    MessageSaveAction m_action = MessageSaveAction::Skip;
    MessageSaveReason m_reason = MessageSaveReason::NoUpdateTrigger;
    bool m_preserveReadState = false;
    bool m_preserveImportantState = false;
};

enum class MessageWriteOutcome {
  NotAttempted,
  Inserted,
  Updated,
  Skipped,
  Failed
};

// One receipt per input, including skipped articles. IDs alone are not unique
// within a batch when more than one input matches the same stored article.
struct MessageWriteResult {
    int m_inputIndex = -1;
    int m_id = 0;
    MessageSaveDecision m_decision;
    MessageWriteOutcome m_outcome = MessageWriteOutcome::NotAttempted;
    bool m_labelsChanged = false;
    bool m_labelsChecked = false;
    bool m_removedByCleanup = false;
    QString m_error;
};

struct MessageSavePreview {
    int m_inputIndex = -1;
    int m_id = 0;
    MessageSaveDecision m_decision;
    Message m_expectedStored;
    bool m_hasStoredMessage = false;
    bool m_labelsChanged = false;
};

// The single result of persistence. Fetching uses m_all/m_unread as before;
// manual processing also displays receipts. Detailed mode verifies label changes.
// A failed batch retains confirmed writes and the original exception. Callers
// which propagate failures must call throwIfFailed() before using the result.
struct MessageUpdateReport {
    QList<Message> m_unread;
    QList<Message> m_all;
    QList<MessageWriteResult> m_results;
    QString m_error;
    std::exception_ptr m_failure;

    bool succeeded() const {
      return !m_failure && m_error.isEmpty();
    }

    void throwIfFailed() const {
      if (m_failure) {
        std::rethrow_exception(m_failure);
      }
      if (!m_error.isEmpty()) {
        throw ApplicationException(m_error);
      }
    }

    void captureFailure() {
      m_failure = std::current_exception();
      try {
        if (m_failure) {
          std::rethrow_exception(m_failure);
        }
      }
      catch (const ApplicationException& error) {
        m_error = error.message();
      }
      catch (const std::exception& error) {
        m_error = QString::fromUtf8(error.what());
      }
      catch (...) {
        m_error = QStringLiteral("Unknown error while saving articles.");
      }
    }

    int count(MessageWriteOutcome outcome) const {
      int result = 0;
      for (const auto& article : m_results) {
        result += article.m_outcome == outcome;
      }
      return result;
    }

    int labelChanges() const {
      int result = 0;
      for (const auto& article : m_results) {
        result += article.m_labelsChecked && article.m_labelsChanged;
      }
      return result;
    }
};

#endif // MESSAGEUPDATERESULT_H
