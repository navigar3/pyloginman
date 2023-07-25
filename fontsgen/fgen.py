from fontTools.ttLib import TTFont
from fontTools.unicode import Unicode


from PIL import ImageFont, ImageDraw, Image

import numpy as np

import struct

fname = 'font.ttf'
fsize = 64

# Get Best Character map dictionary
font = TTFont(fname)
cmap = font.getBestCmap()
font.close()

# Re-open font via PIL
font = ImageFont.truetype(fname, fsize)

(ascent, descent) = font.getmetrics()

cmapval = {}

maxwidth = 0
maxheight = 0
kmaxw = 0
kmaxh = 0

for k in cmap:
  c = chr(k)
  
  print('Rendering [' + str(k) + '] ' + c + ' ...')

  (offx, offy, w, h) = font.getbbox(c)
  
  if type(font.getmask(c).getbbox()) is type(None):
    (toffx, toffy, tw, th) = (0, 0, 0, 0)
  else:
    (toffx, toffy, tw, th) = font.getmask(c).getbbox()
  
  cmapval[k] = {'gname': cmap[k], 
                'offx': offx, 'offy': offy, 'w': w, 'h': h,
                'toffx': toffx, 'toffy': toffy, 'tw': tw, 'th': th}
  
  if offx+toffx+tw > maxwidth:
    maxwidth = offx+toffx+tw
    kmaxw = k
  
  if offy+toffy+th > maxheight:
    maxheight = offy+toffy+th
    kmaxh = k

print ('Number of glyphs: %d' % len(cmapval))

print ('Char with max width is [%d] %c\n width=%d' % (kmaxw, chr(kmaxw), maxwidth))
print ('Char with max height is [%d] %c\n height=%d' % (kmaxh, chr(kmaxh), maxheight))

#print(cmapval)

#c = chr(9835)

#print ("acent=%d\ndescent=%d\noffset x=%d\noffset y=%d\nw=%d\nh=%d\n"
#       "tw=%d\nth=%d\ntoffx=%d\ntoffy=%d" % 
#  (ascent, descent, offx, offy, w, h, tw, th, toffx, toffy))


def draw_glyph(c):
  
  (offx, offy, w, h) = font.getbbox(c)
  
  r  = font.getmask(c).getbbox()
  if type(r) is type(None):
    return None
  
  (toffx, toffy, tw, th) = font.getmask(c).getbbox()

  im = Image.new("RGB", (tw, th), (255, 255, 255))
  draw = ImageDraw.Draw(im)
  
  draw.text((-offx-toffx, -offy-toffy), c, (0, 0, 0), font=font)
  
  #im.show()
  aim = np.array(im)
  #fim = ''
  #fim += 'P2\n%d %d\n15\n' % (aim[:,:,0].shape[1], aim[:,:,0].shape[0])
  #for row in aim[:,:,0]:
  #  for j in row:
  #    fim += '%d ' % int(j / 16)
  #  fim += '\n'
  
  #with open('img.pgm', 'w') as f:
  #  f.write(fim)
  
  # Pack image with 4 bits depht
  
  fbm = struct.pack('<I', offx+toffx) + \
    struct.pack('<I', offy+toffy) + \
    struct.pack('<I', aim[:,:,0].shape[1]) + \
    struct.pack('<I', aim[:,:,0].shape[0])
  ms = True
  doublepxsval = 0
  for row in aim[:,:,0]:
    for j in row:
      if ms is True:
        doublepxsval = j & 0xf0
        ms = False
      else:
        doublepxsval |= ((j & 0xf0) >> 4)
        ms = True
        fbm += struct.pack('B', doublepxsval)
  
  #with open('img.raw', 'wb') as f:
  #  f.write(fbm)
  
  return fbm


# Open dictionary
import sys

sys.path.append('../tools')

from htdicts import htdicts

d = htdicts(libpath='../tools/')

d.newtable(b'.metrics', hash_modulus=31)
d.dl['.metrics'].push(b'boxwidth', struct.pack('<I', maxwidth))
d.dl['.metrics'].push(b'boxheight', struct.pack('<I', maxheight))
d.dl['.metrics'].push(b'ascent', struct.pack('<I', ascent))
d.dl['.metrics'].push(b'descent', struct.pack('<I', descent))

d.newtable(b'.reverse_cmap', hash_modulus=179)
for k in cmap:
  c = chr(k)
  #print (cmap[k].encode(), '  ', c.encode())
  d.dl['.reverse_cmap'].push(cmap[k].encode(), c.encode())


d.newtable(b'.glyphs', hash_modulus=311)
for k in cmap:
  c = chr(k)
  
  #if k == 53:
  #  break
  
  rastg = draw_glyph(c)
  
  if rastg is None:
    continue
  
  #print('push key ', c, ' as ', c.encode())
  
  d.dl['.glyphs'].push(c.encode(), rastg)

print("res = %d " % d.savedicts(b'fonts2.baf')) 
