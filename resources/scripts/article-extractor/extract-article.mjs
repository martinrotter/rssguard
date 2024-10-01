import { extractFromHtml  } from '@extractus/article-extractor'
import  convertBody from 'fetch-charset-detection';

const input = process.argv[2];

try {
  const res = await fetch(input);
  const body = await res.arrayBuffer();
  const html = convertBody(body, res.headers);
  const article = await extractFromHtml(html, input);
  
  console.log(JSON.stringify(article));
}
catch (err) {
  console.error(err);
}