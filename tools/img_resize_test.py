import os

from PIL import ImageDraw, Image

import numpy as np

import struct

from ctypes import *

_img_lib = cdll.LoadLibrary('./img_resize.so')
imresize = _img_lib.img_resize


im = Image.open('/usr/share/wallpapers/FallenLeaf/contents/images/2560x1600.jpg')
#im=Image.new('RGB', (500, 400), (255, 255, 255))
#draw = ImageDraw.Draw(im)
#draw.ellipse((20, 50, 300, 200), fill=(255, 255, 0))

aim = np.array(im)

h = aim[:,:,0].shape[0]
w = aim[:,:,0].shape[1]


#fbm = b'\x00\x00\x00' * (w * h)

#for i in range(0, h):
#  for j in range(0, w):
#    ps = (i * w + j) * 3
#    fbm[ps]   = struct.pack('B', aim[i, j, 0])
#    fbm[ps+1] = struct.pack('B', aim[i, j, 1])
#    fbm[ps+2] = struct.pack('B', aim[i, j, 2])

new_w = 800
new_h = 600

print('new_w=%d new_h=%d' % (new_w, new_h))

rsim = b'\x00\x00\x00' * (new_w * new_h)

#rsim = fbm

print(imresize(24, 24, w, h, new_w, new_h, im.tobytes(), rsim))

#aoutim = np.uint8(np.zeros((new_h, new_w, 3)))
#for i in range(0, new_h):
#  for j in range(0, new_w):
#    ps = (i * new_w + j) * 3
#    aoutim[i, j, 0] = rsim[ps]
#    aoutim[i, j, 1] = rsim[ps+1]
#    aoutim[i, j, 2] = rsim[ps+2]

print('-------------------\n\n')

#outim = Image.fromarray(aoutim)

print('Original size = %d' % len(rsim))

bf = c_void_p(_img_lib.z_prepare_buffer(0))
print('Compress returns %d ' % 
  _img_lib.z_deflate(len(rsim), rsim, bf))

zdl = _img_lib.z_get_buffer_data_sz(bf)

print('Compressed size = %d' % zdl)

_img_lib.z_get_buffer_data.restype = c_void_p

zd = _img_lib.z_get_buffer_data(bf)

bf2 = c_void_p(_img_lib.z_prepare_buffer(0))

_img_lib.z_inflate.argtypes = [c_int, c_void_p, c_void_p]
_img_lib.z_inflate.restype = c_int

print('Decompress returns %d ' % 
  _img_lib.z_inflate(zdl, zd, bf2))
  
unzdl = _img_lib.z_get_buffer_data_sz(bf2)
print('Decompressed size = %d' % unzdl)

unzd = _img_lib.z_get_buffer_data(bf2)

infl = bytearray((c_char * unzdl).from_address(unzd))

print('infl size = %d' % len(infl))
#outim = Image.fromarray(infl)

outim = Image.frombytes("RGB", (new_w, new_h), infl)

#im.show()
#os.read(0, 5)
outim.show()
