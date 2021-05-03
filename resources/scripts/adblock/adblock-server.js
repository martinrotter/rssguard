// Simple local HTTP server providing ad-blocking
// functionality via https://github.com/cliqz-oss/adblocker
//
// How to install:
//     npm i -g @cliqz/adblocker
//     npm i -g concat-stream
//     npm i -g psl
//     npm i -g node-fetch
//
// How to run:
//     NODE_PATH="C:\Users\<user>\AppData\Roaming\npm\node_modules" node ./adblock-server.js
//
// How to use:
//     curl -i -X POST --data '{"url": "http://gompoozu.net", "url_type": "main_frame"}' 'http://localhost:48484'

const fs = require('fs');
const psl = require('psl');
const adblock = require('@cliqz/adblocker')
const http = require('http');
const concat = require('concat-stream');
const constants = require('node:http2');
const fetch = require("node-fetch");

let engine;

adblock.FiltersEngine.fromLists(fetch, [
  'https://easylist.to/easylist/easylist.txt',
  'https://raw.githubusercontent.com/tomasko126/easylistczechandslovak/master/filters.txt',
]).then(function (res) { engine = res; });

const hostname = '127.0.0.1';
const port = 48484;

const server = http.createServer((req, res) => {
  try {
    const chunks = [];
    req.on('data', chunk => chunks.push(chunk));
    req.on('end', () => {
      try {
        const jsonData = Buffer.concat(chunks);
        const jsonStruct = JSON.parse(jsonData.toString());

        const askUrl = jsonStruct['url'];
        const askCosmetic = jsonStruct['cosmetic'];
        const askUrlType = jsonStruct['url_type'];
        const fullUrl = new URL(askUrl);

        resultJson = {};

        const adblockMatch = engine.match(adblock.Request.fromRawDetails({
          type: askUrlType,
          url: askUrl,
        }));

        resultJson["filter"] = adblockMatch;
        console.log(`adblocker: Filter is:\n${JSON.stringify(adblockMatch)}.`)

        if (askCosmetic) {
          const adblockCosmetic = engine.getCosmeticsFilters({
            url: askUrl,
            hostname: fullUrl.hostname,
            domain: psl.parse(fullUrl.hostname).domain
          });

          resultJson["cosmetic"] = adblockCosmetic;
          console.log(`adblocker: Cosmetic is:\n${JSON.stringify(adblockCosmetic)}.`)
        }
        
        res.statusCode = 200;
        res.setHeader('Content-Type', 'application/json');
        res.end(JSON.stringify(resultJson));
      }
      catch (inner_error) {
        console.error(`adblocker: ${inner_error}.`);

        res.statusCode = 500;
        res.setHeader('Content-Type', 'text/plain');
        res.end(String(inner_error));
      }
    })
  }
  catch (error) {
    console.error(`adblocker: ${inner_error}.`);

    res.statusCode = 500;
    res.setHeader('Content-Type', 'text/plain');
    res.end(String(error));
  }
});

server.listen(port, hostname, () => {
  console.log(`adblocker: Server started at local port ${port}.`);
});