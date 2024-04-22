import { extract } from '@extractus/article-extractor'

const input = process.argv[2];

try {
  const article = await extract(input);
  console.log(JSON.stringify(article));
}
catch (err) {
  console.error(err);
}