from ctypes import *

import os

class videoterm:
  def __init__(self, libh, vtobj, fun_prefix='_videoterm_'):
    self._lv = libh
    self._vt = vtobj
    
    self._fpre = fun_prefix
    
    self.__putc_advance_and_sync = \
      getattr(self._lv, self._fpre + 'putc_advance_and_sync')
      
    self.__sync_term = \
      getattr(self._lv, self._fpre + 'sync_term')
    
    self.__get_nrows = \
      getattr(self._lv, self._fpre + 'get_nrows')
    
    self.__get_ncols = \
      getattr(self._lv, self._fpre + 'get_ncols')
    
    self._tc = self.__get_ncols(self._vt)
    self._tr = self.__get_nrows(self._vt)
    
    self.__get_curx = \
      getattr(self._lv, self._fpre + 'get_curx')
      
    self.__get_cury = \
      getattr(self._lv, self._fpre + 'get_cury')
    
    self.__move_cur = \
      getattr(self._lv, self._fpre + 'move_cur')
    
    self.__move_cur_rel = \
      getattr(self._lv, self._fpre + 'move_cur_rel')
    
    self.__set_fontcolor = \
      getattr(self._lv, self._fpre + 'set_fontcolor')
    
  def vputc(self, c, sync=1, adv=1, lc=None):
    if lc is not None:
      _lc = lc
    else:
      _lc = len(c)
    
    self.__putc_advance_and_sync(self._vt, sync, adv, _lc, c)
  
  def sync_term(self):
    self.__sync_term(self._vt)
  
  def get_term_dims(self):
    return (self._tc, self._tr)
  
  def move_cur(self, mv, mtype='abs'):
    if   mtype == 'abs':
      self.__move_cur(self._vt, c_int(mv[0]), c_int(mv[1]))
    elif mtype == 'rel':
      self.__move_cur_rel(self._vt, c_int(mv[0]), c_int(mv[1]))
    elif mtype == 'prop':
      if mv[0] >=0:
        x = int((mv[0] * self._tc) / 1000)
      else:
        x = -1
      if mv[1] >=0:
        y = int((mv[1] * self._tr) / 1000)
      else:
        y = -1
      self.__move_cur(self._vt, c_int(x), c_int(y))
  
  def set_fontcolor(self, color):
    self.__set_fontcolor(self._vt, color)
    

class monitor:
  def __init__(self, libh, mon, mnum, fun_prefix='_monitor_'):
    self._lv = libh
    self._mon = mon
    self._monID = mnum
    
    self._fpre = fun_prefix
    
    self.__get_monitor_width = \
      getattr(self._lv, self._fpre + 'get_monitor_width')
    self.__get_monitor_height = \
      getattr(self._lv, self._fpre + 'get_monitor_height')
    self._w = self.__get_monitor_width(self._mon)
    self._h = self.__get_monitor_height(self._mon)
    
    self.__draw_rectangle = \
      getattr(self._lv, self._fpre + 'draw_rectangle')
    
    self.__videoterminal = \
      getattr(self._lv, self._fpre + 'videoterminal')
    
    print('PyHandler: monitor %d is %d x %d' % 
      (self._monID, self._w, self._h))
  
  def get_monitor_dims(self):
    return (self._w, self._h)
  
  def draw_rectangle(self, x0, y0, rw, rh, color):
    return self.__draw_rectangle(self._mon, x0, y0, rw, rh, color)
  
  def vterm(self, fontID=0):
    _vtobj = c_void_p(self.__videoterminal(self._mon, fontID))
    if _vtobj.value == None:
        print ("PyHandler: Error starting vterm on monitor %d" % self._monID)
        return None
    
    self._vt = videoterm(self._lv, _vtobj)
    
    return self._vt
    

class videodev:
  def __init__(self, card_devname=b'/dev/dri/card0',
               libname='drm_handler.so', libpath='', 
               fun_prefix='_drmvideo_'):
    
    if '__file__' in globals():
      libpath = os.path.dirname(__file__)
    
    fullpath = libpath + '/' + libname
    
    self._card_devname = card_devname
    
    # Load library
    self._lv = cdll.LoadLibrary(fullpath)
    
    # Initialize device
    self._vd = c_void_p(self._lv.new_drmvideo(self._card_devname))
    if self._vd.value == None:
      print('Cannot initialize ', self._card_devname)
      return None
    
    self._fpre = fun_prefix
    
    self.__destroy = \
      getattr(self._lv, self._fpre + 'destroy')
    
    self.__get_num_of_monitors = \
      getattr(self._lv, self._fpre + 'get_num_of_monitors')
    self._nmons = self.__get_num_of_monitors(self._vd)
    
    self.__get_monitor_by_ID = \
      getattr(self._lv, self._fpre + 'get_monitor_by_ID')
    
    self._mons = {}
    for i in range(0, self._nmons):
      _mobj = c_void_p(self.__get_monitor_by_ID(self._vd, i))
      
      if _mobj.value == None:
        print ("PyHandler: Error while fetching monitor %d" % i)
        return None
      
      mon = monitor(self._lv, _mobj, i)
      
      self._mons[i] = mon
      
    print('PyHandler: found %d monitors' % self._nmons)
    
    self.__setup_monitor = \
      getattr(self._lv, self._fpre + 'setup_monitor')
    
    self.__enable_monitor = \
      getattr(self._lv, self._fpre + 'enable_monitor')
    
    self.__enable_all_monitors = \
      getattr(self._lv, self._fpre + 'enable_all_monitors')
    
    self.__setup_all_monitors = \
      getattr(self._lv, self._fpre + 'setup_all_monitors')
      
    self.__activate_vts = \
      getattr(self._lv, self._fpre + 'activate_vts')
    
    self.__set_vts_fontcolor = \
      getattr(self._lv, self._fpre + 'set_vts_fontcolor')
    
    self.__redraw = \
      getattr(self._lv, self._fpre + 'redraw')
    
    self.__move_cur = \
      getattr(self._lv, self._fpre + 'move_cur')
    
    self.__move_cur_rel = \
      getattr(self._lv, self._fpre + 'move_cur_rel')
    
    self.__move_cur_prop = \
      getattr(self._lv, self._fpre + 'move_cur_prop')
    
    self.__vputc = \
      getattr(self._lv, self._fpre + 'vputc')
    
    self.__clear_pos = \
      getattr(self._lv, self._fpre + 'clear_pos')
      
    self.__clear_terms = \
      getattr(self._lv, self._fpre + 'clear_terms')
      
    self.__sync_terms = \
      getattr(self._lv, self._fpre + 'sync_terms')
      
    self.__load_font_from_file = \
      getattr(self._lv, self._fpre + 'load_font_from_file')
    
    self.__set_or_drop_master_mode = \
      getattr(self._lv, self._fpre + 'set_or_drop_master_mode')
    
  
  def set_or_drop_master_mode(self, vdmmode):
    return self.__set_or_drop_master_mode(self._vd, vdmmode)
  
  def redraw(self):
    self.__redraw(self._vd)
  
  def sync_terms(self):
    self.__sync_terms(self._vd)
  
  def move_cur(self, mv, mtype='abs'):
    if   mtype == 'abs':
      self.__move_cur(self._vd, c_int(mv[0]), c_int(mv[1]))
    elif mtype == 'rel':
      self.__move_cur_rel(self._vd, c_int(mv[0]), c_int(mv[1]))
    elif mtype == 'prop':
      self.__move_cur_prop(self._vd, c_int(mv[0]), c_int(mv[1]))
  
  def vputc(self, c, sync=1, adv=1, lc=None):
    if lc is not None:
      _lc = lc
    else:
      _lc = len(c)
    
    self.__vputc(self._vd, sync, adv, _lc, c)
    
  def clear_pos(self, sync=1):
    self.__clear_pos(self._vd, sync)
  
  def clear_terms(self):
    self.__clear_terms(self._vd)
      
  def get_num_of_monitors(self):
    return self._nmons
  
  def setup_monitor(self, monID):
    return self.__setup_monitor(self._vd, monID)
  
  def setup_all_monitors(self):
    return self.__setup_all_monitors(self._vd)
    
  def enable_all_monitors(self):
    return self.__enable_all_monitors(self._vd)
  
  def activate_vts(self, fontID):
    return self.__activate_vts(self._vd, fontID)
  
  def set_vts_fontcolor(self, font_color):
    return self.__set_vts_fontcolor(self._vd, font_color)
    
  def enable_monitor(self, monID):
    return self.__enable_monitor(self._vd, monID)
  
  def get_monitor_by_ID(self, monID):
    return self._mons[monID]
  
  def load_font_from_file(self, font_path):
    self.__load_font_from_file(self._vd, font_path.encode())
    
  def quit_videodev(self):
    self.__destroy(self._vd)
