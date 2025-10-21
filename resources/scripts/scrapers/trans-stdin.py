import sys
import asyncio
from googletrans import Translator


async def translate_string(to_translate, lang_src, lang_dest):
    async with Translator() as translator:
        translated_text = await translator.translate(to_translate, src=lang_src, dest=lang_dest)
        print("'" + translated_text.text.strip() + "'")


lang_from = sys.argv[1]
lang_to = sys.argv[2]

sys.stdin.reconfigure(encoding="utf-8")
sys.stdout.reconfigure(encoding="utf-8")

data = sys.stdin.read()

asyncio.run(translate_string(data, lang_from, lang_to))
