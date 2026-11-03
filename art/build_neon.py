"""Generate the indexed Neon Arcade scene and a 240px animated HTML preview.
Python standard library only. Run from any directory; no external artwork/fonts.
"""
from pathlib import Path
import math, struct, zlib, json, base64
ROOT=Path(__file__).resolve().parents[1]
COLORS=['#090e1e','#141d35','#f7faff','#a0aecd','#21e6ff','#090e1e','#bcff40','#ff6b8a','#b68cff','#ffe36d','#ff9d4d','#5678ff','#154154','#352547','#324426','#28365a']
BG,CARD,WHITE,GREY,CYAN,BLACK,LIME,PINK,PURPLE,YELLOW,ORANGE,BLUE,TEAL,PLUM,OLIVE,GRID=range(16)
FONT={
'A':'010101111101101','B':'110101110101110','C':'011100100100011','D':'110101101101110','E':'111100110100111','F':'111100110100100','G':'011100101101011','H':'101101111101101','I':'111010010010111','J':'001001001101010','K':'101101110101101','L':'100100100100111','M':'101111111101101','N':'101111111111101','O':'010101101101010','P':'110101110100100','Q':'010101101111011','R':'110101110101101','S':'011100010001110','T':'111010010010010','U':'101101101101111','V':'101101101101010','W':'101101111111101','X':'101101010101101','Y':'101101010010010','Z':'111001010100111',
'0':'111101101101111','1':'010110010010111','2':'110001010100111','3':'110001010001110','4':'101101111001001','5':'111100110001110','6':'011100111101111','7':'111001010010010','8':'111101111101111','9':'111101111001110',' ':'000000000000000','-':'000000111000000','_':'000000000000111','.':'000000000000010','/':'001001010100100','<':'001010100010001','>':'100010001010100','?':'110001010000010','!':'010010010000010'}
class Canvas:
 def __init__(self,w=120,h=120): self.w=w;self.h=h;self.p=[[BG]*w for _ in range(h)]
 def dot(self,x,y,c):
  if 0<=x<self.w and 0<=y<self.h:self.p[y][x]=c
 def rect(self,x,y,w,h,c):
  for yy in range(y,y+h):
   for xx in range(x,x+w):self.dot(xx,yy,c)
 def line(self,x0,y0,x1,y1,c):
  steps=max(abs(x1-x0),abs(y1-y0),1)
  for i in range(steps+1):self.dot(round(x0+(x1-x0)*i/steps),round(y0+(y1-y0)*i/steps),c)
 def poly(self,points,c):
  for y in range(min(p[1] for p in points),max(p[1] for p in points)+1):
   cuts=[]
   for (x0,y0),(x1,y1) in zip(points,points[1:]+points[:1]):
    if y0<=y<y1 or y1<=y<y0:cuts.append(x0+(y-y0)*(x1-x0)/(y1-y0))
   cuts.sort()
   for i in range(0,len(cuts)-1,2):self.rect(math.ceil(cuts[i]),y,math.floor(cuts[i+1])-math.ceil(cuts[i])+1,1,c)
 def text(self,s,x,y,c,scale=1):
  for char in s.upper():
   bits=FONT.get(char,FONT['?'])
   for i,v in enumerate(bits):
    if v=='1':self.rect(x+i%3*scale,y+i//3*scale,scale,scale,c)
   x+=4*scale
 def box(self,x,y,w,h,c):self.line(x,y,x+w-1,y,c);self.line(x,y+h-1,x+w-1,y+h-1,c);self.line(x,y,x,y+h-1,c);self.line(x+w-1,y,x+w-1,y+h-1,c)
 def png(self,scale=1):
  def chunk(t,b):return struct.pack('>I',len(b))+t+b+struct.pack('>I',zlib.crc32(t+b)&0xffffffff)
  palette=[bytes.fromhex(c[1:]) for c in COLORS]
  raw=b''.join((b'\0'+b''.join(palette[c]*scale for c in row))*scale for row in self.p)
  return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',self.w*scale,self.h*scale,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b'')

def background():
 a=Canvas()
 # Layered skyline, rooftop antennas and distant apartment windows.
 for x,y,w,c in [(1,2,19,PLUM),(25,8,16,GRID),(43,1,14,CARD),(60,16,16,PLUM),(78,5,18,CARD),(101,0,19,GRID)]:
  a.rect(x,y,w,72-y,c);a.line(x+w//2,y-5,x+w//2,y,BLUE)
  for yy in range(y+4,65,7):
   for xx in range(x+3,x+w-2,5):a.rect(xx,yy,2,3,TEAL if (xx+yy)%3 else PURPLE)
 a.rect(56,22,25,44,BG)
 for y in range(25,65,6):a.rect(59,y,2,2,YELLOW);a.rect(74,y,3,2,BLUE)
 # Perspective alley; wet asphalt and narrow luminous gutters.
 a.poly([(58,64),(82,64),(120,120),(0,120)],CARD)
 a.poly([(61,68),(72,68),(92,120),(25,120)],BG)
 for x in [0,20,45,102,119]:a.line(68,67,x,119,GRID)
 for y in [83,95,110]:a.line(0,y,120,y,GRID)
 a.poly([(0,75),(57,62),(54,84),(0,111)],PLUM)
 a.line(0,111,56,83,PINK);a.line(80,69,119,110,CYAN)
 # Left arcade façade, roof lip, panels, awning and entrance.
 a.rect(0,20,55,66,CARD);a.poly([(55,20),(67,31),(67,67),(55,86)],PLUM)
 a.rect(0,20,55,3,PURPLE);a.rect(0,24,55,2,PLUM)
 for x in [2,17,32,47]:
  a.rect(x,27,11,9,BG);a.box(x,27,11,9,TEAL);a.line(x+5,27,x+5,35,GRID)
 a.rect(1,36,59,16,PLUM);a.box(1,36,59,16,PINK);a.box(3,38,55,12,PURPLE);a.rect(4,39,53,10,BG)
 a.rect(0,52,57,7,CARD)
 for x in range(0,57,6):a.poly([(x,52),(x+3,52),(x+5,58),(x+2,58)],PINK)
 a.line(0,59,58,59,PURPLE)
 a.rect(2,61,52,25,BG);a.box(2,61,52,25,TEAL)
 # Three recognizable arcade cabinets, joysticks, illuminated control decks.
 for x,c in [(5,CYAN),(22,PINK),(39,LIME)]:
  a.rect(x,63,12,19,PLUM);a.box(x,63,12,19,c);a.rect(x+2,65,8,7,BLUE);a.rect(x+3,66,6,5,BG)
  a.rect(x+4,67,2,2,c);a.rect(x+7,69,2,1,YELLOW);a.rect(x+1,74,10,3,c);a.dot(x+4,74,BG);a.line(x+3,73,x+3,74,WHITE);a.dot(x+8,75,BG)
  a.rect(x+3,79,5,1,GRID)
 a.line(0,87,54,87,CYAN);a.line(0,89,51,89,TEAL)
 # Huge right wall, rooftop pipes, vertical game sign and balcony rail.
 a.poly([(86,13),(119,1),(119,105),(88,80)],PLUM)
 a.line(88,16,88,80,PURPLE);a.line(91,17,119,7,GRID)
 for y in range(22,75,13):
  a.rect(99,y,15,9,BG);a.box(99,y,15,9,BLUE);a.line(106,y,106,y+8,GRID)
 a.rect(80,23,16,42,TEAL);a.box(80,23,16,42,CYAN);a.rect(82,25,12,38,BG)
 # Hand-drawn pixel katakana: ゲーム (game), three vertical glyphs.
 glyphs=['01000010/01111101/01010000/10010000/00010000/00100000/01000000/00000000','00000000/00000000/11111110/00000000/00000000/00000000/00000000/00000000','00010000/00100000/00100000/01000100/01000010/10000010/11111110/00000000']
 for n,glyph in enumerate(glyphs):
  for yy,row in enumerate(glyph.split('/')):
   for xx,v in enumerate(row):
    if v=='1':a.dot(84+xx,28+n*11+yy,CYAN)
 a.rect(96,75,24,4,PURPLE);a.rect(98,80,22,2,PINK)
 # Vending machine, drinks, payment slot, dispenser and glow.
 a.rect(99,84,17,25,TEAL);a.box(99,84,17,25,CYAN);a.rect(101,86,11,13,BG)
 for y in [88,93]:
  for x,c in [(102,PINK),(106,YELLOW),(110,CYAN)]:a.rect(x,y,2,3,c);a.dot(x,y-1,WHITE)
 a.rect(113,89,2,5,PINK);a.rect(102,102,10,4,BG);a.line(100,110,116,110,CYAN)
 # Lanterns, hanging placards and a narrow distant shop.
 a.rect(69,52,7,9,ORANGE);a.line(68,53,76,53,YELLOW);a.line(68,59,76,59,PINK);a.line(72,46,72,51,GRID)
 a.rect(59,59,8,18,CARD);a.box(59,59,8,18,PINK);a.text('24',59,63,YELLOW);a.text('H',61,70,YELLOW)
 a.rect(0,2,23,14,PLUM);a.box(0,2,23,14,PINK);a.text('PLAY',3,6,YELLOW)
 a.rect(93,1,25,10,TEAL);a.text('OPEN',97,4,CYAN)
 # Utility poles, insulators and sagging cables crossing the skyline.
 a.rect(77,2,2,78,BG);a.line(73,8,82,8,GRID);a.rect(74,6,2,3,GREY);a.rect(80,6,2,3,GREY)
 for y in [10,15,18]:
  last=(0,y)
  for x in range(1,120):
   point=(x,y+round(9*math.sin(x/120*math.pi)));a.line(*last,*point,BG);last=point
 a.line(56,28,77,36,BG);a.line(77,36,119,21,BG)
 # Drain and pavement highlights.
 a.poly([(47,106),(67,106),(70,112),(44,112)],GRID)
 for x in range(48,67,3):a.line(x,107,x-1,111,BG)
 return a

def scene(now,name='BANDICUSS'):
 a=background();t=now//100
 # Same integer animation rules as NeonArcade.h; all coordinates logical 120px.
 for i in range(28):
  x=(i*29+11)%120;y=88+(i*11)%31;dx=((t//3+i)%5)-2;c=[PINK,TEAL,PURPLE,CYAN][i%4]
  a.rect(x+dx,y,3+i%8,1,c)
 for i in range(30):
  x=(i*37+13+(t//2)%120)%120;y=(i*23+t*(2+i%2))%120
  if 35<=y<=53 and x<61:continue
  a.line(x,y,x-1,y+3,TEAL if i%3 else BLUE)
 for i in range(5):
  x=16+i*21;y=95+i%3*8;phase=(t+i*5)%18
  if phase<5:a.line(x-phase//2,y,x+phase//2,y,TEAL)
 for i in range(10):
  a.rect(4+i*5,50,2,1,PINK if (t//8+i)%10<3 else PURPLE)
 label=name.upper();scale=1;w=len(label)*4-1
 # Full-resolution custom name is drawn separately in firmware/preview.
 return a

def main():
 a=background();pixels=[c for row in a.p for c in row];packed=bytes((pixels[i]<<4)|pixels[i+1] for i in range(0,len(pixels),2))
 rows=[','.join(f'0x{b:02x}' for b in packed[i:i+24]) for i in range(0,len(packed),24)]
 font=[]
 for code in range(32,127):
  bits=FONT.get(chr(code).upper(),FONT['?']);font.append([int(bits[i:i+3],2) for i in range(0,15,3)])
 header='#pragma once\n#include <Arduino.h>\n// Generated by art/build_neon.py. 120x120, two pixels per byte.\n'
 header+='const uint8_t NEON_BACKGROUND[7200] PROGMEM={\n'+',\n'.join(rows)+'\n};\n'
 header+='const uint8_t NEON_FONT[95][5] PROGMEM={\n'+',\n'.join('{'+','.join(map(str,f))+'}' for f in font)+'\n};\n'
 (ROOT/'DeskBuddy/NeonArt.h').write_text(header,encoding='utf8')
 frames=[]
 for i in range(40):
  c=scene(i*100);big=Canvas(240,240)
  for y in range(120):
   for x in range(120):big.rect(x*2,y*2,2,2,c.p[y][x])
  name='BANDICUSS';big.text(name,60-(len(name)*8-2)//2,81,YELLOW,2)
  frames.append(big.png())
 (ROOT/'previews/v070-neon.png').write_bytes(frames[0])
 urls=['data:image/png;base64,'+base64.b64encode(b).decode() for b in frames]
 html='<!doctype html><meta charset="utf-8"><title>Neon Arcade · 240 × 240 preview</title><style>body{margin:0;background:#090e1e;color:#a0aecd;font:14px monospace;display:grid;place-content:center;min-height:100vh;gap:20px}img{width:480px;image-rendering:pixelated;max-width:90vw}p{max-width:480px}button{padding:12px;background:#21e6ff;border:0}</style><img id="scene" alt="Bandicuss neon arcade scene"><p>Neon Arcade · 240 × 240 pixels · 10 FPS<br>Artwork only. Tap the device to change pages.</p><button id="pause">Pause animation</button><script>const frames='+json.dumps(urls)+';let n=0,paused=false;const image=document.getElementById("scene");image.src=frames[0];setInterval(()=>{if(!paused)image.src=frames[++n%frames.length]},100);document.getElementById("pause").onclick=e=>{paused=!paused;e.target.textContent=paused?"Play animation":"Pause animation"}</script>'
 (ROOT/'previews/v070-neon.html').write_text(html,encoding='utf8')
 print('Generated 7200-byte scene, 475-byte font, PNG and animated preview.')
if __name__=='__main__':main()
