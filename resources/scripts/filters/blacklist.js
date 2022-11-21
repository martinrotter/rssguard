// This filter rejects all messages whose title
// is on the blacklist.

var blacklist = [
  'abc',
  '123'
];

function filterMessage() {
  if (blacklist.some(i => msg.title.indexOf(i) != -1)) {
    return MessageObject.Ignore;
  } else {
    return MessageObject.Accept;
  }
}