# monitor compute resources. Called from 'lib/oocran/oocran.c' with dependencies already imported and variables defined previously

cpu = 'cpu_' + NVF + ' value=%s' % psutil.cpu_percent()
disk = 'disk_' + NVF + ' value=%s' % psutil.disk_usage('/').percent
ram = 'ram_' + NVF + ' value=%s' % round(psutil.virtual_memory().used, 2)

last_up_down = up_down
upload = psutil.net_io_counters(pernic=True)[interface][0]
download = psutil.net_io_counters(pernic=True)[interface][1]
t1 = time.time()
up_down = (upload, download)
try:
    ul, dl = [(now - last) / (t1 - t0) / 1024.0
              for now, last in zip(up_down, last_up_down)]
    t0 = time.time()
except:
    pass

network_in = 'network_in_' + NVF + ' value=%s' % ul
network_out = 'network_out_' + NVF + ' value=%s' % dl

requests.post("http://%s:8086/write?db=%s" % (IP, DB), auth=(USER, PASSWORD), data=cpu)
requests.post("http://%s:8086/write?db=%s" % (IP, DB), auth=(USER, PASSWORD), data=disk)
requests.post("http://%s:8086/write?db=%s" % (IP, DB), auth=(USER, PASSWORD), data=ram)
requests.post("http://%s:8086/write?db=%s" % (IP, DB), auth=(USER, PASSWORD), data=network_in)
requests.post("http://%s:8086/write?db=%s" % (IP, DB), auth=(USER, PASSWORD), data=network_out)

