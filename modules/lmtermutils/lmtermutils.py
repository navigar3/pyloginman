from ctypes import *

import os

class termh:
  def __init__(self, libname='term_utils.so', libpath=''):
    
    if '__file__' in globals():
      libpath = os.path.dirname(__file__)
    
    fullpath = libpath + '/' + libname
    
    # Load library
    self._lib = cdll.LoadLibrary(fullpath)
  
  def open_tty(self, tty):
    return self._lib.open_tty(tty.encode())
    
  def set_terminal_mode(self):
    self._lib.set_terminal_mode()
  
  def restore_terminal_mode(self):
    self._lib.restore_terminal_mode()
  
  def get_drm_Set_Drop_handler(self):
    return {'set_handler': self._lib.video_set_master_mode,
            'drop_handler': self._lib.video_drop_master_mode}
  
  def set_drm_Set_Drop_handlers(self, funset, fundrop):
    self._lib.video_set_master_mode = CFUNCTYPE(c_int)(funset)
    self._lib.video_drop_master_mode = CFUNCTYPE(c_int)(fundrop)
  
  def get_curr_tty_num(self):
    return self._lib.get_curr_tty_num()
  
  def readc(self, cbuff):
    return self._lib.blocking_read_multibytes_from_term(cbuff)
  
  def switch_tty(self, ttyn):
    self._lib.switch_tty_and_wait(ttyn)
    
    
