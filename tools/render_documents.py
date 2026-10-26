"""Render the two course Markdown files with Pandoc/XeLaTeX; no installs or uploads."""
from pathlib import Path
import os
import re
import subprocess

root = Path(__file__).resolve().parents[1]
build = Path(os.environ.get('BUILD', '/tmp/codex/build/CrowPanelTelemetry/student-pdfs'))
build.mkdir(parents=True, exist_ok=True)
output = root / 'output/pdf'
output.mkdir(parents=True, exist_ok=True)
header = build / 'header.tex'
header.write_text(r'''
\usepackage{fvextra}
\usepackage{needspace}
\usepackage{etoolbox}
\usepackage{fancyhdr}
\DefineVerbatimEnvironment{Highlighting}{Verbatim}{breaklines,breakanywhere,breaknonspaceingroup=true,commandchars=\\\{\},fontsize=\scriptsize}
\fvset{breaklines=true,breakanywhere=true,breaknonspaceingroup=true,fontsize=\scriptsize}
\setlength{\emergencystretch}{3em}
\setlength{\tabcolsep}{4pt}
\renewcommand{\arraystretch}{1.15}
\AtBeginEnvironment{longtable}{\footnotesize}
\AtBeginEnvironment{Shaded}{\needspace{4\baselineskip}}
\pretocmd{\section}{\clearpage}{}{}
\pagestyle{fancy}
\fancyhf{}
\fancyhead[L]{\small CrowPanelTelemetry}
\fancyhead[R]{\small Student edition}
\fancyfoot[C]{\small\thepage}
\setlength{\headheight}{14pt}
\AtBeginDocument{\hypersetup{breaklinks=true}}
''')
filter_path = build / 'pdf.lua'
filter_path.write_text(r'''
local directory = 'docs'
function Meta(meta)
  directory = pandoc.utils.stringify(meta['source-directory'])
  return meta
end
function Link(el)
  local file, anchor = el.target:match('^(.-)%.md(#.*)$')
  if not file then file = el.target:match('^(.-)%.md$') end
  if file then
    local name = file:match('([^/]+)$')
    if name == 'documentation' or name == 'learning-material' then
      el.target = name .. '.pdf' .. (anchor or '')
    else
      el.target = 'https://github.com/VOC-FH/CrowPanelTelemetry/blob/main/' .. (el.target:match('^%.%./') and el.target:gsub('^%.%./', '') or directory .. '/' .. el.target)
    end
  elseif not el.target:match('^[%a]+:') and not el.target:match('^#') then
    el.target = 'https://github.com/VOC-FH/CrowPanelTelemetry/blob/main/' .. (el.target:match('^%.%./') and el.target:gsub('^%.%./', '') or directory .. '/' .. el.target)
  end
  return el
end
local escapes = {['\\']='\u{005c}textbackslash{}', ['{']='\\{', ['}']='\\}',
 ['%']='\\%', ['&']='\\&', ['#']='\\#', ['$']='\\$', ['_']='\\_',
 ['~']='\\textasciitilde{}', ['^']='\\textasciicircum{}'}
function Image(el)
  if not el.src:match('^/') and not el.src:match('^[%a]+:') then
    el.src = directory .. '/' .. el.src
  end
  return el
end
function Blocks(blocks)
  local result = pandoc.List()
  for _, block in ipairs(blocks) do
    result:insert(block)
    if block.tag == 'Header' and block.identifier ~= '' then
      result:insert(pandoc.RawBlock('latex', '\\hypertarget{' .. block.identifier .. '}{}'))
    end
  end
  return result
end
function Code(el)
  local pieces = {}
  for _, cp in utf8.codes(el.text) do
    local c = utf8.char(cp)
    table.insert(pieces, escapes[c] or c)
    if c:match('[_/%.:%-,=]') then table.insert(pieces, '\\allowbreak{}') end
  end
  return pandoc.RawInline('latex', '\\texttt{' .. table.concat(pieces) .. '}')
end
return {{Meta=Meta}, {Link=Link, Code=Code, Image=Image}, {Blocks=Blocks}}
''')
# Render the existing Mermaid data-flow figure with Graphviz for the LaTeX edition.
source = (root / 'docs/documentation.md').read_text()
diagram = re.search(r'```mermaid\n(.*?)```', source, re.S).group(1)
nodes = dict(re.findall(r'(\w+)\[([^]]+)\]', diagram))
edges = []
for line in diagram.splitlines():
    cleaned = re.sub(r'\[[^]]+\]', '', line.strip())
    edge = re.fullmatch(r'(\w+)\s*-->\s*(\w+)', cleaned)
    if edge:
        edges.append(edge.groups())
dot = build / 'data-flow.dot'
dot.write_text('digraph { rankdir=TB; graph [bgcolor="transparent", ranksep=0.3]; '
               'node [shape=box, style="rounded,filled", fillcolor="#EDF3F7", '
               'fontname="DejaVu Sans", fontsize=10]; edge [color="#53758A"];\n'
               + '\n'.join(f'{key} [label="{value}"];' for key, value in nodes.items())
               + '\n' + '\n'.join(f'{a} -> {b};' for a, b in edges) + '\n}\n')
subprocess.run(['dot', '-Tpdf', str(dot), '-o', str(build / 'data-flow.pdf')], check=True)
for relative in ['docs/documentation.md', 'learning/learning-material.md']:
    original = root / relative
    text = original.read_text()
    if original.name == 'documentation.md':
        text = re.sub(r'```mermaid\n.*?```',
                      lambda _: f'![How measurements and requests move through the program]({build / "data-flow.pdf"}){{width=95%}}',
                      text, flags=re.S)
    # Keep Markdown sources unchanged. Explicit anchors become LaTeX destinations.
    prepared = build / original.name
    text = re.sub(r'<a id="([^"]+)"></a>\n\n(#{1,6} [^\n]+)',
                  lambda match: match[2] + ' {#' + match[1] + '}', text)
    prepared.write_text(text)
    command = ['pandoc', str(prepared), '--from=markdown+raw_html', '--to=latex',
               '--standalone', '--toc', '--toc-depth=2', '--syntax-highlighting=tango',
               '--lua-filter', str(filter_path), '--include-in-header', str(header), '--resource-path', str(original.parent),
               '-V', 'mainfont=DejaVu Sans', '-V', 'sansfont=DejaVu Sans',
               '-V', 'monofont=DejaVu Sans Mono', '-V', 'fontsize=11pt',
               '-V', 'geometry:margin=18mm', '-V', 'papersize=a4',
               '-V', 'colorlinks=true', '-V', 'linkcolor=teal', '-V', 'urlcolor=teal',
               '-V', 'linestretch=1.08',
               '--metadata', 'lang=en-GB', '--metadata', 'source-directory=' + original.parent.name, '--metadata', 'title=' +
               ('CrowPanelTelemetry - Documentation' if original.name == 'documentation.md'
                else 'CrowPanelTelemetry - Learning material'),
               '--output', str(build / (original.stem + '.tex'))]
    env = dict(os.environ, TMPDIR=str(build), TEXMFOUTPUT=str(build))
    result = subprocess.run(command, cwd=root, env=env, capture_output=True, text=True)
    (build / (original.stem + '-render.log')).write_text(result.stdout + result.stderr)
    print(original.name, 'PASS' if result.returncode == 0 else 'FAIL', flush=True)
    if result.returncode:
        print(result.stderr)
    result.check_returncode()

    for run in range(3):
        latex = subprocess.run(['xelatex', '-interaction=nonstopmode', '-halt-on-error',
                                '-output-directory', str(build), str(build / (original.stem + '.tex'))],
                               cwd=root, env=env, capture_output=True, text=True)
        (build / f'{original.stem}-latex-{run + 1}.log').write_text(latex.stdout + latex.stderr)
        if latex.returncode:
            print(latex.stdout[-4000:])
        latex.check_returncode()
    (output / (original.stem + '.pdf')).write_bytes((build / (original.stem + '.pdf')).read_bytes())
    print(original.stem + '.pdf rendered', flush=True)
