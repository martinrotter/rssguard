// Watch entire document and keep date of last DOM change.
const targetNode = document;
const waitTimeMs = 5000;
const idleIdString = "iiddllee";

var scrollings = 3;
var lastResourceTime = new Date();

// Setup DOM observer and observe for changes in elements only.
const config = { attributes: false, characterData: false, childList: true, subtree: true };

const callback = (mutationList, observer) => {
  lastResourceTime = new Date();
  console.log("res " + lastResourceTime);
};

const observer = new MutationObserver(callback);

observer.observe(targetNode, config);

var intervalId = window.setInterval(function () {
  var actualTime = new Date();

  if ((actualTime - lastResourceTime) > waitTimeMs) {
    if (scrollings > 0) {
      // Just scroll to bottom of current DOM
      // to make sure more content is loaded.
      scrollings = scrollings - 1;
      lastResourceTime = new Date();
      window.scrollTo(0, document.body.scrollHeight);
      console.log("scroll");
    }
    else {
      console.log(idleIdString);
      clearInterval(intervalId);
    }
  }
}, 1100);