from ctypes import *

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
    
    print('PyHandler: monitor %d is %d x %d' % 
      (self._monID, self._w, self._h))
  
  def draw_rectangle(self, x0, y0, rw, rh, color):
    return self.__draw_rectangle(self._mon, x0, y0, rw, rh, color)
    
    

class videodev:
  def __init__(self, card_devname=b'/dev/dri/card0',
               libname='drm_handler.so', libpath='', 
               fun_prefix='_drmvideo_'):
    
    fullpath = libpath + libname
    
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
  
  
  def setup_monitor(self, monID):
    return self.__setup_monitor(self._vd, monID)
    
  def enable_monitor(self, monID):
    return self.__enable_monitor(self._vd, monID)
  
  def get_monitor_by_ID(self, monID):
    return self._mons[monID]
    
  def quit_videodev(self):
    self.__destroy(self._vd)
  
