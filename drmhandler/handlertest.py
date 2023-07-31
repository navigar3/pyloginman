from drmhandler import videodev

from time import sleep

vd = videodev(libpath='../')

vd.setup_monitor(0)
vd.enable_monitor(0)

mon0 = vd.get_monitor_by_ID(0)

vd.load_font_from_file('../fontsgen/fonts2.baf')

vt0 = mon0.vterm()

vt0.set_fontcolor(0x00ffffff)

#mon0.draw_rectangle(0, 0, 800, 600, 0x0000aaaa)

vt0.move_cur(8, 5)

for i in range(0, 1000):
  c = chr((i % 93) + 33).encode()
  vt0.vputc(c, sync=0)

sleep(1)

vt0.vputc(b'P')

sleep(4)



vd.quit_videodev()
