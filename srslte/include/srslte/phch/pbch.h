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

/******************************************************************************
 *  File:         pbch.h
 *
 *  Description:  Physical broadcast channel. If cell.nof_ports = 0, the number 
 *                of ports is blindly determined using the CRC of the received 
 *                codeword for 1, 2 and 4 ports 
 *
 *  Reference:    3GPP TS 36.211 version 10.0.0 Release 10 Sec. 6.6
 *****************************************************************************/

#ifndef PBCH_
#define PBCH_

#include "srslte/config.h"
#include "srslte/common/phy_common.h"
#include "srslte/mimo/precoding.h"
#include "srslte/mimo/layermap.h"
#include "srslte/modem/mod.h"
#include "srslte/modem/demod_soft.h"
#include "srslte/scrambling/scrambling.h"
#include "srslte/fec/rm_conv.h"
#include "srslte/fec/convcoder.h"
#include "srslte/fec/viterbi.h"
#include "srslte/fec/crc.h"

#define SRSLTE_BCH_PAYLOAD_LEN     24
#define SRSLTE_BCH_PAYLOADCRC_LEN  (SRSLTE_BCH_PAYLOAD_LEN+16)
#define SRSLTE_BCH_ENCODED_LEN     3*(SRSLTE_BCH_PAYLOADCRC_LEN)

/* PBCH object */
typedef struct SRSLTE_API {
  srslte_cell_t cell;
  
  uint32_t nof_symbols;

  /* buffers */
  cf_t *ce[SRSLTE_MAX_PORTS];
  cf_t *symbols[SRSLTE_MAX_PORTS];
  cf_t *x[SRSLTE_MAX_PORTS];
  cf_t *d;
  float *llr;
  float *temp;
  float rm_f[SRSLTE_BCH_ENCODED_LEN];
  uint8_t *rm_b;
  uint8_t data[SRSLTE_BCH_PAYLOADCRC_LEN];
  uint8_t data_enc[SRSLTE_BCH_ENCODED_LEN];

  uint32_t frame_idx;

  /* tx & rx objects */
  srslte_modem_table_t mod;
  srslte_sequence_t seq;
  srslte_viterbi_t decoder;
  srslte_crc_t crc;
  srslte_convcoder_t encoder;
  bool search_all_ports;
  
} srslte_pbch_t;

SRSLTE_API int srslte_pbch_init(srslte_pbch_t *q,
                         srslte_cell_t cell);

SRSLTE_API void srslte_pbch_free(srslte_pbch_t *q);

SRSLTE_API int srslte_pbch_decode(srslte_pbch_t *q, 
                           cf_t *slot1_symbols, 
                           cf_t *ce_slot1[SRSLTE_MAX_PORTS], 
                           float noise_estimate, 
                           uint8_t bch_payload[SRSLTE_BCH_PAYLOAD_LEN], 
                           uint32_t *nof_tx_ports,
                           int *sfn_offset);

SRSLTE_API int srslte_pbch_encode(srslte_pbch_t *q, 
                           uint8_t bch_payload[SRSLTE_BCH_PAYLOAD_LEN], 
                           cf_t *slot1_symbols[SRSLTE_MAX_PORTS], 
                           uint32_t frame_idx);

SRSLTE_API void srslte_pbch_decode_reset(srslte_pbch_t *q);

SRSLTE_API void srslte_pbch_mib_unpack(uint8_t *msg, 
                                srslte_cell_t *cell, 
                                uint32_t *sfn);

SRSLTE_API void srslte_pbch_mib_pack(srslte_cell_t *cell, 
                              uint32_t sfn, 
                              uint8_t *msg);

#endif // PBCH_
