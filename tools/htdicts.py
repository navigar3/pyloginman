from ctypes import *

class lres(Structure):
  _fields_ = [("key", c_void_p),
              ("content", c_void_p),
              ("keysize", c_uint16),
              ("contentsize", c_uint32)]

class htdict:
  def __init__(self, htlib, p):
    self._hl = htlib
    self._dic = p
  
  def push(self, keysize, key, valuesize, value):
    self._hl.ht_table_push(self._dic, keysize, key, valuesize, value)
  
  def search(self, key):
    keysize = len(key)
    
    res = lres()
    
    nres = \
      self._hl.ht_table_lookup(self._dic, keysize, key, pointer(res))
    
    if not nres:
      return (c_char * res.contentsize).from_address(res.content)
    else:
      return None
    
  def print_data(self):
    self._hl.ht_print_data(self._dic)
      

class htdicts:
  
  def __init__(self, filename=None, libpath=None):
    if libpath is None:
      libfullpath = 'ht_dict.so'
    else:
      libfullpath = libpath + '/' + 'ht_dict.so'
    #Load library
    self._hl = cdll.LoadLibrary(libfullpath)
    
    #Create empty dictionaries container
    self._dics = c_void_p(self._hl.new_ht_dicts())
    
    #Create empty dictionaries dictionary
    self.dl = {}
    
    #Set as None _current_tref
    self._current_tref = None
    
    if filename is not None:
      #Load dictionaries from file
      self._hl.ht_load_from_file(self._dics, filename)
      
      #Get number of tables
      _nt = self._hl.ht_get_num_of_tables(self._dics)
      
      #Load tables names and references
      for i in range(0, _nt):
        _currt = self._get_table_by_ref(self._get_next_tref())
        _currt_name = self._get_table_name(_currt)
        self.dl[_currt_name.decode()] = htdict(self._hl, _currt)
  
  def get_num_of_tables(self):
    return self._hl.ht_get_num_of_tables(self._dics)
  
  def _get_first_tref(self):
    return c_void_p(self._hl.ht_get_first_table_ref(self._dics))
    
  def _get_next_tref(self):
    if self._current_tref is None:
      self._current_tref = self._get_first_tref()
      return self._current_tref
    else:
      self._current_tref = c_void_p(
        self._hl.ht_get_next_table_ref(self._dics, self._current_tref))
      return self._current_tref
  
  def _get_table_by_ref(self, tref):
    return c_void_p(self._hl.ht_get_table_by_ref(self._dics, tref))
  
  def _get_table_name(self, ptable):
    self._hl.ht_get_table_name.restype = c_char_p
    return self._hl.ht_get_table_name(ptable)
    
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
    return self._hl.ht_save_dicts(self._dics, file_name)
