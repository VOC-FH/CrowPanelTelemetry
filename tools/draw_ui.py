"""Layout preview: illustrative data; same dashboard rectangles as Ui.cpp."""
from pathlib import Path
import html

root = Path(__file__).resolve().parents[1]
a = ['<svg xmlns="http://www.w3.org/2000/svg" width="1024" height="640" viewBox="0 0 1024 640"><rect width="1024" height="600" fill="#091329"/><rect y="600" width="1024" height="40" fill="#fff"/>']

def rect(x,y,w,h,fill='#122541',radius=14):
    a.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{radius}" fill="{fill}"/>')

def txt(x,y,s,size=18,fill='#edf4ef',weight='normal'):
    a.append(f'<text x="{x}" y="{y}" font-family="DejaVu Sans Mono,monospace" font-size="{size}" fill="{fill}" font-weight="{weight}">{html.escape(s)}</text>')

def button(x,y,w,h,label,accent=False):
    rect(x,y,w,h,'#43e3ad' if accent else '#19334f',10)
    txt(x+w/2-len(label)*5.4,y+h/2+6,label,18,'#091329' if accent else '#edf4ef','bold' if accent else 'normal')

txt(24,44,'FIELD STATION',30,weight='bold')
txt(26,64,'ENERGY / LOCATION / LORAWAN',10,'#8da0ba')
button(802,18,198,44,'WiFi setup')
for n,(label,value) in enumerate([('VOLTAGE','12.34 V'),('CURRENT','0.37 A'),('POWER','4.60 W')]):
    x=24+n*328
    rect(x,84,312,106)
    txt(x+18,116,label,17,'#8da0ba')
    txt(x+18,165,value,34,'#43e3ad' if n==2 else '#edf4ef','bold')
txt(28,204,'Lab WiFi / 192.168.1.42',10,'#8da0ba')
rect(24,210,640,366)
# An illustrative image placeholder, never a bundled or claimed actual JPG.
a.append('<defs><clipPath id="photo"><rect x="44" y="230" width="600" height="270" rx="4"/></clipPath></defs>')
a.append('<g clip-path="url(#photo)"><rect x="44" y="230" width="600" height="270" fill="#a9d8d6"/><circle cx="537" cy="285" r="29" fill="#f5d797"/><path d="M44 391 L181 275 L344 431 L475 316 L644 437 L644 500 L44 500Z" fill="#497f88"/><path d="M44 443 L216 341 L417 487 L644 391 L644 500 L44 500Z" fill="#22495a"/><path d="M44 478 Q236 407 644 475 L644 500 L44 500Z" fill="#102f40"/></g>')
rect(56,242,231,30,'#173b50',6)
txt(68,263,'SD /images/001.jpg',14)
button(44,520,90,46,'<-')
button(144,520,90,46,'->')
button(244,520,190,46,'Play sound',True)
button(444,520,180,46,'Retry SD')
rect(688,210,312,366)
txt(704,244,'GPS POSITION',17,'#43e3ad','bold')
txt(704,271,'48.368200,',17)
txt(704,292,'14.512600 | 9 sats',17)
txt(704,338,'TTN / CLASS A',17,'#43e3ad','bold')
txt(704,365,'Joined / EU868',17)
txt(704,386,'Last batch sent',17)
txt(704,430,'DOWNLINK (tap)',16,'#8da0ba')
txt(704,456,'Port 10: Hello panel',15)
button(704,490,280,56,'Send demo',True)
txt(24,586,'INA226 ready | minute 20 | batch 0/10 | queued 0 | dropped 0',10,'#f3b86d')
txt(24,598,'/images/001.jpg | SD ready',9,'#8da0ba')
txt(24,626,'UI layout preview — illustrative data; firmware raster typography differs.',12,'#344c61')
a.append('</svg>')
(root/'docs/ui-preview.svg').write_text('\n'.join(a))
