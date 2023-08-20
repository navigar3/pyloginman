from modules.lmlog.lmlog import *

import os

from modules.drmhandler.drmhandler import videodev

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
         'X_session_flag' : 
            {'pos': (('prop', 200, 400), ('rel', -8, 4)),
             'value': 'X'},
         'invalid_login': 
            [{'pos': (('prop', 500, 500), ('rel', -12, 0)), 
             'value': 'Retry,'},
             {'pos': (('prop', 500, 500), ('rel', -12, 2)), 
             'value': ' and be more lucky!'}],
         'successfull_login': 
            [{'pos': (('prop', 500, 500), ('rel', -9, 0)), 
             'value': 'Login successfull!'}],
        }

SessionsPath = None

class usrname:
  def __init__(self):
    self._name = []
    
  def set_scrs_handler(self, scr):
    self._S = scr
  
  def reset_pos(self):
    for m in IDesc['username_prompt']['pos']:
      self._S.move_cur((m[1], m[2]), m[0])
    self._S.move_cur((len(self._name), 0), mtype='rel')
    self._S.vputc(b'_', adv=0)
  
  def leave_field(self):
    self._S.clear_pos(sync=0)
  
  def clear_field(self, clear_tiles=True):
    if clear_tiles:
      for m in IDesc['username_prompt']['pos']:
        self._S.move_cur((m[1], m[2]), m[0])
      _ln = len(self._name)
      for i in range(0, _ln):
        self._S.clear_pos(sync=0)
        self._S.move_cur((1, 0), mtype='rel')
      self._S.sync_terms()
      
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
        self._S.clear_pos(sync=0)
        self._S.move_cur((-1, 0), mtype='rel')
        self._S.vputc(b'_', adv=0)
      return 0
    
    try:
      c=(buff[0:lb]).decode()
      self._name.append(buff[0:lb])
      self._S.vputc(buff, sync=0, lc=lb)
      self._S.vputc(b'_', adv=0)
      return lb
    except:
      pass
    
    return 0


class pwdc:
  def __init__(self):
    self._name = []
  
  def set_scrs_handler(self, scr):
    self._S = scr
    
  def reset_pos(self):
    for m in IDesc['password_prompt']['pos']:
      self._S.move_cur((m[1], m[2]), m[0])
    self._S.move_cur((len(self._name), 0), mtype='rel')
    self._S.vputc(b'_', adv=0)
  
  def leave_field(self):
    self._S.clear_pos(sync=0)
  
  def clear_field(self, clear_tiles=True):
    if clear_tiles:
      for m in IDesc['password_prompt']['pos']:
        self._S.move_cur((m[1], m[2]), m[0])
      _ln = len(self._name)
      for i in range(0, _ln):
        self._S.clear_pos(sync=0)
        self._S.move_cur((1, 0), mtype='rel')
      self._S.sync_terms()
      
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
        self._S.clear_pos(sync=0)
        self._S.move_cur((-1, 0), mtype='rel')
        self._S.vputc(b'_', adv=0)
      return 0
    
    try:
      c=(buff[0:lb]).decode()
      self._name.append(buff[0:lb])
      self._S.vputc(b'*', sync=0)
      self._S.vputc(b'_', adv=0)
      return lb
    except:
      pass
    
    return 0


class sess:
  def __init__(self, sessions_path=None):
    self._name = ['default']
    self._lname = 0
    self._sess_type_def = 'X'
    self._sess_type = self._sess_type_def
    self._sess_num_t = 0
    self._sess_num_x = 0
    self._sessions = {'T': [], 'X':[]}
    self._sessions['T'].append(
      {'name': 'Default', 'cmd': None, 'default': True})
    self._sessions['X'].append(
      {'name': 'Your .xinitrc', 'xinitrc': None, 
       'vt': 'current_vt', 'default': True})
    self._sessions_path = sessions_path
    self.load_sessions()
  
  def set_scrs_handler(self, scr):
    self._S = scr
  
  def load_sessions(self):
    if not self._sessions_path is None:
      _t_path = self._sessions_path + '/sessions/shells'
      _x_path = self._sessions_path + '/sessions/xsessions'
      try:
        _td = os.listdir(_t_path)
        _xd = os.listdir(_x_path)
      except:
        return False
      
      for it in _td:
        try:
          with open(_t_path + '/' + it, 'r') as f:
            for line in f:
              keyval = line.replace('\n', '').split('=')
              for i in range(0, len(keyval)):
                keyval[i] = keyval[i].strip()
              if keyval[0] == 'Name':
                _name = keyval[1]
              elif keyval[0] == 'Shell':
                _cmd = keyval[1]
            self._sessions['T'].append({'name': _name, 'cmd': _cmd})
        except:
          lmlog_log('Error while opening ', it)
      
      for it in _xd:
        try:
          with open(_x_path + '/' + it, 'r') as f:
            for line in f:
              keyval = line.replace('\n', '').split('=')
              for i in range(0, len(keyval)):
                keyval[i] = keyval[i].strip()
              if keyval[0] == 'Name':
                _name = keyval[1]
              elif keyval[0] == 'Xinitrc':
                _xinitrc = keyval[1]
            self._sessions['X'].append(
              {'name': _name, 'xinitrc': _cmd, 'vt': 'current_vt'})
        except:
          lmlog_log('Error while opening ', it)
    
    return True
    
  def reset_pos(self):
    for m in IDesc['session_prompt']['pos']:
      self._S.move_cur((m[1], m[2]), m[0])
    self._S.move_cur((len(self._name), 0), mtype='rel')
    self._fill_field()
  
  def leave_field(self):
    pass
  
  def clear_field(self, clear_tiles=True):
    self._sess_type = self._sess_type_def
    self._sess_num_t = 0
    self._sess_num_x = 0
    self._fill_field(sync=False)
  
  def validate(self):
    if self._sess_type == 'X':
      nsess = self._sess_num_x
    elif self._sess_type == 'T':
      nsess = self._sess_num_t
    return ('session', 
            {'type': self._sess_type, 
             'conf': self._sessions[self._sess_type][nsess]})
  
  def _fill_field(self, sync=True):
    if self._sess_type == 'X':
      for m in IDesc['X_session_flag']['pos']:
        self._S.move_cur((m[1], m[2]), m[0])
      for l in IDesc['X_session_flag']['value']:
        self._S.vputc(l.encode(), sync=0)
      for m in IDesc['session_prompt']['pos']:
        self._S.move_cur((m[1], m[2]), m[0])
      for i in range(0, self._lname):
        self._S.clear_pos(sync=0)
        self._S.move_cur((1, 0), mtype='rel')
      for m in IDesc['session_prompt']['pos']:
        self._S.move_cur((m[1], m[2]), m[0])
      for l in self._sessions['X'][self._sess_num_x]['name']:
        self._S.vputc(l.encode(), sync=0)
      self._lname = len(self._sessions['X'][self._sess_num_x]['name'])
      
    elif self._sess_type == 'T':
      for m in IDesc['X_session_flag']['pos']:
        self._S.move_cur((m[1], m[2]), m[0])
      for l in IDesc['X_session_flag']['value']:
        self._S.clear_pos(sync=0)
        self._S.move_cur((1, 0), mtype='rel')
      for m in IDesc['session_prompt']['pos']:
        self._S.move_cur((m[1], m[2]), m[0])
      for i in range(0, self._lname):
        self._S.clear_pos(sync=0)
        self._S.move_cur((1, 0), mtype='rel')
      for m in IDesc['session_prompt']['pos']:
        self._S.move_cur((m[1], m[2]), m[0])
      for l in self._sessions['T'][self._sess_num_t]['name']:
        self._S.vputc(l.encode(), sync=0)
      self._lname = len(self._sessions['T'][self._sess_num_t]['name'])
    
    if sync == True:
      self._S.sync_terms()
  
  def pushch(self, buff, lb):
    
    # Space switchs between Text and X sessions
    if lb == 1 and buff[0] == ord(' '):
      if self._sess_type == 'X':
        self._sess_type = 'T'
        self._fill_field()
      elif self._sess_type == 'T':
        self._sess_type = 'X'
        self._fill_field()
    
    # p and n switchs between Text and X sessions
    elif lb == 1 and (buff[0] == ord('n') or buff[0] == ord('N')):
      if self._sess_type == 'X':
        self._sess_num_x += 1
        if self._sess_num_x >= len(self._sessions['X']):
          self._sess_num_x = 0
        self._fill_field()
      elif self._sess_type == 'T':
        self._sess_num_t += 1
        if self._sess_num_t >= len(self._sessions['T']):
          self._sess_num_t = 0
        self._fill_field()
    elif lb == 1 and (buff[0] == ord('p') or buff[0] == ord('P')):
      if self._sess_type == 'X':
        self._sess_num_x -= 1
        if self._sess_num_x < 0:
          self._sess_num_x = len(self._sessions['X']) - 1
        self._fill_field()
      elif self._sess_type == 'T':
        self._sess_num_t -= 1
        if self._sess_num_t < 0:
          self._sess_num_t = len(self._sessions['T']) - 1
        self._fill_field()
      
    return 0


class loop_man:
  def __init__(self, sessions_path=None):
    self._fs = [usrname(), pwdc(), sess(sessions_path)]
    self._fn = 0
    self._f = self._fs[self._fn]
    
  def initialize_screens(self, font_path, font_color, image_path):
    self._S = videodev()

    self._S.setup_all_monitors()
    self._S.enable_all_monitors()
    
    self._S.load_font_from_file(font_path)
    self._S.load_image_from_file(image_path)
    
    self._S.activate_vts(0)
    self._S.set_vts_fontcolor(font_color)
    
    self._S.set_backgrounds()
    
    for f in self._fs:
      f.set_scrs_handler(self._S)
  
  def set_sessions_path(self, path):
    SessionsPath = path
    
  def reset_if(self):
    self._S.clear_terms(sync=0)
    
    for f in self._fs:
      f.clear_field(clear_tiles=False)
    
    self._fn = 0
    self._f = self._fs[self._fn]
    
    for field in ('username', 'password', 'session'):
      for m in IDesc[field]['pos']:
        self._S.move_cur((m[1], m[2]), m[0])
      for l in IDesc[field]['value']:
        self._S.vputc(l.encode(), sync=0)
    for m in IDesc['username_prompt']['pos']:
      self._S.move_cur((m[1], m[2]), m[0])
      
    self._S.vputc(b'_', adv=0)
  
  def show_login_err_msg(self):
    self._S.clear_terms(sync=0)
    for l in IDesc['invalid_login']:
      for m in l['pos']:
        self._S.move_cur((m[1], m[2]), m[0])
      for v in l['value']:
        self._S.vputc(v.encode(), sync=0)
    self._S.sync_terms()
  
  def show_login_success_msg(self):
    self._S.clear_terms(sync=0)
    for l in IDesc['successfull_login']:
      for m in l['pos']:
        self._S.move_cur((m[1], m[2]), m[0])
      for v in l['value']:
        self._S.vputc(v.encode(), sync=0)
    self._S.sync_terms()
  
  def validate(self):
    _login_par = {}
    _ifn = 0
    for f in self._fs:
      v = f.validate()
      if v is None:
        _login_par = None
        self._fn = _ifn
        self._f = f
        self._S.clear_pos(sync=0)
        f.reset_pos()
        break
      else:
        _ifn += 1
        _login_par[v[0]] = v[1]
    
    return _login_par
  
  def lselect(self, buff, lb):
    # Tab key
    if lb == 1 and buff[0] == 9:
      self._f.leave_field()
      
      self._fn += 1
      if self._fn >= len(self._fs):
        self._fn = 0
      self._f = self._fs[self._fn]
      
      self._f.reset_pos()
      
    else:
      self._f.pushch(buff, lb)
