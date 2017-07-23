
/**********************************************************************************************
 *  File:         monitoring.h
 *
 *  Description:  OOCRAN monitoring and reconfiguration functions
 *
 *  Reference:    
 *********************************************************************************************/


#include "srslte/config.h"


typedef struct SRSLTE_API {
  char *name;
  char *ip;
  char *NVF;
  char *user;
  char *pwd;
  int Nid2;
} oocran_monitoring_eNB_t;

typedef struct SRSLTE_API {
  char *name;
  char *ip;
  char *NVF;
  char *user;
  char *pwd;
  double SNR;
  int pkt_errors;
  int pkt_total;
  int iterations;
} oocran_monitoring_UE_t;

SRSLTE_API void oocran_monitoring_eNB(oocran_monitoring_eNB_t *q);

SRSLTE_API void oocran_monitoring_UE(oocran_monitoring_UE_t *q);

SRSLTE_API int oocran_reconfiguration_tc_iterations(void);
