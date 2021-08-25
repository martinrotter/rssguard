// Simple local HTTP server providing ad-blocking functionality via https://github.com/cliqz-oss/adblocker
//
// How to install:
//     npm i -g @cliqz/adblocker
//     npm i -g concat-stream
//     npm i -g tldts-experimental
//     npm i -g node-fetch
//
// How to run:
//     NODE_PATH="C:\Users\<user>\AppData\Roaming\npm\node_modules" node ./adblock-server.js "<port>" "<filters-file-path>"
//
// How to use:
//     curl -i -X POST --data '
//       {
//         "url": "http://gompoozu.net/cwqcwq/js.js",
//         "fp_url": "http://bbc.com",
//         "url_type": "main_frame",
//         "filter": true,
//         "cosmetic": true
//       }' 'http://localhost:<port>'

const fs = require('fs');
const tldts = require('tldts-experimental');
const adblock = require('@cliqz/adblocker')
const http = require('http');
const concat = require('concat-stream');
const constants = require('node:http2');
const fetch = require("node-fetch");
const cluster = require('cluster');

const numCPUs = require('os').cpus().length;
const port = process.argv[2];
const filtersFile = process.argv[3];
const engine = adblock.FiltersEngine.parse(fs.readFileSync(filtersFile, 'utf-8'));
const hostname = '127.0.0.1';

if (cluster.isPrimary) {
  console.log(`Primary ${process.pid} is running`);

  // Fork workers.
  for (let i = 0; i < numCPUs; i++) {
    cluster.fork();
  }

  cluster.on('exit', (worker, code, signal) => {
    console.log(`worker ${worker.process.pid} died`);
  });
}
else {
  const server = http.createServer((req, res) => {
    try {
      console.log(new Date());

      const chunks = [];
      req.on('data', chunk => chunks.push(chunk));
      req.on('end', () => {
        console.log(new Date());

        try {
          const jsonData = Buffer.concat(chunks);
          const jsonStruct = JSON.parse(jsonData.toString());

          const askUrl = jsonStruct['url'];
          const askFpUrl = jsonStruct['fp_url'];
          const askFilter = jsonStruct['filter'];
          const askCosmetic = jsonStruct['cosmetic'];
          const askUrlType = jsonStruct['url_type'];
          const fullUrl = new URL(askUrl);

          resultJson = {};

          if (askFilter) {
            const adblockMatch = engine.match(adblock.Request.fromRawDetails({
              type: askUrlType,
              url: askUrl,
              sourceUrl: askFpUrl
            }));

            resultJson["filter"] = adblockMatch;
            console.log(`adblocker: Filter is:\n${JSON.stringify(adblockMatch)}.`)
          }

          if (askCosmetic) {
            const adblockCosmetic = engine.getCosmeticsFilters({
              url: askUrl,
              hostname: fullUrl.hostname,
              domain: tldts.getDomain(fullUrl.hostname)
            });

            resultJson["cosmetic"] = adblockCosmetic;
            console.log(`adblocker: Cosmetic is:\n${JSON.stringify(adblockCosmetic)}.`)
          }

          res.statusCode = 200;
          res.setHeader('Content-Type', 'application/json');
          res.end(JSON.stringify(resultJson));

          console.log(new Date());
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
}
