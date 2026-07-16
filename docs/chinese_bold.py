# chinese_bold.py
import re
from markdown.extensions import Extension
from markdown.inlinepatterns import InlineProcessor
import xml.etree.ElementTree as etree

class ChineseBoldProcessor(InlineProcessor):
    """匹配中文**加粗**场景：前面是中文或行首"""
    RE = re.compile(r'(?<=\S)(?<!\s)\*\*([^\s].*?)\*\*')

    def handleMatch(self, m, data):
        el = etree.Element('strong')
        el.text = m.group(1)
        return el, m.start(), m.end()

class ChineseBoldExtension(Extension):
    def extendMarkdown(self, md):
        # 优先级设高，在默认 strong_em 之前处理
        md.inlinePatterns.register(
            ChineseBoldProcessor(r'(?<=\S)(?<!\s)\*\*([^\s].*?)\*\*', md),
            'chinese-bold', 65
        )

def makeExtension(**kwargs):
    return ChineseBoldExtension(**kwargs)
