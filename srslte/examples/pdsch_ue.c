/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2015 Software Radio Systems Limited
 *
 * \section LICENSE
 *
 * This file is part of the srsLTE library.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <python2.7/Python.h>

#include "srslte/srslte.h"

#define ENABLE_AGC_DEFAULT

#ifndef DISABLE_RF
#include "srslte/rf/rf.h"
#include "srslte/rf/rf_utils.h"

cell_search_cfg_t cell_detect_config = {
  SRSLTE_DEFAULT_MAX_FRAMES_PBCH,
  SRSLTE_DEFAULT_MAX_FRAMES_PSS, 
  SRSLTE_DEFAULT_NOF_VALID_PSS_FRAMES,
  0
};

#else
#warning Compiling pdsch_ue with no RF support
#endif

oocran_monitoring_UE_t monitor = {
  30.0,				//SNR
  0.0,				//BLER
  1,					//turbo code iterations (SRSLTE_PDSCH_MAX_TDEC_ITERS)
  0.0,        //percentCPU
  0           //throughput_UE
};

DB_credentials_t credentials = {
  "oocran",			//name
  "localhost",		//ip
  "OOCRAN",			//NVF
  "admin",			//user
  "oocran"			//password
};

//#define STDOUT_COMPACT

#ifndef DISABLE_GRAPHICS
#include "srsgui/srsgui.h"
void init_plots();
pthread_t plot_thread; 
sem_t plot_sem; 
uint32_t plot_sf_idx=0;
bool plot_track = true; 
#endif

#define PRINT_CHANGE_SCHEDULIGN

//#define CORRECT_SAMPLE_OFFSET

/**********************************************************************
 *  Program arguments processing
 ***********************************************************************/
typedef struct {
  int nof_subframes;
  bool disable_plots;
  bool disable_plots_except_constellation;
  bool disable_cfo; 
  uint32_t time_offset; 
  int force_N_id_2;
  uint16_t rnti;
  char *input_file_name;
  char *log_file_name;
  int file_offset_time; 
  float file_offset_freq;
  uint32_t file_nof_prb;
  uint32_t file_nof_ports;
  uint32_t file_cell_id;
  char *rf_args; 
  double rf_freq; 
  float rf_gain;
  int net_port; 
  char *net_address; 
  int net_port_signal; 
  char *net_address_signal;
  bool influx_DB;
  char *DB_name;
  char *DB_ip;
  char *DB_NVF;
  char *DB_user;
  char *DB_pwd;
  bool conf_parser;
}prog_args_t;

void args_default(prog_args_t *args) {
  args->disable_plots = false; 
  args->disable_plots_except_constellation = false; 
  args->nof_subframes = -1;
  args->rnti = SRSLTE_SIRNTI;
  args->force_N_id_2 = -1; // Pick the best
  args->input_file_name = NULL;
  args->log_file_name = NULL;
  args->disable_cfo = false; 
  args->time_offset = 0; 
  args->file_nof_prb = 25; 
  args->file_nof_ports = 1; 
  args->file_cell_id = 0; 
  args->file_offset_time = 0; 
  args->file_offset_freq = 0; 
  args->rf_args = "";
  args->rf_freq = -1.0;
#ifdef ENABLE_AGC_DEFAULT
  args->rf_gain = -1.0; 
#else
  args->rf_gain = 50.0; 
#endif
  args->net_port = -1; 
  args->net_address = "127.0.0.1";
  args->net_port_signal = -1; 
  args->net_address_signal = "127.0.0.1";
  args->influx_DB = false;
  args->conf_parser = false;
  args->DB_name = credentials.name;
  args->DB_ip = credentials.ip;
  args->DB_NVF = credentials.NVF;
  args->DB_user = credentials.user;
  args->DB_pwd = credentials.pwd;
}

FILE *stream;

void usage(prog_args_t *args, char *prog) {
  printf("Usage: %s [agpPoOcildDnruvFEBNIRW] -f rx_frequency (in Hz) | -i input_file\n", prog);
#ifndef DISABLE_RF
  printf("\t-a RF args [Default %s]\n", args->rf_args);
#ifdef ENABLE_AGC_DEFAULT
  printf("\t-g RF fix RX gain [Default AGC]\n");
#else
  printf("\t-g Set RX gain [Default %.1f dB]\n", args->rf_gain);
#endif  
#else
  printf("\t   RF is disabled.\n");
#endif
  printf("\t-i input_file [Default use RF board]\n");
  printf("\t-o offset frequency correction (in Hz) for input file [Default %.1f Hz]\n", args->file_offset_freq);
  printf("\t-O offset samples for input file [Default %d]\n", args->file_offset_time);
  printf("\t-p nof_prb for input file [Default %d]\n", args->file_nof_prb);
  printf("\t-P nof_ports for input file [Default %d]\n", args->file_nof_ports);
  printf("\t-c cell_id for input file [Default %d]\n", args->file_cell_id);
  printf("\t-r RNTI in Hex [Default 0x%x]\n",args->rnti);
  printf("\t-l Force N_id_2 [Default best]\n");
  printf("\t-C Disable CFO correction [Default %s]\n", args->disable_cfo?"Disabled":"Enabled");
  printf("\t-t Add time offset [Default %d]\n", args->time_offset);
#ifndef DISABLE_GRAPHICS
  printf("\t-d disable plots [Default enabled]\n");
  printf("\t-D disable all but constellation plots [Default enabled]\n");
#else
  printf("\t plots are disabled. Graphics library not available\n");
#endif
  printf("\t-n nof_subframes [Default %d]\n", args->nof_subframes);
  printf("\t-s remote UDP port to send input signal (-1 does nothing with it) [Default %d]\n", args->net_port_signal);
  printf("\t-S remote UDP address to send input signal [Default %s]\n", args->net_address_signal);
  printf("\t-u remote TCP port to send data (-1 does nothing with it) [Default %d]\n", args->net_port);
  printf("\t-U remote TCP address to send data [Default %s]\n", args->net_address);
  printf("\t-v [set srslte_verbose to debug, default none]\n");
  printf("\t-i wrtie to log file [Default 'stdout']\n");
  printf("\t===============================\n");
  printf("\tinfluxDB settings:\n");
  printf("\t-E Enable monitoring module [Default %s]\n", args->influx_DB?"Disabled":"Enabled");
  printf("\t-B Database [Default %s]\n", args->DB_name);
  printf("\t-N NVF [Default %s]\n", args->DB_NVF);
  printf("\t-I IP address [Default %s]\n", args->DB_ip);
  printf("\t-R User [Default %s]\n", args->DB_user);
  printf("\t-W Password [Default %s]\n", args->DB_pwd);  
  printf("\t===============================\n");
  printf("\tReconfiguration settings:\n");
  printf("\t-A Parse parameters from /etc/bbu/bbu.conf [Default %s]\n", args->conf_parser?"Disabled":"Enabled");
}

void parse_args(prog_args_t *args, int argc, char **argv) {
  int opt;
  args_default(args);
  while ((opt = getopt(argc, argv, "aogliIpPcOCtdDnNvrRfuUsSEBWAF")) != -1) {
    switch (opt) {
    case 'i':
      args->input_file_name = argv[optind];
      break;
    case 'p':
      args->file_nof_prb = atoi(argv[optind]);
      break;
    case 'P':
      args->file_nof_ports = atoi(argv[optind]);
      break;
    case 'o':
      args->file_offset_freq = atof(argv[optind]);
      break;
    case 'O':
      args->file_offset_time = atoi(argv[optind]);
      break;
    case 'c':
      args->file_cell_id = atoi(argv[optind]);
      break;
    case 'a':
      args->rf_args = argv[optind];
      break;
    case 'g':
      args->rf_gain = atof(argv[optind]);
      break;
    case 'C':
      args->disable_cfo = true;
      break;
    case 't':
      args->time_offset = atoi(argv[optind]);
      break;
    case 'f':
      args->rf_freq = strtod(argv[optind], NULL);
      break;
    case 'n':
      args->nof_subframes = atoi(argv[optind]);
      break;
    case 'r':
      args->rnti = strtol(argv[optind], NULL, 16);
      break;
    case 'l':
      args->force_N_id_2 = atoi(argv[optind]);
      break;
    case 'u':
      args->net_port = atoi(argv[optind]);
      break;
    case 'U':
      args->net_address = argv[optind];
      break;
    case 's':
      args->net_port_signal = atoi(argv[optind]);
      break;
    case 'S':
      args->net_address_signal = argv[optind];
      break;
    case 'd':
      args->disable_plots = true;
      break;
    case 'D':
      args->disable_plots_except_constellation = true;
      break;
    case 'F':
      args->log_file_name = argv[optind];
      break;
    case 'E':
      args->influx_DB = true;
      break;
    case 'B':
      args->DB_name = argv[optind];
      credentials.name = args->DB_name;
      args->influx_DB = true;
      break;
    case 'N':
      args->DB_NVF = argv[optind];
      credentials.NVF = args->DB_NVF;
      args->influx_DB = true;
      break;
    case 'I':
      args->DB_ip = argv[optind];
      credentials.ip = args->DB_ip;
      args->influx_DB = true;
      break;
    case 'R':
      args->DB_user = argv[optind];
      credentials.user = args->DB_user;
      args->influx_DB = true;
      break;
    case 'W':
      args->DB_pwd = argv[optind];
      credentials.pwd = args->DB_pwd;
      args->influx_DB = true;
      break;
    case 'A':
      args->conf_parser = true;
      args->influx_DB = true;
      break;
    case 'v':
      srslte_verbose++;
      break;
    default:
      usage(args, argv[0]);
      exit(-1);
    }
  }
  if (args->rf_freq < 0 && args->input_file_name == NULL) {
    usage(args, argv[0]);
    exit(-1);
  }
}

////////////////////////////////////////////////////////////////////////

int execMOPS(long int numMOPs){
  long int j;
  float xf;
  numMOPs = numMOPs/4;
//  printf("numOPs=%ld\n", numMOPs);
  xf=1.0;
  for (j = 0; j < numMOPs; j++) {
    xf =33.0 * xf;
    xf = xf + 33.0;
    xf = xf / 33.0;
    xf = xf - 46.0;
  }
  return(xf);
}

void measureMOPS_0(long int num_operations, int *numMOPS, long int *numNS){

  float outf=0.0;
  struct timespec current, current1, current2;
  long int time1_ns, time2_ns;
  float x;
  
//  printf(".................measureMACS_0 NumOps=%ld \n", (long int)num_operations);
  clock_gettime(CLOCK_MONOTONIC, &current1);
  time1_ns=((long int)current1.tv_sec)*1000000000 + (long int)current1.tv_nsec;
  x=execMOPS((long int)num_operations);
  clock_gettime(CLOCK_MONOTONIC, &current2);
  time2_ns=((long int)current2.tv_sec)*1000000000 + (long int)current2.tv_nsec;
//  printf("time1=%ld, time2=%ld, diff=%ld\n",time1_ns, time2_ns, time2_ns-time1_ns);

//  current.tv_sec = current2.tv_sec - current1.tv_sec;
//  current.tv_nsec = current2.tv_nsec - current1.tv_nsec;

//  printf("x=%3.2f\n", x);


  *numMOPS=(int)((float)(num_operations)*1000.0/(float)(time2_ns-time1_ns));
  *numNS=(time2_ns-time1_ns);
//  printf("num_operations=%ld, measured MOPS=%3.1f, numNS=%ld\n", num_operations, (float)(*numMOPS), time2_ns-time1_ns);
  printf("/////////////////////////////////////////////%3.2f\n", x);
  printf("Measured MOPS=%3.1f\n", (float)(*numMOPS));
  printf("/////////////////////////////////////////////%3.2f\n", x);
  //Return out value only for wait till result available purposes
}



float averbuffer0[1024*16];
float averpercentCPU=0;
float windowAverage(float inputdata, int windowlength, float *buffer){

  int i;
  static int first=0;
  float average=0.0;
  static int counter=0;
  static float mbuffer[1024];

  if(first==0){
    for(i=0; i<windowlength; i++){
      *(mbuffer+i)=0;
    }
    first=1;
  }else{
    for(i=windowlength-2; i>=0; i--){

//      printf("Abuffer[%d]=%3.3f, buffer[%d]=%3.3f\n", i, mbuffer[i], i+1, mbuffer[i+1]);
      *(mbuffer+i+1)=*(mbuffer+i);
//      printf("Bbuffer[%d]=%3.3f, buffer[%d]=%3.3f\n", i, mbuffer[i], i+1, mbuffer[i+1]);

    }
//    printf("buffer[0]=%3.3f, buffer[1]=%3.3f, buffer[2]=%3.3f, buffer[3]=%3.3f,buffer[4]=%3.3f\n", 
//            mbuffer[0], mbuffer[1],mbuffer[2],mbuffer[3],mbuffer[4]);
  }
  mbuffer[0]=inputdata;
  average=0.0;
  for(i=0; i<windowlength; i++){
      average+=*(mbuffer+i);
  }
  counter++;
  if(counter==50){
    counter=0;
//    printf("average=%3.3f, \n", (average/(float)windowlength)*100);
  }
  return(average/(float)windowlength);
}


////////////////////////////////////////////////////////////////////////
/**********************************************************************/

/* TODO: Do something with the output data */
uint8_t data[20000];

bool go_exit = false; 
void sig_int_handler(int signo)
{
  printf("SIGINT received. Exiting...\n");
  if (signo == SIGINT) {
    go_exit = true;
  }
}

#ifndef DISABLE_RF
int srslte_rf_recv_wrapper(void *h, void *data, uint32_t nsamples, srslte_timestamp_t *t) {
  DEBUG(" ----  Receive %d samples  ---- \n", nsamples);
  return srslte_rf_recv(h, data, nsamples, 1);
}

double srslte_rf_set_rx_gain_th_wrapper_(void *h, double f) {
  return srslte_rf_set_rx_gain_th((srslte_rf_t*) h, f);
}

#endif

extern float mean_exec_time;

enum receiver_state { DECODE_MIB, DECODE_PDSCH} state; 

srslte_ue_dl_t ue_dl; 
srslte_ue_sync_t ue_sync; 
prog_args_t prog_args; 

uint32_t sfn = 0; // system frame number
cf_t *sf_buffer = NULL; 
srslte_netsink_t net_sink, net_sink_signal; 

int main(int argc, char **argv) {
  int ret; 
  srslte_cell_t cell;  
  int64_t sf_cnt;
  srslte_ue_mib_t ue_mib; 
#ifndef DISABLE_RF
  srslte_rf_t rf; 
#endif
  uint32_t nof_trials = 0; 
  int n; 
  uint8_t bch_payload[SRSLTE_BCH_PAYLOAD_LEN];
  int sfn_offset;
  float cfo = 0, BLER = 0.0; 
#define NUNAVER 1024
  float _BLER[NUNAVER];  
  int idxBLER=0;
  //char BLER_s[50];
  //srslte_filesink_t logsink; //log control

  // PyObject *py_main, *py_handler;
  uint32_t tc_iterations = 1;
  short reconf_count = 0;

  uint32_t last_noi;
  long hist_iterations[10];

  for (short j=0; j<10; j++) {
	  hist_iterations[j] = 0;
  }
  //parse arguments
  parse_args(&prog_args, argc, argv);

  // redirect stdout to file
  if (prog_args.log_file_name) {
    if((stream = freopen(prog_args.log_file_name, "w", stdout)) == NULL) {
      fprintf(stderr, "Error opening log file %s\n", prog_args.log_file_name);
      exit(-1);
    }
  }

/////////////////////////////////////////////////////////////////////////////////////////////////////
  //CHECK PROCESSOR's COMPUTING CAPACITY
  struct timespec current1, current2;
  long int time1_ns, time2_ns;
  float percentCPU=0;
  int numMOPS=0;
  long int numNS=0;
  long int numOPs=10000000;
  measureMOPS_0(numOPs, &numMOPS, &numNS);
//  printf("num_operations=%ld, measured MOPS=%3.1f, numNS=%ld\n", numOPs, (float)(numMOPS), numNS);

/////////////////////////////////////////////////////////////////////////////////////////////////////

  if (prog_args.net_port > 0) {
    if (srslte_netsink_init(&net_sink, prog_args.net_address, prog_args.net_port, SRSLTE_NETSINK_TCP)) {
      fprintf(stderr, "Error initiating UDP socket to %s:%d\n", prog_args.net_address, prog_args.net_port);
      exit(-1);
    }
    srslte_netsink_set_nonblocking(&net_sink);
  }
  if (prog_args.net_port_signal > 0) {
    if (srslte_netsink_init(&net_sink_signal, prog_args.net_address_signal, 
      prog_args.net_port_signal, SRSLTE_NETSINK_UDP)) {
      fprintf(stderr, "Error initiating UDP socket to %s:%d\n", prog_args.net_address_signal, prog_args.net_port_signal);
      exit(-1);
    }
    srslte_netsink_set_nonblocking(&net_sink_signal);
  }
  
  /* open log file */
  /*if (srslte_filesink_init(&logsink, "log_ue.txt", SRSLTE_CHAR)) {
	  fprintf(stderr, "Error opening log file %s\n", "log_ue.txt");
	  exit(-1);
  }*/
  //fprintf(logsink.f,"%s","UE log control\n==================\n\n");

#ifndef DISABLE_RF
  if (!prog_args.input_file_name) {
    
    printf("Opening RF device...\n");
    if (srslte_rf_open(&rf, prog_args.rf_args)) {
      fprintf(stderr, "Error opening rf\n");
      exit(-1);
    }
    //fprintf(logsink.f,"%s","RF device opened successfully\n");

    /* Set receiver gain */
    if (prog_args.rf_gain > 0) {
      srslte_rf_set_rx_gain(&rf, prog_args.rf_gain);
      //fprintf(logsink.f,"%s","Rx gain set successfully\n");
    } else {
      printf("Starting AGC thread...\n");
      if (srslte_rf_start_gain_thread(&rf, false)) {
        fprintf(stderr, "Error opening rf\n");
        exit(-1);
      }
      //fprintf(logsink.f,"%s","AGC thread started\n");
      srslte_rf_set_rx_gain(&rf, 50);      
      cell_detect_config.init_agc = 50; 
    }
    
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigprocmask(SIG_UNBLOCK, &sigset, NULL);
    signal(SIGINT, sig_int_handler);
    
    //srslte_rf_set_master_clock_rate(&rf, 30.72e6*3);

    /* set receiver frequency */
    printf("Tunning receiver to %.3f MHz\n", prog_args.rf_freq/1000000);
    srslte_rf_set_rx_freq(&rf, prog_args.rf_freq);
    srslte_rf_rx_wait_lo_locked(&rf);

    //fprintf(logsink.f,"%s","Scanning for cells...\n");
    uint32_t ntrial=0; 
    do {
      ret = rf_search_and_decode_mib(&rf, &cell_detect_config, prog_args.force_N_id_2, &cell, &cfo);
      if (ret < 0) {
        fprintf(stderr, "Error searching for cell\n");
        exit(-1); 
      } else if (ret == 0 && !go_exit) {
        printf("Cell not found after %d trials. Trying again (Press Ctrl+C to exit)\n", ntrial++);
      }      
    } while (ret == 0 && !go_exit); 

    if (go_exit) {
      exit(0);
    }

    printf("\n ret = %d\n", ret);

    //fprintf(logsink.f,"Cell with %d PRBs found!", cell.nof_prb);

/*

    printf("\n nof_prb = %d\n", cell.nof_prb);
    printf("\n nof_ports = %d\n", cell.nof_ports);
    printf("\n bw_idx = %d\n", cell.bw_idx);
    printf("\n id = %d\n", cell.nof_prb);
    printf("\n cp = %d\n", cell.cp);
    printf("\n phich_length = %d\n", cell.phich_length);
    printf("\n phich_resources = %d\n", cell.phich_resources);

    //force default
    cell.id = 0;
    cell.cp = SRSLTE_CP_NORM;
    cell.phich_length = SRSLTE_PHICH_NORM;
    cell.phich_resources = SRSLTE_PHICH_R_1;
    cell.nof_ports = 1;
    cell.nof_prb = 6;
    cell.bw_idx = 0;
*/
    /* set sampling frequency */
    int srate = srslte_sampling_freq_hz(cell.nof_prb);    
    //printf("%d\n", srate);
    if (srate != -1) {  
      /*if (srate < 10e6) {
        srslte_rf_set_master_clock_rate(&rf, 4*srate);        
      } else {
        srslte_rf_set_master_clock_rate(&rf, srate);        
      }*/
      printf("Setting sampling rate %.2f MHz\n", (float) srate/1000000);
      float srate_rf = srslte_rf_set_rx_srate(&rf, (double) srate);
      if (srate_rf != srate) {
        fprintf(stderr, "Could not set sampling rate\n");
        exit(-1);
      }
      //fprintf(logsink.f,"Setting sampling rate %.2f MHz\n", (float) srate/1000000);
    } else {
      fprintf(stderr, "Invalid number of PRB %d\n", cell.nof_prb);
      exit(-1);
    }

    INFO("Stopping RF and flushing buffer...\r",0);
    srslte_rf_stop_rx_stream(&rf);
    srslte_rf_flush_buffer(&rf);    
  }
#endif
  
  /* If reading from file, go straight to PDSCH decoding. Otherwise, decode MIB first */
  if (prog_args.input_file_name) {
    /* preset cell configuration */
    cell.id = prog_args.file_cell_id; 
    cell.cp = SRSLTE_CP_NORM; 
    cell.phich_length = SRSLTE_PHICH_NORM;
    cell.phich_resources = SRSLTE_PHICH_R_1;
    cell.nof_ports = prog_args.file_nof_ports; 
    cell.nof_prb = prog_args.file_nof_prb; 
    
    if (srslte_ue_sync_init_file(&ue_sync, prog_args.file_nof_prb, 
      prog_args.input_file_name, prog_args.file_offset_time, prog_args.file_offset_freq)) {
      fprintf(stderr, "Error initiating ue_sync\n");
      exit(-1); 
    }

  } else {
#ifndef DISABLE_RF
    if (srslte_ue_sync_init(&ue_sync, cell, srslte_rf_recv_wrapper, (void*) &rf)) {
      fprintf(stderr, "Error initiating ue_sync\n");
      exit(-1); 
    }
    //fprintf(logsink.f,"UE sync init successful");
#endif
  }

  if (srslte_ue_mib_init(&ue_mib, cell)) {
    fprintf(stderr, "Error initaiting UE MIB decoder\n");
    exit(-1);
  }
  //fprintf(logsink.f,"MIB decoder init successful");

  if (srslte_ue_dl_init(&ue_dl, cell)) {  // This is the User RNTI
    fprintf(stderr, "Error initiating UE downlink processing module\n");
    exit(-1);
  }
  //fprintf(logsink.f,"DL init successful");
  
  /* Configure downlink receiver for the SI-RNTI since will be the only one we'll use */
  srslte_ue_dl_set_rnti(&ue_dl, prog_args.rnti); 
  //fprintf(logsink.f,"RNTI set successfully");
  
  /* Initialize subframe counter */
  sf_cnt = 0;

#ifndef DISABLE_GRAPHICS
  if (!prog_args.disable_plots) {
    init_plots(cell);    
    sleep(1);
  }
#endif

#ifndef DISABLE_RF
  if (!prog_args.input_file_name) {
    srslte_rf_start_rx_stream(&rf);    
  }
#endif
    
  // Variables for measurements 
  uint32_t nframes=0;
  float rsrp=0.0, rsrq=0.0, noise=0.0;
  bool decode_pdsch = false; 

#ifndef DISABLE_RF
  if (prog_args.rf_gain < 0) {
    srslte_ue_sync_start_agc(&ue_sync, srslte_rf_set_rx_gain_th_wrapper_, cell_detect_config.init_agc);
  }
#endif
#ifdef PRINT_CHANGE_SCHEDULIGN
  srslte_ra_dl_dci_t old_dl_dci; 
  bzero(&old_dl_dci, sizeof(srslte_ra_dl_dci_t));
#endif
  
  ue_sync.correct_cfo = !prog_args.disable_cfo;
  
  // Set initial CFO for ue_sync
  srslte_ue_sync_set_cfo(&ue_sync, cfo); 
  
  srslte_pbch_decode_reset(&ue_mib.pbch);

  if (prog_args.influx_DB) {
	  oocran_monitoring_init(&credentials);
	  oocran_monitoring_compute();
  }

  INFO("\nEntering main loop...\n\n", 0);
  /* Main loop */
  while (!go_exit && (sf_cnt < prog_args.nof_subframes || prog_args.nof_subframes == -1)) {

//////////////////////////////////////////////////////////////////////////////////
//  clock_gettime(CLOCK_MONOTONIC, &current1);
//  time1_ns=((long int)current1.tv_sec)*1000000000 + (long int)current1.tv_nsec;
//////////////////////////////////////////////////////////////////////////////////

	if (prog_args.conf_parser && sfn == 1000) {
		//reconfigure number of turbo code iterations /etc/bbu/bbu.conf
		tc_iterations = oocran_parse_rx();
		//tc_iterations = oocran_reconfiguration_UE();

		if ((tc_iterations != monitor.iterations) && (tc_iterations >= 1) && (tc_iterations <= 10)) {
			printf("\nRECONFIGURATION - The number of turbo code iterations will be changed to: %d \n", tc_iterations);
			srslte_sch_set_max_noi(&ue_dl.pdsch.dl_sch, (uint32_t)tc_iterations);
			monitor.iterations = tc_iterations;
		}
		//printf("\nAverage number of iterations: %f \n", ue_dl.pdsch.dl_sch.average_nof_iterations);
	}

    ret = srslte_ue_sync_get_buffer(&ue_sync, &sf_buffer);
    if (ret < 0) {
      fprintf(stderr, "Error calling srslte_ue_sync_work()\n");
    }

#ifdef CORRECT_SAMPLE_OFFSET
    float sample_offset = (float) srslte_ue_sync_get_last_sample_offset(&ue_sync)+srslte_ue_sync_get_sfo(&ue_sync)/1000; 
    srslte_ue_dl_set_sample_offset(&ue_dl, sample_offset);
#endif
    
    /* srslte_ue_sync_get_buffer returns 1 if successfully read 1 aligned subframe */
    if (ret == 1) {
      switch (state) {
        case DECODE_MIB:
          if (srslte_ue_sync_get_sfidx(&ue_sync) == 0) {
            n = srslte_ue_mib_decode(&ue_mib, sf_buffer, bch_payload, NULL, &sfn_offset);
            if (n < 0) {
              fprintf(stderr, "Error decoding UE MIB\n");
              exit(-1);
            } else if (n == SRSLTE_UE_MIB_FOUND) {
              srslte_pbch_mib_unpack(bch_payload, &cell, &sfn);
              srslte_cell_fprint(stdout, &cell, sfn);
              printf("Decoded MIB. SFN: %d, offset: %d\n", sfn, sfn_offset);
              sfn = (sfn + sfn_offset)%1024; 
              state = DECODE_PDSCH;               
            }
          }
          break;
        case DECODE_PDSCH:

//2///////////////////////////////////////////////////////////////////////////////
  clock_gettime(CLOCK_MONOTONIC, &current1);
  time1_ns=((long int)current1.tv_sec)*1000000000 + (long int)current1.tv_nsec;
//////////////////////////////////////////////////////////////////////////////////


          if (prog_args.rnti != SRSLTE_SIRNTI) {
            decode_pdsch = true;             
          } else {
            /* We are looking for SIB1 Blocks, search only in appropiate places */
            if ((srslte_ue_sync_get_sfidx(&ue_sync) == 5 && (sfn%2)==0)) {
              decode_pdsch = true; 
            } else {
              decode_pdsch = false; 
            }
          }
          if (decode_pdsch) {    

/*
//1///////////////////////////////////////////////////////////////////////////////
  clock_gettime(CLOCK_MONOTONIC, &current1);
  time1_ns=((long int)current1.tv_sec)*1000000000 + (long int)current1.tv_nsec;
//////////////////////////////////////////////////////////////////////////////////
*/

            INFO("Attempting DL decode SFN=%d\n", sfn);
            n = srslte_ue_dl_decode(&ue_dl, 
                                    &sf_buffer[prog_args.time_offset], 
                                    data, 
                                    sfn*10+srslte_ue_sync_get_sfidx(&ue_sync));

/*
//1///////////////////////////////////////////////////////////////////////////////
  clock_gettime(CLOCK_MONOTONIC, &current2);
  time2_ns=((long int)current2.tv_sec)*1000000000 + (long int)current2.tv_nsec;
  percentCPU=((float)(time2_ns-time1_ns)/1000000.0)*100.0;  //Should be executed in 1ms
//  printf("percentCPU=%3.2f, diff_ns=%ld\n", percentCPU, time2_ns-time1_ns);
  time2_ns=0;
  time1_ns=0;
  averpercentCPU=windowAverage(percentCPU, 512, averbuffer0);
//if(sfn==3)exit(0);
//  printf("averpercentCPU=%3.2f\n", averpercentCPU);
//////////////////////////////////////////////////////////////////////////////////
*/

            if (n < 0) {
             // fprintf(stderr, "Error decoding UE DL\n");fflush(stdout);
            } else if (n > 0) {
              last_noi = srslte_sch_last_noi(&ue_dl.pdsch.dl_sch);
              hist_iterations[last_noi-1]++;
              
              /* Send data if socket active */
              if (prog_args.net_port > 0) {
                srslte_netsink_write(&net_sink, data, 1+(n-1)/8);
              }
              
              #ifdef PRINT_CHANGE_SCHEDULIGN
              if (ue_dl.dl_dci.mcs_idx         != old_dl_dci.mcs_idx           || 
                  memcmp(&ue_dl.dl_dci.type0_alloc, &old_dl_dci.type0_alloc, sizeof(srslte_ra_type0_t)) ||
                  memcmp(&ue_dl.dl_dci.type1_alloc, &old_dl_dci.type1_alloc, sizeof(srslte_ra_type1_t)) ||
                  memcmp(&ue_dl.dl_dci.type2_alloc, &old_dl_dci.type2_alloc, sizeof(srslte_ra_type2_t)))
              {
                memcpy(&old_dl_dci, &ue_dl.dl_dci, sizeof(srslte_ra_dl_dci_t));
                fflush(stdout);
                printf("Format: %s\n", srslte_dci_format_string(ue_dl.dci_format));
                srslte_ra_pdsch_fprint(stdout, &old_dl_dci, cell.nof_prb);
                srslte_ra_dl_grant_fprint(stdout, &ue_dl.pdsch_cfg.grant);
              }
              #endif

            } 
                                    
            nof_trials++; 
            
            rsrq = SRSLTE_VEC_EMA(srslte_chest_dl_get_rsrq(&ue_dl.chest), rsrq, 0.1);
            rsrp = SRSLTE_VEC_EMA(srslte_chest_dl_get_rsrp(&ue_dl.chest), rsrp, 0.05);      
            noise = SRSLTE_VEC_EMA(srslte_chest_dl_get_noise_estimate(&ue_dl.chest), noise, 0.05);      
            nframes++;
            if (isnan(rsrq)) {
              rsrq = 0; 
            }
            if (isnan(noise)) {
              noise = 0; 
            }
            if (isnan(rsrp)) {
              rsrp = 0; 
            }        
          }


          // Plot and Printf
          if (srslte_ue_sync_get_sfidx(&ue_sync) == 5) {
            float gain = prog_args.rf_gain; 
            if (gain < 0) {
              gain = 10*log10(srslte_agc_get_gain(&ue_sync.agc)); 
            }
            BLER = (float) ue_dl.pkt_errors/ue_dl.pkts_total;
            _BLER[idxBLER]=BLER;
            idxBLER++;
	    //BLER = (float) 100*ue_dl.pkt_errors/ue_dl.pkts_total;
/*            printf("CFO: %+6.2f kHz, "
                   "SNR: %4.1f dB, "
                   "PDCCH-Miss: %5.2f%%, PDSCH-BLER: %5.2f%%\r",                 
                  srslte_ue_sync_get_cfo(&ue_sync)/1000,
                  10*log10(rsrp/noise), 
                  100*(1-(float) ue_dl.nof_detected/nof_trials), 
                  BLER);     
*/               
          }


//2///////////////////////////////////////////////////////////////////////////////
  clock_gettime(CLOCK_MONOTONIC, &current2);
  time2_ns=((long int)current2.tv_sec)*1000000000 + (long int)current2.tv_nsec;
  percentCPU=((float)(time2_ns-time1_ns)/1000000.0)*100.0;  //Should be executed in 1ms
//  printf("percentCPU=%3.2f, diff_ns=%ld\n", percentCPU, time2_ns-time1_ns);
  time2_ns=0;
  time1_ns=0;
  averpercentCPU=windowAverage(percentCPU, 512, averbuffer0);
//if(sfn==3)exit(0);
//  printf("averpercentCPU=%3.2f\n", averpercentCPU);
//////////////////////////////////////////////////////////////////////////////////


          break;
      }
      if (srslte_ue_sync_get_sfidx(&ue_sync) == 9) {
        sfn++; 
        if (sfn == 1024) {

////////////////////////////
            monitor.BLER = BLER;
            monitor.SNR = 10*log10(rsrp/noise)-3.0;
            monitor.iterations = tc_iterations;
            monitor.percentCPU = averpercentCPU;
            monitor.throughput_UE = 1000*ue_dl.pdsch_cfg.grant.mcs.tbs;

//float srslte_pdsch_average_noi(srslte_pdsch_t *q) 
printf("BLER=%5.4f, SNR=%5.2f, iterations=%d, percentCPU=%5.2f%, throughput=%d\n", 
        monitor.BLER, monitor.SNR, monitor.iterations, monitor.percentCPU, monitor.throughput_UE);
printf("BLER=%5.3f, SNR=%5.3f, ue_dl.pdsch.dl_sch.average_nof_iterations=%f\n", monitor.BLER, monitor.SNR, ue_dl.pdsch.dl_sch.average_nof_iterations);

//printf("averpercentCPU=%3.2f\n", averpercentCPU);

///////////////////////////////

        	if (prog_args.influx_DB) {
				//store UE parameters in influxDB
				    oocran_monitoring_UE(&monitor);
				    oocran_monitoring_compute();
        	}

        	sfn = 0;
		    printf("\n");
		    ue_dl.pkt_errors = 0;
		    ue_dl.pkts_total = 0;
		    ue_dl.nof_detected = 0;
		    nof_trials = 0;
		    reconf_count = (reconf_count + 1)%10;
        } 
      }
      
      #ifndef DISABLE_GRAPHICS
      if (!prog_args.disable_plots) {
        if ((sfn%4) == 0 && decode_pdsch) {
          plot_sf_idx = srslte_ue_sync_get_sfidx(&ue_sync);
          plot_track = true;
          sem_post(&plot_sem);
        }
      }
      #endif
    } else if (ret == 0) {
      printf("Finding PSS... Peak: %8.1f, FrameCnt: %d, State: %d\r", 
        srslte_sync_get_peak_value(&ue_sync.sfind), 
        ue_sync.frame_total_cnt, ue_sync.state);      
      #ifndef DISABLE_GRAPHICS
      if (!prog_args.disable_plots) {
        plot_sf_idx = srslte_ue_sync_get_sfidx(&ue_sync);
        plot_track = false; 
        sem_post(&plot_sem);                
      }
      #endif
    }
 
//////////////////////////////////////////////////////////////////////////////////
/*  clock_gettime(CLOCK_MONOTONIC, &current2);
  time2_ns=((long int)current2.tv_sec)*1000000000 + (long int)current2.tv_nsec;
  percentCPU=((float)(time2_ns-time1_ns)/1000000.0)*100.0;  //Should be execute in 1ms
//  printf("percentCPU=%3.2f, diff_ns=%ld\n", percentCPU, time2_ns-time1_ns);
  time2_ns=0;
  time1_ns=0;
  averpercentCPU=windowAverage(percentCPU, 100, averbuffer0);
  printf("averpercentCPU=%3.2f\n", averpercentCPU);*/
//////////////////////////////////////////////////////////////////////////////////


               
    sf_cnt++;                  
  } // Main loop
  
#ifndef DISABLE_GRAPHICS
  if (!prog_args.disable_plots) {
    if (!pthread_kill(plot_thread, 0)) {
      pthread_kill(plot_thread, SIGHUP);
      pthread_join(plot_thread, NULL);    
    }
  }
#endif
  srslte_ue_dl_free(&ue_dl);
  srslte_ue_sync_free(&ue_sync);
  
#ifndef DISABLE_RF
  if (!prog_args.input_file_name) {
    srslte_ue_mib_free(&ue_mib);
    srslte_rf_close(&rf);    
  }
#endif

  if (prog_args.influx_DB) {
    oocran_monitoring_exit();
  }

  /* redirect output to stdout */
  if (prog_args.log_file_name) {
    stream = freopen("CON", "w", stdout);
  }

  for (short j=0; j<10; j++) {
	  printf("\n %d %d \n", j, hist_iterations[j]);
  }

  exit(0);
}






  

/**********************************************************************
 *  Plotting Functions
 ***********************************************************************/
#ifndef DISABLE_GRAPHICS


plot_real_t p_sync, pce;
plot_scatter_t  pscatequal, pscatequal_pdcch;

float tmp_plot[110*15*2048];
float tmp_plot2[110*15*2048];
float tmp_plot3[110*15*2048];

void *plot_thread_run(void *arg) {
  int i;
  uint32_t nof_re = SRSLTE_SF_LEN_RE(ue_dl.cell.nof_prb, ue_dl.cell.cp);
    
  
  sdrgui_init();
  
  plot_scatter_init(&pscatequal);
  plot_scatter_setTitle(&pscatequal, "PDSCH - Equalized Symbols");
  plot_scatter_setXAxisScale(&pscatequal, -4, 4);
  plot_scatter_setYAxisScale(&pscatequal, -4, 4);

  plot_scatter_addToWindowGrid(&pscatequal, (char*)"pdsch_ue", 0, 0);

  if (!prog_args.disable_plots_except_constellation) {
    plot_real_init(&pce);
    plot_real_setTitle(&pce, "Channel Response - Magnitude");
    plot_real_setLabels(&pce, "Index", "dB");
    plot_real_setYAxisScale(&pce, -40, 40);
    
    plot_real_init(&p_sync);
    plot_real_setTitle(&p_sync, "PSS Cross-Corr abs value");
    plot_real_setYAxisScale(&p_sync, 0, 1);

    plot_scatter_init(&pscatequal_pdcch);
    plot_scatter_setTitle(&pscatequal_pdcch, "PDCCH - Equalized Symbols");
    plot_scatter_setXAxisScale(&pscatequal_pdcch, -4, 4);
    plot_scatter_setYAxisScale(&pscatequal_pdcch, -4, 4);

    plot_real_addToWindowGrid(&pce, (char*)"pdsch_ue",    0, 1);
    plot_real_addToWindowGrid(&pscatequal_pdcch, (char*)"pdsch_ue", 1, 0);
    plot_real_addToWindowGrid(&p_sync, (char*)"pdsch_ue", 1, 1);
  }
  
  while(1) {
    sem_wait(&plot_sem);
    
    uint32_t nof_symbols = ue_dl.pdsch_cfg.nbits.nof_re;
    if (!prog_args.disable_plots_except_constellation) {      
      for (i = 0; i < nof_re; i++) {
        tmp_plot[i] = 20 * log10f(cabsf(ue_dl.sf_symbols[i]));
        if (isinf(tmp_plot[i])) {
          tmp_plot[i] = -80;
        }
      }
      int sz = srslte_symbol_sz(ue_dl.cell.nof_prb);
      bzero(tmp_plot2, sizeof(float)*sz);
      int g = (sz - 12*ue_dl.cell.nof_prb)/2;
      for (i = 0; i < 12*ue_dl.cell.nof_prb; i++) {
        tmp_plot2[g+i] = 20 * log10(cabs(ue_dl.ce[0][i]));
        if (isinf(tmp_plot2[g+i])) {
          tmp_plot2[g+i] = -80;
        }
      }
      plot_real_setNewData(&pce, tmp_plot2, sz);        
      
      if (!prog_args.input_file_name) {
        if (plot_track) {
          srslte_pss_synch_t *pss_obj = srslte_sync_get_cur_pss_obj(&ue_sync.strack);
          int max = srslte_vec_max_fi(pss_obj->conv_output_avg, pss_obj->frame_size+pss_obj->fft_size-1);
          srslte_vec_sc_prod_fff(pss_obj->conv_output_avg, 
                          1/pss_obj->conv_output_avg[max], 
                          tmp_plot2, 
                          pss_obj->frame_size+pss_obj->fft_size-1);        
          plot_real_setNewData(&p_sync, tmp_plot2, pss_obj->frame_size);        
        } else {
          int max = srslte_vec_max_fi(ue_sync.sfind.pss.conv_output_avg, ue_sync.sfind.pss.frame_size+ue_sync.sfind.pss.fft_size-1);
          srslte_vec_sc_prod_fff(ue_sync.sfind.pss.conv_output_avg, 
                          1/ue_sync.sfind.pss.conv_output_avg[max], 
                          tmp_plot2, 
                          ue_sync.sfind.pss.frame_size+ue_sync.sfind.pss.fft_size-1);        
          plot_real_setNewData(&p_sync, tmp_plot2, ue_sync.sfind.pss.frame_size);        
        }
        
      }
      
      plot_scatter_setNewData(&pscatequal_pdcch, ue_dl.pdcch.d, 36*ue_dl.pdcch.nof_cce);
    }
    
    plot_scatter_setNewData(&pscatequal, ue_dl.pdsch.d, nof_symbols);
    
    if (plot_sf_idx == 1) {
      if (prog_args.net_port_signal > 0) {
        srslte_netsink_write(&net_sink_signal, &sf_buffer[srslte_ue_sync_sf_len(&ue_sync)/7], 
                            srslte_ue_sync_sf_len(&ue_sync)); 
      }
    }

  }
  
  return NULL;
}

void init_plots() {

  if (sem_init(&plot_sem, 0, 0)) {
    perror("sem_init");
    exit(-1);
  }
  
  pthread_attr_t attr;
  struct sched_param param;
  param.sched_priority = 0;  
  pthread_attr_init(&attr);
  pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
  pthread_attr_setschedparam(&attr, &param);
  if (pthread_create(&plot_thread, NULL, plot_thread_run, NULL)) {
    perror("pthread_create");
    exit(-1);
  }  
}

#endif
