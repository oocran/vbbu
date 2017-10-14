
/**********************************************************************************************
 *  File:         monitoring.h
 *
 *  Description:  OOCRAN monitoring and reconfiguration functions
 *
 *  Reference:    
 *********************************************************************************************/


#include "srslte/config.h"


// Database info and credentials
typedef struct SRSLTE_API {
  char *name;
  char *ip;
  char *NVF;
  char *user;
  char *pwd;
} DB_credentials_t;

// eNB parameters to monitor
typedef struct SRSLTE_API {
  int RB_assigned;
  int MCS;
  int throughput;
} oocran_monitoring_eNB_t;

// UE parameters to monitor
typedef struct SRSLTE_API {
  double SNR;
  double BLER;
  int iterations;
  double percentCPU;
} oocran_monitoring_UE_t;

SRSLTE_API void oocran_monitoring_init(DB_credentials_t *q);

SRSLTE_API void oocran_monitoring_compute(void);

SRSLTE_API int oocran_parse_tx(void);

SRSLTE_API int oocran_parse_rx(void);

SRSLTE_API void oocran_monitoring_eNB(oocran_monitoring_eNB_t *q);

SRSLTE_API void oocran_monitoring_UE(oocran_monitoring_UE_t *q);

SRSLTE_API int oocran_reconfiguration_eNB(void);

SRSLTE_API int oocran_reconfiguration_UE(void);

SRSLTE_API void oocran_monitoring_exit(void);
