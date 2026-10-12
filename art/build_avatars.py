"""Code-native pixel artwork. Regenerates device arrays and matching phone SVGs.
No external images, fonts, packages or network calls are required.
"""
from pathlib import Path
import math,json,re,struct,zlib
ROOT=Path(__file__).resolve().parents[1]
BG,CARD,WHITE,GREY,CYAN,BLACK,LIME,PINK,PURPLE,YELLOW,ORANGE,BLUE,TEAL,PLUM,OLIVE,GRID=range(16)
COLORS=['#090e1e','#141d35','#f7faff','#a0aecd','#21e6ff','#090e1e','#bcff40','#ff6b8a','#b68cff','#ffe36d','#ff9d4d','#5678ff','#154154','#352547','#324426','#28365a']
NAMES=['Weather Cloud','Paperclip','Hacker Dolphin','Money Sun','Mr. Meeseeks','Pokemon Parade','Joe Camel']
CAPTIONS=['weather-aware company','looks like you need a friend','access granted_','stay sunny. stay paid.','existence is a bit much','a walk with the crew','cool as ever']
class Pixel:
 def __init__(self,w=48,h=40):self.w=w;self.h=h;self.p=[[BG]*w for _ in range(h)]
 def dot(self,x,y,c):
  if 0<=x<self.w and 0<=y<self.h:self.p[y][x]=c
 def rect(self,x,y,w,h,c):
  for yy in range(y,y+h):
   for xx in range(x,x+w):self.dot(xx,yy,c)
 def line(self,x0,y0,x1,y1,c,t=1):
  steps=max(abs(x1-x0),abs(y1-y0),1)
  for i in range(steps+1):self.rect(round(x0+(x1-x0)*i/steps)-t//2,round(y0+(y1-y0)*i/steps)-t//2,t,t,c)
 def ellipse(self,x,y,rx,ry,c):
  for yy in range(y-ry,y+ry+1):
   for xx in range(x-rx,x+rx+1):
    if (xx-x)**2/max(rx*rx,1)+(yy-y)**2/max(ry*ry,1)<=1.05:self.dot(xx,yy,c)
 def poly(self,points,c):
  for y in range(min(p[1] for p in points),max(p[1] for p in points)+1):
   cuts=[]
   for (x0,y0),(x1,y1) in zip(points,points[1:]+points[:1]):
    if (y0<=y<y1) or (y1<=y<y0):cuts.append(x0+(y-y0)*(x1-x0)/(y1-y0))
   cuts.sort()
   for i in range(0,len(cuts)-1,2):self.rect(math.ceil(cuts[i]),y,math.floor(cuts[i+1])-math.ceil(cuts[i])+1,1,c)
  for a,b in zip(points,points[1:]+points[:1]):self.line(*a,*b,c)
 def glyph(self,text,x,y,c):
  for yy,row in enumerate(text.split('/')):
   for xx,v in enumerate(row):
    if v=='1':self.dot(x+xx,y+yy,c)
 def svg(self):
  paths=[]
  for y,row in enumerate(self.p):
   x=0
   while x<self.w:
    c=row[x];end=x+1
    while end<self.w and row[end]==c:end+=1
    if c:paths.append(f'<path fill="{COLORS[c]}" d="M{x} {y}h{end-x}v1H{x}z"/>')
    x=end
  return f'<svg viewBox="0 0 {self.w} {self.h}" aria-hidden="true" shape-rendering="crispEdges">'+''.join(paths)+'</svg>'
def cloud(state,frame):
 a=Pixel();bob=frame
 if state==1:
  a.ellipse(33,11,8,8,YELLOW)
  a.rect(32,0,2,3,ORANGE);a.rect(43,10,3,2,ORANGE);a.rect(41,2,2,2,ORANGE)
 if state==5:
  a.ellipse(41,6,6,6,PURPLE);a.ellipse(44,4,5,5,BG)
  for x,y in [(8,5),(18,1),(43,19)]:a.rect(x,y,2,2,YELLOW)
 shade=BLUE if state==3 else GREY
 a.ellipse(13,22+bob,9,8,shade);a.ellipse(24,17+bob,12,11,shade);a.ellipse(35,23+bob,9,7,shade);a.rect(13,22+bob,24,10,shade)
 a.ellipse(13,20+bob,8,7,WHITE);a.ellipse(24,15+bob,10,10,WHITE);a.ellipse(35,21+bob,8,7,WHITE);a.rect(13,21+bob,24,8,WHITE)
 a.rect(17,21+bob,3,4,BLACK);a.rect(29,21+bob,3,4,BLACK);a.rect(14,26+bob,4,2,PINK);a.rect(32,26+bob,4,2,PINK)
 a.line(23,26+bob,24,27+bob,BLACK);a.line(24,27+bob,26,26+bob,BLACK)
 if state==2:
  for x in [12,24,36]:a.line(x+1,33+frame*2,x,36+frame*2,CYAN,2)
 if state==3:
  a.poly([(25,30),(20,35),(25,35),(22,39),(31,33),(26,33),(29,30)],YELLOW)
 if state==4:
  for x in [12,25,37]:a.line(x-2,35+frame,x+2,35+frame,CYAN);a.line(x,33+frame,x,37+frame,CYAN)
 if state==6:
  a.line(8+frame*2,34,37+frame*2,34,GREY);a.line(13-frame*2,38,40-frame*2,38,GREY)
 return a
def clip(frame):
 a=Pixel();points=[(16,29),(16,10),(19,6),(28,6),(32,10),(32,29),(28,34),(21,34),(20,32),(20,13),(22,11),(26,11),(28,13),(28,27),(26,29),(24,27),(24,17)]
 for x,y in zip(points,points[1:]):a.line(x[0]+1,x[1]+1,y[0]+1,y[1]+1,BLUE,3)
 for x,y in zip(points,points[1:]):a.line(*x,*y,GREY,3);a.line(x[0]-1,x[1],y[0]-1,y[1],WHITE)
 a.line(13,5,19,3,BLACK,2);a.line(24,3,29,5,BLACK,2)
 for x in [16,26]:
  a.ellipse(x,10,5,6,BLUE);a.ellipse(x,9,4,5,WHITE);a.ellipse(x+frame,10,1,2,BLACK)
 a.rect(10,36,27,1,GRID)
 return a
def dolphin(frame):
 a=Pixel();a.poly([(5,18),(13,23),(5,29),(8,23)],CYAN);a.poly([(17,15),(22,5),(27,16)],BLUE)
 a.ellipse(24,20,15,8,BLUE);a.ellipse(26,17,15,8,CYAN);a.ellipse(40,16,6,3,CYAN)
 a.ellipse(29,21,10,4,WHITE);a.line(38,19,43,18,TEAL)
 a.line(28,8,32,7,BLACK,3);a.line(32,7,37,10,BLACK,3);a.rect(28,10,4,10,BLACK);a.rect(29,12,2,5,LIME)
 a.rect(35,12,3,3,BLACK);a.dot(35,12,WHITE);a.line(31,20,34,23,BLACK,2);a.rect(34,22,4,2,LIME)
 a.rect(15,26,27,12,LIME);a.rect(16,27,25,9,CARD);a.line(18,29,20,31,LIME);a.line(20,31,18,33,LIME);a.rect(23,33,4 if frame else 2,1,LIME)
 a.rect(13,38,31,2,GREY);a.poly([(17,20),(13,28),(18,31),(20,25)],CYAN)
 return a
def sun(frame):
 a=Pixel()
 for dx,dy in [(0,-17),(12,-12),(17,0),(12,12),(0,17),(-12,12),(-17,0),(-12,-12)]:
  a.line(24+dx,20+dy,24+round(dx*1.15),20+round(dy*1.15),ORANGE,2 if frame else 3)
 a.ellipse(24,21,14,14,ORANGE);a.ellipse(24,19,14,13,YELLOW)
 a.rect(9,13,14,10,BLACK);a.rect(26,13,14,10,BLACK);a.rect(22,15,5,2,BLACK)
 dollar='00100/01111/10100/01110/00101/11110/00100'
 a.glyph(dollar,14,14,LIME);a.glyph(dollar,31,14,LIME)
 a.line(19,27,22,30,BLACK,2);a.line(22,30,27,30,BLACK,2);a.line(27,30,30,27,BLACK,2);a.rect(22,27,6,2,WHITE)
 return a
def meeseeks(frame):
 a=Pixel();a.line(20,29,19-frame,37,BLUE,3);a.line(27,29,29+frame,37,BLUE,3);a.rect(16-frame,37,6,2,CYAN);a.rect(28+frame,37,6,2,CYAN)
 a.line(18,23,13,31,CYAN,3);a.line(29,23,34,24,CYAN,3);a.line(34,24,36+frame,16-frame,CYAN,2);a.rect(35+frame,13-frame,3,5,CYAN)
 a.ellipse(24,25,6,7,CYAN);a.rect(22,18,4,6,CYAN);a.ellipse(24,11,10,9,BLUE);a.ellipse(23,10,10,9,CYAN)
 a.line(22,2,24,0,ORANGE);a.line(24,2,27,1,ORANGE);a.line(25,3,29,3,ORANGE)
 a.ellipse(19,9,2,3,WHITE);a.ellipse(28,9,2,3,WHITE);a.rect(19,9,1,2,BLACK);a.rect(27,9,1,2,BLACK)
 a.ellipse(24,15,4,3,BLACK);a.rect(22,13,5,1,WHITE);a.rect(23,16,3,2,PINK)
 return a
def camel(frame):
 a=Pixel();a.poly([(13,34),(18,29),(29,29),(37,34),(39,39),(9,39)],BLUE);a.poly([(18,29),(24,34),(29,29),(30,39),(17,39)],PLUM)
 a.rect(20,22,8,11,ORANGE);a.poly([(17,29),(23,34),(19,35)],WHITE);a.poly([(29,29),(23,34),(29,35)],WHITE)
 a.ellipse(19,8,4,7,ORANGE);a.ellipse(30,8,4,7,ORANGE);a.line(18,3,19,8,YELLOW,2);a.line(31,3,30,8,YELLOW,2)
 a.ellipse(24,14,8,11,ORANGE);a.ellipse(24,12,6,9,YELLOW);a.ellipse(30,22,11,6,ORANGE);a.ellipse(31,20,10,5,YELLOW)
 a.rect(17,10,8,6,BLACK);a.rect(27,10,8,6,BLACK);a.rect(24,11,4,2,BLACK);a.line(18,11,21,11,CYAN);a.line(28,11,31,11,CYAN)
 a.rect(37,18,2,2,BLACK);a.line(28,25,36,24,BLACK);a.rect(30,24,4,1,WHITE)
 a.rect(14,34,2,3,PURPLE if frame else CYAN)
 return a
def parade(frame):
 a=Pixel(72,40)
 for x,h in [(1,15),(16,10),(33,17),(53,12),(64,20)]:
  a.rect(x,8,8,h,CARD)
  for y in range(10,8+h-2,5):a.rect(x+2,y,2,2,PLUM);a.rect(x+5,y,2,2,TEAL)
 a.rect(0,32,72,2,GREY);a.rect(0,34,72,6,CARD)
 for x in range(-6+frame*5,72,14):a.rect(x,37,7,1,YELLOW)
 # Pikachu: pointed ears, red cheeks, lightning tail.
 x,y=9,22-frame;a.poly([(x-5,y),(x-7,y-11),(x-4,y-10),(x-2,y-3)],YELLOW);a.poly([(x+2,y-3),(x+5,y-11),(x+7,y-10),(x+5,y)],YELLOW);a.rect(x-7,y-11,3,3,BLACK);a.rect(x+5,y-11,2,3,BLACK)
 a.poly([(x+5,y+5),(x+10,y+3),(x+8,y),(x+12,y-2),(x+11,y+5),(x+7,y+8)],YELLOW)
 a.ellipse(x,y,6,5,YELLOW);a.ellipse(x,y+6,4,5,YELLOW);a.dot(x-2,y-1,BLACK);a.dot(x+3,y-1,BLACK);a.rect(x-5,y+1,2,2,PINK);a.rect(x+4,y+1,2,2,PINK);a.dot(x+1,y+2,BLACK)
 a.line(x-2,y+10,x-4+frame*3,32,YELLOW,2);a.line(x+2,y+10,x+4-frame*3,32,YELLOW,2)
 # Bulbasaur: broad face and leafy bulb, four short walking feet.
 x,y=28,25+frame;a.ellipse(x+3,y-4,5,5,OLIVE);a.poly([(x-2,y-5),(x+2,y-12),(x+6,y-7),(x+9,y-9),(x+7,y-2)],LIME);a.ellipse(x+1,y+2,7,4,LIME);a.ellipse(x-4,y,5,4,LIME);a.rect(x-8,y-5,3,3,LIME);a.rect(x-2,y-5,3,3,LIME)
 a.rect(x-7,y-1,2,2,PINK);a.rect(x-2,y-1,2,2,PINK);a.dot(x-7,y-1,WHITE);a.dot(x-2,y-1,WHITE);a.dot(x-4,y+2,BLACK);a.rect(x+2,y+1,2,2,OLIVE)
 for dx in [-5,0,5]:a.rect(x+dx+frame,y+4,2,3-frame,LIME)
 # Charmander: orange lizard, cream belly and a bright tail flame.
 x,y=46,22-frame;a.line(x+3,y+7,x+9,y+5,ORANGE,3);a.poly([(x+9,y+4),(x+8,y),(x+11,y-4),(x+13,y+1),(x+11,y+5)],PINK);a.rect(x+10,y,2,4,YELLOW)
 a.ellipse(x,y+5,4,7,ORANGE);a.ellipse(x,y+6,2,4,YELLOW);a.ellipse(x,y-2,5,5,ORANGE);a.rect(x-2,y-4,2,2,WHITE);a.dot(x-1,y-4,BLACK);a.line(x+1,y,x+4,y,BLACK)
 a.line(x-2,y+10,x-4+frame*3,32,ORANGE,2);a.line(x+2,y+10,x+4-frame*3,32,ORANGE,2)
 # Squirtle: blue head/limbs, brown shell and cream belly.
 x,y=63,23+frame;a.ellipse(x+2,y+3,5,6,ORANGE);a.ellipse(x,y+4,4,5,CYAN);a.ellipse(x-1,y+4,2,4,YELLOW);a.ellipse(x-1,y-4,5,5,CYAN);a.rect(x-3,y-6,2,3,WHITE);a.dot(x-2,y-5,BLACK);a.line(x-1,y-1,x+2,y-1,BLACK)
 a.line(x-4,y+2,x-6,y+5,CYAN,2);a.line(x-2,y+8,x-4+frame*2,32,CYAN,2);a.line(x+2,y+8,x+4-frame*2,32,CYAN,2);a.ellipse(x+6,y+6,2,2,CYAN)
 return a
def png(path,pixels,scale=1):
 h=len(pixels);w=len(pixels[0]);rgb=[bytes.fromhex(c[1:]) for c in COLORS];data=b''
 for row in pixels:
  encoded=b'\0'+b''.join(rgb[c]*scale for c in row)
  data+=encoded*scale
 def chunk(k,v):return struct.pack('>I',len(v))+k+v+struct.pack('>I',zlib.crc32(k+v))
 path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w*scale,h*scale,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(data))+chunk(b'IEND',b''))
def build():
 frames=[[fn(i) for i in range(2)] for fn in [lambda i:cloud(0,i),clip,dolphin,sun,meeseeks,parade,camel]]
 clouds=[[cloud(s,i) for i in range(2)] for s in range(7)]
 code=['#pragma once','#include <Arduino.h>','// Generated by art/build_avatars.py. Palette indexes match Visuals::Color.','struct AvatarFrame { uint8_t width,height;const uint8_t* pixels; };', 'constexpr uint8_t ART_AVATAR_COUNT=7,CLOUD_STATE_COUNT=7;','const char* const AVATAR_NAMES[7]='+json.dumps(NAMES).replace('[','{').replace(']','}')+';','const char* const AVATAR_CAPTIONS[7]='+json.dumps(CAPTIONS).replace('[','{').replace(']','}')+';']
 def emit(name,f):
  packed=[]
  for row in f.p:
   assert len(row)%2==0 and all(0<=p<16 for p in row)
   packed.extend((row[i]<<4)|row[i+1] for i in range(0,f.w,2))
  code.append(f'const uint8_t {name}[] PROGMEM={{'+','.join(str(v) for v in packed)+'};')
 for ai,pair in enumerate(frames):
  for fi,f in enumerate(pair):emit(f'art_{ai}_{fi}',f)
 for ai,pair in enumerate(clouds):
  for fi,f in enumerate(pair):emit(f'cloud_{ai}_{fi}',f)
 for name,allframes,prefix in [('AVATAR_FRAMES',frames,'art'),('CLOUD_FRAMES',clouds,'cloud')]:
  rows=['{'+','.join('{'+f'{f.w},{f.h},{prefix}_{ai}_{fi}'+'}' for fi,f in enumerate(pair))+'}' for ai,pair in enumerate(allframes)]
  code.append(f'const AvatarFrame {name}[7][2]={{'+','.join(rows)+'};')
 (ROOT/'DeskBuddy/AvatarArt.h').write_text('\n'.join(code)+'\n',encoding='utf8')
 p=ROOT/'DeskBuddy/DashboardPage.h';s=p.read_text(encoding='utf8')
 s=re.sub(r'const avatarPreviews=.*?;\nfunction renderAvatar',lambda m:'const avatarPreviews='+json.dumps([pair[0].svg() for pair in frames])+';\nfunction renderAvatar',s,flags=re.S)
 options=''.join(f'<option value="{i}">{name}</option>' for i,name in enumerate(NAMES))
 s=re.sub(r'<select id="avatar">.*?</select>',lambda m:'<select id="avatar">'+options+'</select>',s)
 s=s.replace('One Companion page. Pick the face that feels like yours.','One Companion page. Seven characters. The cloud follows your weather; the parade keeps walking.')
 s=s.replace('.avatar-preview svg{width:96px;height:96px;flex-shrink:0}', '.avatar-preview svg{width:144px;height:120px;max-width:100%;flex-shrink:0}.avatar-preview{flex-wrap:wrap}')
 p.write_text(s,encoding='utf8')
 previews=ROOT/'previews';previews.mkdir(exist_ok=True)
 for i,pair in enumerate(frames):png(previews/f'v050-art-{i}.png',pair[0].p,4)
 for i,pair in enumerate(clouds):png(previews/f'v050-cloud-state-{i}.png',pair[0].p,4)
 for pair in frames+clouds:
  assert pair[0].p!=pair[1].p,'Each character/state must have two distinct animation frames'
 sheet=Pixel(72*4,46*2)
 for i,pair in enumerate(frames):
  f=pair[0];ox=(i%4)*72+(72-f.w)//2;oy=(i//4)*46
  for y,row in enumerate(f.p):
   for x,c in enumerate(row):sheet.dot(ox+x,oy+y,c)
 png(previews/'v050-art-lineup.png',sheet.p,4)
 print('Generated seven two-frame avatars, seven weather states, and matching SVG previews.')
if __name__=='__main__':build()
