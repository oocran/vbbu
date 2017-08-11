
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
} oocran_monitoring_eNB_t;

// UE parameters to monitor
typedef struct SRSLTE_API {
  double SNR;
  double BLER;
  int iterations;
} oocran_monitoring_UE_t;

SRSLTE_API void oocran_monitoring_init(DB_credentials_t *q);

SRSLTE_API void oocran_monitoring_eNB(oocran_monitoring_eNB_t *q);

SRSLTE_API void oocran_monitoring_UE(oocran_monitoring_UE_t *q);

SRSLTE_API int oocran_reconfiguration_tc_iterations(void);

SRSLTE_API void oocran_monitoring_exit(void);
