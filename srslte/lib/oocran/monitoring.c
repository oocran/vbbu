#include <python2.7/Python.h>
#include "srslte/oocran/monitoring.h"

PyObject *py_main;
PyThreadState *pMainThreadState;

// initialize Python interpreter and add DB credentials
void oocran_monitoring_init(DB_credentials_t *q) {
  Py_Initialize();
  py_main = PyImport_AddModule("__main__");
  PyRun_SimpleString("import requests");

  PyModule_AddStringConstant(py_main, "NVF", q->NVF);
  PyModule_AddStringConstant(py_main, "IP", q->ip);
  PyModule_AddStringConstant(py_main, "DB", q->name);
  PyModule_AddStringConstant(py_main, "USER", q->user);
  PyModule_AddStringConstant(py_main, "PASSWORD", q->pwd);

  PyEval_InitThreads();
  pMainThreadState = PyEval_SaveThread();
}

// store eNB parameters in DB
void oocran_monitoring_eNB(oocran_monitoring_eNB_t *q) {
  PyEval_AcquireThread(pMainThreadState);

  PyModule_AddIntConstant(py_main, "RBA", (long)q->RB_assigned);
  PyModule_AddIntConstant(py_main, "MCS", (long)q->MCS);
  PyModule_AddIntConstant(py_main, "Throughput", (long)q->throughput);

  PyRun_SimpleString("rba = 'rba_' + NVF + ' value=%s' % RBA");
  PyRun_SimpleString("mcs = 'mcs_' + NVF + ' value=%s' % MCS");
  PyRun_SimpleString("throughput = 'throughput_' + NVF + ' value=%s' % Throughput");

  PyRun_SimpleString("requests.post('http://%s:8086/write?db=%s' % (IP, DB), auth=(USER, PASSWORD), data=rba)");
  PyRun_SimpleString("requests.post('http://%s:8086/write?db=%s' % (IP, DB), auth=(USER, PASSWORD), data=mcs)");
  PyRun_SimpleString("requests.post('http://%s:8086/write?db=%s' % (IP, DB), auth=(USER, PASSWORD), data=throughput)");

  pMainThreadState = PyEval_SaveThread();
}

// store UE parameters in DB
void oocran_monitoring_UE(oocran_monitoring_UE_t *q) {
  PyObject *py_BLER, *py_SNR;

  PyEval_AcquireThread(pMainThreadState);

  py_BLER = Py_BuildValue("d", q->BLER);
  py_SNR = Py_BuildValue("d", q->SNR);
  PyModule_AddObject(py_main, "BLERs", py_BLER);
  PyModule_AddObject(py_main, "snr", py_SNR);
  PyModule_AddIntConstant(py_main, "iteration", q->iterations);

  PyRun_SimpleString("BLER = 'BLER_' + NVF + ' value=%s' % BLERs");
  PyRun_SimpleString("SNR = 'SNR_' + NVF + ' value=%s' % round(snr, 1)");
  PyRun_SimpleString("iterations = 'iterations_' + NVF + ' value=%s' % iteration");

  PyRun_SimpleString("requests.post('http://%s:8086/write?db=%s' % (IP, DB), auth=(USER, PASSWORD), data=BLER)");
  PyRun_SimpleString("requests.post('http://%s:8086/write?db=%s' % (IP, DB), auth=(USER, PASSWORD), data=SNR)");
  PyRun_SimpleString("requests.post('http://%s:8086/write?db=%s' % (IP, DB), auth=(USER, PASSWORD), data=iterations)");

  pMainThreadState = PyEval_SaveThread();
}

// check 'monitor.conf' file
int oocran_reconfiguration_eNB(void) {
  PyObject *py_handler;
  int MCS;

  PyEval_AcquireThread(pMainThreadState);

  if( access("monitor.conf", R_OK) != -1 ) {
	  PyRun_SimpleString("with open('monitor.conf', 'r') as f: txt = f.readlines()");
	  PyRun_SimpleString("for line in txt:\n\t if 'mcs' in line:\n\t\t mcs = int(line.split(" ")[-1]) \n\t\t break");

	  py_handler = PyObject_GetAttrString(py_main,"mcs");
	  MCS = PyInt_AsLong(py_handler);
  } else {
	  MCS = 1;
  }

  pMainThreadState = PyEval_SaveThread();

  return MCS;
}

// check 'monitor.conf' file
int oocran_reconfiguration_UE(void) {
  PyObject *py_handler;
  int iterations;

  PyEval_AcquireThread(pMainThreadState);

  if( access("monitor.conf", R_OK) != -1 ) {
	  PyRun_SimpleString("with open('monitor.conf', 'r') as f: txt = f.readlines()");
	  PyRun_SimpleString("for line in txt:\n\t if 'iterations' in line:\n\t\t tc_iterations = int(line.split(" ")[-1]) \n\t\t break");

	  py_handler = PyObject_GetAttrString(py_main,"tc_iterations");
	  iterations = PyInt_AsLong(py_handler);
  } else {
	  iterations = 4;
  }

  pMainThreadState = PyEval_SaveThread();

  return iterations;
}

// close Python interpreter
void oocran_monitoring_exit(void) {
	// clean up
	PyEval_AcquireThread(pMainThreadState);
    Py_Finalize();
}
