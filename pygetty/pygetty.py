import sys

sys.path.append('../')

from modules.kbdhandler.kbdhandler import kbdman

from modules.drmhandler.drmhandler import videodev


vd = videodev()

vd.setup_monitor(0)
vd.enable_monitor(0)

mon0 = vd.get_monitor_by_ID(0)

vd.load_font_from_file('../fontsgen/fonts2.baf')

vt0 = mon0.vterm()

vt0.set_fontcolor(0x00ffffff)

kbdh = kbdman()

kbdh.set_terminal_mode()

cbuff = b'\x00' * 10

# Main loop 
while(True):
  r = kbdh.readc(cbuff)
  
  #print('res bytes = %d' % r)
  #print('read = ', cbuff)
  
  if r==1 and cbuff[0]==ord('q'):
    print ('Bye!')
    break

  elif r==1 and cbuff[0]==ord('t'):
    #print ('Switch tty')
    vd.set_or_drop_master_mode(0)
    kbdh.switch_tty(1)
    vd.set_or_drop_master_mode(1)
    vd.redraw()
  
  else:
    vt0.vputc(cbuff, lc=r)


vd.quit_videodev()
