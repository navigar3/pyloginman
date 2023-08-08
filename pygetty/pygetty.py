import sys
from time import sleep

import pwd, spwd, crypt

sys.path.append('../')

from modules.kbdhandler.kbdhandler import kbdman

from modules.drmhandler.drmhandler import videodev


S = videodev()

S.setup_all_monitors()
S.enable_all_monitors()

S.load_font_from_file('../fontsgen/fonts2.baf')
S.load_image_from_file('../tools/FallenLeaf.baf')

S.activate_vts(0)
S.set_vts_fontcolor(0x00ffffff)


IDesc = {'username': {'pos': (('prop', 200, 400), ('rel', -8, 0)), 
                      'value': 'username: '},
         'password': {'pos': (('prop', 200, 400), ('rel', -8, 2)), 
                      'value': 'password: '},
         'session':  {'pos': (('prop', 200, 400), ('rel', -7, 4)), 
                      'value': 'session: '},
         'username_prompt' : 
            {'pos': (('prop', 200, 400), ('rel', 2, 0))},
         'password_prompt' : 
            {'pos': (('prop', 200, 400), ('rel', 2, 2))},
         'session_prompt' : 
            {'pos': (('prop', 200, 400), ('rel', 2, 4))},
         'invalid_login': {'pos': (('prop', 500, 500), ('rel', -12, 0)), 
                      'value': 'Retry, and be more lucky!'},
        }

kbdh = kbdman()

kbdh.set_terminal_mode()

cbuff = b'\x00' * 10


class usrname:
  def __init__(self):
    self._name = []
  
  def reset_pos(self):
    for m in IDesc['username_prompt']['pos']:
      S.move_cur((m[1], m[2]), m[0])
    S.move_cur((len(self._name), 0), mtype='rel')
    S.vputc(b'_', adv=0)
  
  def clear_field(self, clear_tiles=True):
    if clear_tiles:
      for m in IDesc['username_prompt']['pos']:
        S.move_cur((m[1], m[2]), m[0])
      _ln = len(self._name)
      for i in range(0, _ln):
        S.clear_pos(sync=0)
        S.move_cur((1, 0), mtype='rel')
      S.sync_terms()
      
    self._name = []
  
  def validate(self):
    if len(self._name)>0:
      _nam = b''
      for l in self._name:
        _nam += l
      return ('username', _nam)
    else:
      return None
  
  def pushch(self, buff, lb):
    
    if lb == 1 and buff[0] == 127:
      if len(self._name)>0:
        self._name.pop()
        S.clear_pos(sync=0)
        S.move_cur((-1, 0), mtype='rel')
        S.vputc(b'_', adv=0)
      return 0
    
    try:
      c=(buff[0:lb]).decode()
      self._name.append(buff[0:lb])
      S.vputc(buff, sync=0, lc=lb)
      S.vputc(b'_', adv=0)
      return lb
    except:
      pass
    
    return 0


class pwdc:
  def __init__(self):
    self._name = []
    
  def reset_pos(self):
    for m in IDesc['password_prompt']['pos']:
      S.move_cur((m[1], m[2]), m[0])
    S.move_cur((len(self._name), 0), mtype='rel')
    S.vputc(b'_', adv=0)
  
  def clear_field(self, clear_tiles=True):
    if clear_tiles:
      for m in IDesc['password_prompt']['pos']:
        S.move_cur((m[1], m[2]), m[0])
      _ln = len(self._name)
      for i in range(0, _ln):
        S.clear_pos(sync=0)
        S.move_cur((1, 0), mtype='rel')
      S.sync_terms()
      
    self._name = []
  
  def validate(self):
    if len(self._name)>0:
      _nam = b''
      for l in self._name:
        _nam += l
      return ('password', _nam)
    else:
      return None
  
  def pushch(self, buff, lb):
    
    if lb == 1 and buff[0] == 127:
      if len(self._name)>0:
        self._name.pop()
        S.clear_pos(sync=0)
        S.move_cur((-1, 0), mtype='rel')
        S.vputc(b'_', adv=0)
      return 0
    
    try:
      c=(buff[0:lb]).decode()
      self._name.append(buff[0:lb])
      S.vputc(b'*', sync=0)
      S.vputc(b'_', adv=0)
      return lb
    except:
      pass
    
    return 0
    

class loop_man:
  def __init__(self):
    self._fs = [usrname(), pwdc()]
    self._fn = 0
    self._f = self._fs[self._fn]
    
  def reset_if(self):
    S.clear_terms()
    
    for f in self._fs:
      f.clear_field(clear_tiles=False)
    
    self._fn = 0
    self._f = self._fs[self._fn]
    
    for field in ('username', 'password', 'session'):
      for m in IDesc[field]['pos']:
        S.move_cur((m[1], m[2]), m[0])
      for l in IDesc[field]['value']:
        S.vputc(l.encode(), sync=0)
    for m in IDesc['username_prompt']['pos']:
      S.move_cur((m[1], m[2]), m[0])
      
    S.sync_terms()
    S.vputc(b'_', adv=0)
  
  def validate(self):
    _login_par = {}
    _ifn = 0
    for f in self._fs:
      v = f.validate()
      if v is None:
        _login_par = None
        self._fn = _ifn
        self._f = f
        S.clear_pos(sync=0)
        f.reset_pos()
        break
      else:
        _ifn += 1
        _login_par[v[0]] = v[1]
    
    return _login_par
  
  def lselect(self, buff, lb):
    # Tab key
    if lb == 1 and buff[0] == 9:
      self._fn += 1
      if self._fn >= len(self._fs):
        self._fn = 0
      self._f = self._fs[self._fn]
      
      S.clear_pos(sync=0)
      self._f.reset_pos()
      
    else:
      self._f.pushch(buff, lb)
    

def authenticate_user(pwnam, clear_pwd):
  
  print(pwnam)
  
  try:
    u = pwd.getpwnam(pwnam)
  except KeyError:
    return {'ans': False, 'res': 'PWentNotFound'}
  
  if not u.pw_passwd == 'x':
    return {'ans': False, 'res': 'NotShadowPwd'}
  
  try:
    sp = spwd.getspnam(pwnam)
  except KeyError:
    return {'ans': False, 'res': 'SPentNotFound'}
  
  if crypt.crypt(clear_pwd, sp.sp_pwd) == sp.sp_pwd:
    return {'ans': True,
            'res': {'pw_name': u.pw_name,
                    'pw_uid': u.pw_uid,
                    'pw_gid': u.pw_gid,
                    'pw_dir': u.pw_dir,
                    'pw_shell': u.pw_shell} 
           }
  else:
    return {'ans': False, 'res': 'SPentBadPwd'}
    

L = loop_man()
L.reset_if()

# Main loop 
while(True):
  r = kbdh.readc(cbuff)
  
  # Ctrl-C breaks by now
  if r==1 and cbuff[0]==3:
    print ('Bye!')
    break
  
  # Catch Shift-Fn sequences
  elif r==5 and cbuff[0] == 27:
    if cbuff[1] == ord('[') and cbuff[4] == ord('~'):
      sfn = None
      if   cbuff[2] == ord('2') and cbuff[3] == ord('5'):
        sfn = 1
      elif cbuff[2] == ord('2') and cbuff[3] == ord('6'):
        sfn = 2
      elif cbuff[2] == ord('2') and cbuff[3] == ord('8'):
        sfn = 3
      elif cbuff[2] == ord('2') and cbuff[3] == ord('9'):
        sfn = 4
      elif cbuff[2] == ord('3') and cbuff[3] == ord('1'):
        sfn = 5
      elif cbuff[2] == ord('3') and cbuff[3] == ord('2'):
        sfn = 6
      elif cbuff[2] == ord('3') and cbuff[3] == ord('3'):
        sfn = 7
      elif cbuff[2] == ord('3') and cbuff[3] == ord('4'):
        sfn = 8
      
    if not sfn is None:
      if not sfn == kbdh.get_curr_tty_num():
        S.set_or_drop_master_mode(0)
        kbdh.switch_tty(sfn)
        S.set_or_drop_master_mode(1)
        S.redraw()
      
  # Catch Enter Key
  elif r == 1 and cbuff[0] == 13:
    _lp = L.validate()
    if not _lp is None:
      #S.clear_terms()
      #for m in IDesc['invalid_login']['pos']:
      #  S.move_cur((m[1], m[2]), m[0])
      #for l in IDesc['invalid_login']['value']:
      #  S.vputc(l.encode(), sync=0)
      #S.sync_terms()
      
      #sleep(3)
      #L.reset_if()
      auth_res = authenticate_user(_lp['username'].decode(), 
                                   _lp['password'].decode())
      print(auth_res)
  
  else:
    L.lselect(cbuff, r)


S.quit_videodev()
