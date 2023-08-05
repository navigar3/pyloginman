import sys

sys.path.append('../')

from modules.kbdhandler.kbdhandler import kbdman

from modules.drmhandler.drmhandler import videodev


S = videodev()

S.setup_all_monitors()
S.enable_all_monitors()

S.load_font_from_file('../fontsgen/fonts2.baf')

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
            {'pos': (('prop', 200, 400), ('rel', 2, 4))} 
        }

kbdh = kbdman()

kbdh.set_terminal_mode()

for field in ('username', 'password', 'session'):
  for m in IDesc[field]['pos']:
    S.move_cur((m[1], m[2]), m[0])
  for l in IDesc[field]['value']:
    S.vputc(l.encode(), sync=0)
for m in IDesc['username_prompt']['pos']:
  S.move_cur((m[1], m[2]), m[0])
S.sync_terms()
S.vputc(b'_', adv=0)

cbuff = b'\x00' * 10


class usrname:
  def __init__(self):
    self._name = []
  
  def reset_pos(self):
    for m in IDesc['username_prompt']['pos']:
      S.move_cur((m[1], m[2]), m[0])
    S.move_cur((len(self._name), 0), mtype='rel')
    S.vputc(b'_', adv=0)
  
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
    
L = loop_man()
    
# Main loop 
while(True):
  r = kbdh.readc(cbuff)
  
  # Ctrl-C breaks by now
  if r==1 and cbuff[0]==3:
    print ('Bye!')
    break

  # Ctrl-Z switch to tty1 by now
  elif r==1 and cbuff[0]==26:
    S.set_or_drop_master_mode(0)
    kbdh.switch_tty(1)
    S.set_or_drop_master_mode(1)
    S.redraw()
  
  else:
    L.lselect(cbuff, r)

print(L._fs[0]._name, L._fs[1]._name)

S.quit_videodev()
