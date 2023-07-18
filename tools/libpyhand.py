import ctypes

ht_dict = ctypes.cdll.LoadLibrary('./ht_dict.so')

r = ht_dict.new_ht_dict(b"./ver_0_1_prova2.baf", b"", b"", 0, 0, 0, 0, 0, 0)

print ("Calling ht_print_data()")

out = ht_dict.ht_print_data(ctypes.c_void_p(r))

print ("Returned %d" % out)
