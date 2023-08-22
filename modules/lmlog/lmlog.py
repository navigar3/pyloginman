import os, sys, time, struct
from ctypes import *


_libpath = ''
_libname = 'log.so'

if '__file__' in globals():
  _libpath = os.path.dirname(__file__)

fullpath = _libpath + '/' + _libname
ll = cdll.LoadLibrary(fullpath)

# Log file descriptor
drlm_log_fd = struct.pack('<i', -1)

# Debug Level
drlm_dbg_level = struct.pack('<i', 0)

def set_lmlog_fd(lib=None):
  if lib is None:
    ll.set_log_fd(drlm_log_fd)
  else:
    try:
      getattr(lib, 'set_log_fd')
    except AttributeError:
      lmlog_log('Cannot set log fd on external library!')
      return
    lib.set_log_fd(drlm_log_fd)

def set_lmlog_dbg_level(lib=None):
  if lib is None:
    ll.set_log_dbg_level(drlm_dbg_level)
  else:
    try:
      getattr(lib, 'set_log_dbg_level')
    except AttributeError:
      lmlog_log('Cannot set log fd on external library!')
      return
    lib.set_log_dbg_level(drlm_dbg_level)

def setup_lmlog_dbg_level(level):
  global drlm_dbg_level
  drlm_dbg_level = struct.pack('<i', level)
  lmlog_log('Debug level set %d' % level)

def init_lm_log(log_file_path, dbg_level=0):
  fd = ll.init_logger(log_file_path.encode())
  global drlm_log_fd
  drlm_log_fd = struct.pack('<i', fd)
  set_lmlog_fd()

def lmlog_log_start():
  start_msg = sys.argv[0] + ' start whith pid: ' + str(os.getpid())
  if len(sys.argv) > 1:
    start_msg += '. Command line: '
    for it in sys.argv[1:]:
      start_msg += it + ' '
  else:
    start_msg += '.'
  lmlog_log('*************')
  lmlog_log(start_msg)

def get_drlm_log():
  return drlm_log
  
def lmlog_log(msg):
  ll.log_msg(msg.encode())
  
def lmlog_warn(msg):
  ll.log_warn(msg.encode())

def lmlog_err(msg):
  ll.log_err(msg.encode())

def lmlog_dbg(level, msg):
  ll.log_dbg(level, msg.encode())
