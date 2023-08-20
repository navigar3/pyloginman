from modules.lmlog.lmlog import *

from ctypes import *

import os

class utils:
  def __init__(self, libname='utils.so', libpath=''):
    
    if '__file__' in globals():
      libpath = os.path.dirname(__file__)
    
    fullpath = libpath + '/' + libname
    
    # Load library
    self._lib = cdll.LoadLibrary(fullpath)
    
    # Set log fd
    set_lmlog_fd(self._lib)
    set_lmlog_dbg_level(self._lib)
  
  def open_tty(self, tty):
    return self._lib.open_tty(tty.encode())
  
  def detach_tty(self):
    return self._lib.detach_tty()
  
  def update_utmp(self, tty):
    self._lib.update_utmp(tty.encode())
  
  def log_utmp(self, pid, tty, username, hostname=None, hostaddr=None):
    if hostname is None:
      _hn = 0
    else:
      _hn = hostname.encode()
    if hostaddr is None:
      _ha = 0
    else:
      _ha = hostaddr.encode()
    self._lib.log_utmp(pid, tty.encode(), username.encode(), _hn, _ha)
  
  def log_btmp(self, pid, tty, username, hostname=None, hostaddr=None):
    if username is None:
      _us = 0
    else:
      _us = username.encode()
    if hostname is None:
      _hn = 0
    else:
      _hn = hostname.encode()
    if hostaddr is None:
      _ha = 0
    else:
      _ha = hostaddr.encode()
    self._lib.log_btmp(pid, tty.encode(), _us, _hn, _ha)
    
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
  
  def wait_for_tty(self, ttyn):
    self._lib.wait_for_tty_active(ttyn)
  
  def switch_tty(self, ttyn):
    self._lib.switch_tty_and_wait(ttyn)
    
    
