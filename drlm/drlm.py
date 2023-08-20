#!/usr/bin/env python3

LOGIN_MANAGER_LOG_PATH = '/var/log/drlm'
LOGIN_MANAGER_MOD_PATH = '/opt/home/maverick/work/drm-framebuffer/drm-splash4slack/'
LOGIN_MANAGER_RES_PATH = '/opt/home/maverick/work/drm-framebuffer/drm-splash4slack/resources/'

import sys
from time import sleep

sys.path.append(LOGIN_MANAGER_MOD_PATH)

from modules.lmlog.lmlog import *

from modules.lmutils.lmutils import utils
from modules.lminterface.lminterface import loop_man
from modules.lmauthenticate.lmauthenticate import authenticate_user

tty_num = None

if __name__ == '__main__':

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
        lmlog_log('Missing argument for %s' % sys.argv[i])
        sys.exit(1)
    elif sys.argv[i] == '--dbg-level':
      if i+1 < nargs:
        opts['debug_level'] = int(sys.argv[i+1])
        i += 2
      else:
        lmlog_log('Missing argument for %s' % sys.argv[i])
        sys.exit(1)
    else:
      i += 1
  
  #if 'debug_level' in opts:
  #  drlm_dbg_level = opts['debug_level']
  #else:
  #  drlm_dbg_level = 0
  
  lm_log_path = '.'
  if 'tty' in opts:
    lm_log_path = LOGIN_MANAGER_LOG_PATH
  
  # Initialize logger
  init_lm_log(lm_log_path + '/' + 'drlm.log')
  
  # Log start
  lmlog_log_start()
  
  # Initialize Utils
  U = utils()
  
  if 'tty' in opts:
    if U.open_tty(opts['tty']) != 0:
      lmlog_log('Error opening tty %s!' % opts['tty'])
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
  
  # Wait for tty to become active again
  #U.wait_for_tty(tty_num)
  
  cbuff = b'\x00' * 50
      
  L = loop_man(LOGIN_MANAGER_RES_PATH)
  L.initialize_screens(LOGIN_MANAGER_RES_PATH + 'fonts2.baf', 
                       0x00000000, 
                       LOGIN_MANAGER_RES_PATH + 'MountainSky.baf')
  
  L.reset_if()
  
  # Set auth token to None
  authok = None
  
  # Main loop 
  while(True):
    r = U.readc(cbuff)
      
    # Ctrl-C shutdown by now
    if r==1 and cbuff[0]==3:
      res = {'shutdown': 'Yes'}
      authok = res
      break
    
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
        if auth_res['res'] == 'PWentNotFound' or \
          auth_res['res'] == 'SPentNotFound':
          _username = None
        elif auth_res['res'] == 'SPentBadPwd':
          _username = auth_res['usrname']
        
        if 'tty' in opts:
          U.log_btmp(0, opts['tty'], _username)
        
        if _username is None:
          lmlog_log('Failed auth')
        else:
          lmlog_log('Failed auth for %s' % _username)
          
        L.show_login_err_msg()
        sleep(3)
        L.reset_if()
      
      elif auth_res['ans'] == True:
        auth_res['session'] = _lp['session']
        L.show_login_success_msg()
        sleep(4)
        
        # Set authok
        authok = auth_res
        
        # Log successfull auth has succeded
        lmlog_log('user \'%s\' successfull authenticated.' % 
          auth_res['res']['pw_name'])
        
        # Quit main loop
        break
    
    else:
      L.lselect(cbuff, r)
  
  
  L._S.quit_videodev()
  U.restore_terminal_mode()
  
  
  import os
  
  # Sanity checks
  if authok is None:
    sleep(3)
    sys.exit(0)
    
  if 'shutdown' in authok:
    if authok['shutdown'] == 'Yes':
      os.execv('/sbin/shutdown', ['/sbin/shutdown', '-h', 'now'])
      
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
    
    # Default to passwd shell
    cmd = shell
    cmdargs = [shell]
    
    # Check shell login options
    if 'session' in authok:
      if authok['session']['type'] == 'T':
        _sess = authok['session']['conf']
        if 'default' in _sess:
          if _sess['default'] == True:
            # Exec default passwd shell
            cmd = shell
            cmdargs = [shell]
        else:
          cmd = _sess['cmd']
          cmdargs = [_sess['cmd']]
      
      elif authok['session']['type'] == 'X':
        
        if not 'tty' in opts:
          print('Not started by init. Refusing to launch Xsession!')
          sleep(4)
          sys.exit(2)
          
        _sess = authok['session']['conf']
        if 'default' in _sess:
          if _sess['default'] == True:
            # Exec default passwd shell
            cmd = '/usr/bin/xinit'
            cmdargs = ['/usr/bin/xinit', '--', 'vt' + str(tty_num)]
    
    # Log
    full_cmd = ''
    for it in cmdargs:
      full_cmd += it + ' '
    lmlog_log('Launching shell \'%s\' for user \'%s\'' % 
      (full_cmd, username))
    
    # Launch the shell
    os.execv(cmd, cmdargs)
      
        
  # Parent
  else:
    # Close stdin, stdout and stderr
    os.close(0)
    os.close(1)
    os.close(2)
    
    # Wait for user shell to exit...
    os.wait()
