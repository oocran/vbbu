#include <python2.7/Python.h>
#include "srslte/oocran/monitoring.h"

void oocran_monitoring_eNB(oocran_monitoring_eNB_t *q) {
  PyObject *py_main;
  Py_Initialize();
  py_main = PyImport_AddModule("__main__");
  PyRun_SimpleString("import requests");

  //influxDB credentials
  PyModule_AddStringConstant(py_main, "NVF", q->NVF);
  PyModule_AddStringConstant(py_main, "IP", q->ip);
  PyModule_AddStringConstant(py_main, "DB", q->name);
  PyModule_AddStringConstant(py_main, "USER", q->user);
  PyModule_AddStringConstant(py_main, "PASSWORD", q->pwd);

  PyModule_AddIntConstant(py_main, "Nid2", (long)q->Nid2);

  PyRun_SimpleString("id = 'id_' + NVF + ' value=%s' % Nid2");
  PyRun_SimpleString("requests.post('http://%s:8086/write?db=%s' % (IP, DB), auth=(USER, PASSWORD), data=id)");

  Py_Finalize();
}

void oocran_monitoring_UE(oocran_monitoring_UE_t *q) {
  PyObject *py_main;
  Py_Initialize();
  py_main = PyImport_AddModule("__main__");
  PyRun_SimpleString("import requests");

  //influxDB credentials
  PyModule_AddStringConstant(py_main, "NVF", q->NVF);
  PyModule_AddStringConstant(py_main, "IP", q->ip);
  PyModule_AddStringConstant(py_main, "DB", q->name);
  PyModule_AddStringConstant(py_main, "USER", q->user);
  PyModule_AddStringConstant(py_main, "PASSWORD", q->pwd);

  PyModule_AddIntConstant(py_main, "pkt_errors", q->pkt_errors);
  PyModule_AddIntConstant(py_main, "pkt_total", q->pkt_total);
  PyModule_AddIntConstant(py_main, "snr", (long)q->SNR);
  PyModule_AddIntConstant(py_main, "iteration", q->iterations);

  PyRun_SimpleString("BLERs = 100*float(float(pkt_errors)/float(pkt_total))");
  PyRun_SimpleString("BLER = 'BLER_' + NVF + ' value=%s' % BLERs");
  PyRun_SimpleString("SNR = 'SNR_' + NVF + ' value=%s' % snr");
  PyRun_SimpleString("iterations = 'iterations_' + NVF + ' value=%s' % iteration");

  PyRun_SimpleString("requests.post('http://%s:8086/write?db=%s' % (IP, DB), auth=(USER, PASSWORD), data=BLER)");
  PyRun_SimpleString("requests.post('http://%s:8086/write?db=%s' % (IP, DB), auth=(USER, PASSWORD), data=SNR)");
  PyRun_SimpleString("requests.post('http://%s:8086/write?db=%s' % (IP, DB), auth=(USER, PASSWORD), data=iterations)");

  Py_Finalize();
}

int oocran_reconfiguration_tc_iterations(void) {
  PyObject *py_main, *py_handler;
  int iterations;
  Py_Initialize();
  py_main = PyImport_AddModule("__main__");

  PyRun_SimpleString("with open('monitor.conf', 'r') as f: txt = f.readlines()");
  PyRun_SimpleString("for line in txt:\n\t if 'iterations' in line:\n\t\t tc_iterations = int(line.split(" ")[-1]) \n\t\t break");
  py_handler = PyObject_GetAttrString(py_main,"tc_iterations");
  iterations = PyInt_AsLong(py_handler);

  Py_Finalize();

  return iterations;
}
