from drmhandler import videodev

from time import sleep

vd = videodev(libpath='../')

vd.setup_monitor(0)
vd.enable_monitor(0)

mon0 = vd.get_monitor_by_ID(0)

mon0.draw_rectangle(500, 300, 800, 600, 0x0000aaaa)

sleep(4)



vd.quit_videodev()
