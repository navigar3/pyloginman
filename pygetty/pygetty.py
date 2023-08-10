#!/usr/bin/env python3

import sys
from time import sleep

sys.path.append('../')

from modules.lmtermutils.lmtermutils import termh
from modules.lminterface.lminterface import loop_man
from modules.lmauthenticate.lmauthenticate import authenticate_user

if __name__ == '__main__':
  T = termh()
  
  T.set_terminal_mode()
  
  cbuff = b'\x00' * 10
      
  L = loop_man()
  L.initialize_screens('../fontsgen/fonts2.baf', 
                       0x0000ffff, 
                       '../tools/FallenLeaf.baf')
  L.reset_if()
  
  # Set auth token to None
  authok = None
  
  # Main loop 
  while(True):
    r = T.readc(cbuff)
    
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
          L._S.set_or_drop_master_mode(0)
          kbdh.switch_tty(sfn)
          L._S.set_or_drop_master_mode(1)
          L._S.redraw()
        
    # Catch Enter Key
    elif r == 1 and cbuff[0] == 13:
      _lp = L.validate()
      
      if _lp is None:
        continue
        
      auth_res = authenticate_user(_lp['username'].decode(), 
                                   _lp['password'].decode())
      
      if auth_res['ans'] == False:
        L.show_login_err_msg()
        sleep(3)
        L.reset_if()
      
      elif auth_res['ans'] == True:
        L.show_login_success_msg()
        sleep(4)
        
        # Set authok
        authok = auth_res
        
        # Quit main loop
        break
    
    else:
      L.lselect(cbuff, r)
  
  
  L._S.quit_videodev()
  T.restore_terminal_mode()
  
  
  import os
  
  # Fork
  pid = os.fork()
  
  #Child
  if pid == 0:
    # Now open a shell for authenticated user
    if authok is not None:
      
      # Check for keys in authok
      for key in ('pw_uid', 'pw_gid', 'pw_shell', 'pw_dir'):
        if not key in authok['res']:
          print('Cannot login!')
          sys.exit(1)
      
      # Get user uid, gid, shell and home
      uid = authok['res']['pw_uid']
      gid = authok['res']['pw_gid']
      shell = authok['res']['pw_shell']
      home = authok['res']['pw_dir']
      
      # Drop privileges
      os.setgid(gid)
      os.setegid(gid)
      os.setuid(uid)
      os.seteuid(uid)
      
      # Change directory
      os.chdir(home)
      
      # Set safe IFS
      if 'IFS' in os.environ:
        os.environ['IFS'] = " \t\n"
      
      # Set HOME env
      os.environ['HOME'] = home
      
      # Launch the shell
      os.execv(shell, [shell])
  
  # Parent
  else:
    # Close stdin, stdout and stderr
    os.close(0)
    os.close(1)
    os.close(2)
    
    # Wait for user shell to exit...
    os.wait()
  
