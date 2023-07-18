from ctypes import *

class lres(Structure):
  _fields_ = [("key", c_char_p),
              ("content", c_char_p),
              ("keysize", c_uint16),
              ("contentsize", c_uint32)]

class htdict:
  def __init__(self, htlib, p):
    self._hl = htlib
    self._dic = p
  
  def push(self, keysize, key, valuesize, value):
    self._hl.ht_table_push(self._dic, keysize, key, valuesize, value)
  
  def search(self, keysize, key):
    res = lres()
    
    nres = \
      self._hl.ht_table_lookup(self._dic, keysize, key, pointer(res))
    
    if not nres:
      return {'sz': res.contentsize, 'val': res.content}
    else:
      return None
      

class htdicts:
  
  def __init__(self, filename=None):
    #Load library
    self._hl = cdll.LoadLibrary('./ht_dict.so')
    
    if filename is None:
      #Create empty dictionaries container
      self._dics = c_void_p(self._hl.new_ht_dicts())
    
      #Create empty dictionaries dictionary
      self.dl = {}
  
  def newtable(self, table_name, hash_algo=b"mod", hash_modulus=256, 
               hash_dl_helper=b"", has_only_key=False,
               has_fixed_key_size=False, has_fixed_size=False,
               key_size=0, fixed_size_len=0):
    
    _dic = \
      self._hl.new_ht_table(self._dics,
                            table_name, hash_algo, hash_dl_helper,
                            c_uint32(hash_modulus),
                            c_uint32(has_only_key),
                            c_uint32(has_fixed_key_size),
                            c_uint32(key_size),
                            c_uint32(has_fixed_size),
                            c_uint32(fixed_size_len))
    
    if _dic != 0:
      self.dl[table_name.decode()] = htdict(self._hl, c_void_p(_dic))
      
  def savedicts(self, file_name):
    self._hl.ht_save_dicts(self._dics, file_name)

d = htdicts()
d.newtable(b"prova", hash_modulus=42)

d.dl['prova'].push(3, b"rc", 5, b"fila")

sr = d.dl['prova'].search(3, b"rc")

if sr is not None:
  print(sr)
else:
  print('No search results!')

#d.savedicts(b"from_py.baf")
