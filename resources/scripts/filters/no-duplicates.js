// This filter ensures that there are no duplicate
// articles received across all feeds.
//
// Remember you have to assign this filter to all your feeds
// you want to de-duplicate.

function filterMessage() {
  if (msg.isAlreadyInDatabase(MessageObject.SameTitle | MessageObject.AllFeedsSameAccount)) {
    return MessageObject.Purge;
  }
  else {
    return MessageObject.Accept;
  }
}