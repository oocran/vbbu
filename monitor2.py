# monitor compute resources. Called from 'lib/oocran/oocran.c' with dependencies already imported and variables defined previously

for proc in psutil.process_iter():
    process = psutil.Process(proc.pid)
    pname = process.name()
    if pname == "pdsch_enodeb" or pname == "pdsch_ue":
        percent = process.cpu_percent(interval=0.1)

        cpu = 'cpu_' + NVF + ' value=%s' % percent
        disk = 'disk_' + NVF + ' value=%s' % psutil.disk_usage('/').percent
        ram = 'ram_' + NVF + ' value=%s' % round(process.memory_percent(), 2)

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
            continue

        network_in = 'network_in_' + NVF + ' value=%s' % ul
        network_out = 'network_out_' + NVF + ' value=%s' % dl

        requests.post("http://%s:8086/write?db=%s" % (IP, DB), auth=(USER, PASSWORD), data=cpu)
        requests.post("http://%s:8086/write?db=%s" % (IP, DB), auth=(USER, PASSWORD), data=disk)
        requests.post("http://%s:8086/write?db=%s" % (IP, DB), auth=(USER, PASSWORD), data=ram)
        requests.post("http://%s:8086/write?db=%s" % (IP, DB), auth=(USER, PASSWORD), data=network_in)
        requests.post("http://%s:8086/write?db=%s" % (IP, DB), auth=(USER, PASSWORD), data=network_out)

