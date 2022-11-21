// This filter accepts only messages whose title
// is on the whitelist.

var whitelist = [
  'abc',
  '123'
];

function filterMessage() {
  if (whitelist.some(i => msg.title.indexOf(i) != -1)) {
    return MessageObject.Accept;
  } else {
    return MessageObject.Ignore;
  }
}