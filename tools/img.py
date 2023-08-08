import os, sys

from PIL import ImageDraw, Image

import struct

from ctypes import *


_zlib = cdll.LoadLibrary('./zhelper.so')

_zlib.z_get_buffer_data.restype = c_void_p

_zlib.z_inflate.argtypes = [c_int, c_void_p, c_void_p]
_zlib.z_inflate.restype = c_int

im = Image.open(sys.argv[1])

mode = im.mode
w = im.width
h = im.height

if mode == 'RGB':
  bmode = 24
elif mode == 'RGBA':
  bmode = 32
else:
  sys.exit(1)

bim = struct.pack('4B', ord(b'i'), ord(b'0'), ord(b'i'), ord(b'0'))
bim += struct.pack('<I', bmode)
bim += struct.pack('<I', w)
bim += struct.pack('<I', h)
bim += im.tobytes()


bf = c_void_p(_zlib.z_prepare_buffer(0))
_zlib.z_deflate(len(bim), bim, bf)

zdl = _zlib.z_get_buffer_data_sz(bf)
zd = _zlib.z_get_buffer_data(bf)

zdb = bytearray((c_char * zdl).from_address(zd))

print (zdl)

zdb = bytearray(b'z0z0') + struct.pack('<I', len(bim)) + zdb

with open(sys.argv[2], 'wb') as f:
  f.write(zdb)
