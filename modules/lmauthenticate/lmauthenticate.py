import pwd, spwd, crypt

def authenticate_user(pwnam, clear_pwd):
  
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
