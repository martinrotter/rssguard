var rd = require('@mozilla/readability');
var fs = require('fs');
var jsd = require('jsdom');
var iconvlite = require('iconv-lite');
var data = fs.readFileSync(0);
var encData = iconvlite.decode(data, "utf-8");
var doc = new jsd.JSDOM(encData);

var article = new rd.Readability(doc.window.document).parse();
console.log(article['content']);