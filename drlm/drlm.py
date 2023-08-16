#!/usr/bin/env python3

LOGIN_MANAGER_MOD_PATH = '/opt/home/maverick/work/drm-framebuffer/drm-splash4slack/'
LOGIN_MANAGER_RES_PATH = '/opt/home/maverick/work/drm-framebuffer/drm-splash4slack/resources/'

import sys
from time import sleep

sys.path.append(LOGIN_MANAGER_MOD_PATH)

from modules.lmutils.lmutils import utils
from modules.lminterface.lminterface import loop_man
from modules.lmauthenticate.lmauthenticate import authenticate_user

tty_num = None

if __name__ == '__main__':
  
  sleep(3)

  # Parse cmdline args
  nargs = len(sys.argv)
  i = 1
  opts = {}
  while(i < nargs):
    if sys.argv[i] == '--tty':
      if i+1 < nargs:
        opts['tty'] = sys.argv[i+1]
        i += 2
      else:
        print('Missing argument for %s' % sys.argv[i])
        sys.exit(1)
  
  U = utils()
  
  if 'tty' in opts:
    if U.open_tty(opts['tty']) != 0:
      print('Error opening tty %s!' % opts['tty'])
      sleep(5)
      sys.exit(1)
    
    tty_num = int(opts['tty'][3:])
    
    # Wait for tty to become active
    U.wait_for_tty(tty_num)
    
  # Set terminal mode
  U.set_terminal_mode()
  
  if 'tty' in opts:
    # Update utmp
    U.update_utmp(opts['tty'])
  
  
  cbuff = b'\x00' * 50
      
  L = loop_man()
  L.initialize_screens(LOGIN_MANAGER_RES_PATH + 'fonts2.baf', 
                       0x00000000, 
                       LOGIN_MANAGER_RES_PATH + 'MountainSky.baf')
  L.reset_if()
  
  # Set auth token to None
  authok = None
  
  # Main loop 
  while(True):
    r = U.readc(cbuff)
      
    # Ctrl-C breaks by now
    #if r==1 and cbuff[0]==3:
    #  print ('Bye!')
    #  break
    
    # Catch Shift-Fn sequences
    if r==5 and cbuff[0] == 27:
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
        if not sfn == U.get_curr_tty_num():
          L._S.set_or_drop_master_mode(0)
          U.switch_tty(sfn)
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
  U.restore_terminal_mode()
  
  
  import os
  
  # First detach tty
  #if 'tty' in opts:
  #  U.detach_tty()
  
  # Sanity checks
  if authok is not None:
    for key in ('pw_name', 'pw_uid', 'pw_gid', 'pw_shell', 'pw_dir'):
      if not key in authok['res']:
        sleep(2)
        sys.exit(1)
  
  # Get user uid, gid, shell and home
  username = authok['res']['pw_name']
  uid = authok['res']['pw_uid']
  gid = authok['res']['pw_gid']
  shell = authok['res']['pw_shell']
  home = authok['res']['pw_dir']
  
  if 'tty' in opts:
    # Log utmp entry
    U.log_utmp(os.getpid(), opts['tty'], username)
  
  # Fork
  pid = os.fork()

  #Child
  if pid == 0:
    # Now open a shell for authenticated user
    
    if 'tty' in opts:
      # Start new session
      os.setsid()
      
      # Open tty
      U.open_tty(opts['tty'])

      # Get tty gid
      tty_gid = os.stat('/dev/' + opts['tty']).st_gid

      # Change tty owner
      os.chown('/dev/' + opts['tty'], uid, tty_gid)

      print('Started new session.')
      sleep(3)
    
    if uid == 0:
      print('Refuse to login as root.')
      sleep(2)
      sys.exit(2)
    
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
