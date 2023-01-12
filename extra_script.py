import re
from pathlib import Path

def html_as_raw_string_literal(source):
    varName = re.sub(r'\.', "_", source.name)
    varName = varName.upper()
    with open(source, encoding="UTF-8") as src: 
        res = "const char " + varName + "[] = R\"("
        res += src.read()
        res = re.sub(r'\/\/.*\n', "", res)
        res = re.sub(r'\/\*(\n|.)*?\*\/', "", res)
        res = re.sub(r'[ ]+', " ", res)
        res = re.sub(r'[\n]+', "", res)
        res = re.sub(r'\n ', "\n", res)
        res = re.sub(r';}', "}", res)
        res += ")\";"
        with open(str(source)+".cpp", mode="w", encoding="UTF-8") as dest:
            dest.write(res)

def convert_html():
    for path in Path('.').rglob('*.html'):
        html_as_raw_string_literal(path)
    for path in Path('.').rglob('*.js'):
        html_as_raw_string_literal(path)

convert_html()