/******************************************************************************
  SPDX-License-Identifier: BSD-3-Clause

  Copyright (c) 2020 Rubicon Communications, LLC (Netgate)
  All rights reserved.
  
  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions are met:
  
   1. Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
  
   2. Redistributions in binary form must reproduce the above copyright 
      notice, this list of conditions and the following disclaimer in the 
      documentation and/or other materials provided with the distribution.
  
   3. Neither the name of Rubicon Communications nor the names of its
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.
  
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/

#ifndef _IGC_API_H_
#define _IGC_API_H_

#include "igc_hw.h"

extern void igc_init_function_pointers_i225(struct igc_hw *hw);

s32 igc_set_obff_timer(struct igc_hw *hw, u32 itr);
s32 igc_set_mac_type(struct igc_hw *hw);
s32 igc_setup_init_funcs(struct igc_hw *hw, bool init_device);
s32 igc_init_mac_params(struct igc_hw *hw);
s32 igc_init_nvm_params(struct igc_hw *hw);
s32 igc_init_phy_params(struct igc_hw *hw);
s32 igc_get_bus_info(struct igc_hw *hw);
void igc_clear_vfta(struct igc_hw *hw);
void igc_write_vfta(struct igc_hw *hw, u32 offset, u32 value);
s32 igc_force_mac_fc(struct igc_hw *hw);
s32 igc_check_for_link(struct igc_hw *hw);
s32 igc_reset_hw(struct igc_hw *hw);
s32 igc_init_hw(struct igc_hw *hw);
s32 igc_setup_link(struct igc_hw *hw);
s32 igc_get_speed_and_duplex(struct igc_hw *hw, u16 *speed, u16 *duplex);
s32 igc_disable_pcie_master(struct igc_hw *hw);
void igc_config_collision_dist(struct igc_hw *hw);
int igc_rar_set(struct igc_hw *hw, u8 *addr, u32 index);
u32 igc_hash_mc_addr(struct igc_hw *hw, u8 *mc_addr);
void igc_update_mc_addr_list(struct igc_hw *hw, u8 *mc_addr_list,
			       u32 mc_addr_count);
s32 igc_setup_led(struct igc_hw *hw);
s32 igc_cleanup_led(struct igc_hw *hw);
s32 igc_check_reset_block(struct igc_hw *hw);
s32 igc_blink_led(struct igc_hw *hw);
s32 igc_led_on(struct igc_hw *hw);
s32 igc_led_off(struct igc_hw *hw);
s32 igc_id_led_init(struct igc_hw *hw);
void igc_reset_adaptive(struct igc_hw *hw);
void igc_update_adaptive(struct igc_hw *hw);
s32 igc_get_cable_length(struct igc_hw *hw);
s32 igc_validate_mdi_setting(struct igc_hw *hw);
s32 igc_read_phy_reg(struct igc_hw *hw, u32 offset, u16 *data);
s32 igc_write_phy_reg(struct igc_hw *hw, u32 offset, u16 data);
s32 igc_get_phy_info(struct igc_hw *hw);
void igc_release_phy(struct igc_hw *hw);
s32 igc_acquire_phy(struct igc_hw *hw);
s32 igc_phy_hw_reset(struct igc_hw *hw);
s32 igc_phy_commit(struct igc_hw *hw);
void igc_power_up_phy(struct igc_hw *hw);
void igc_power_down_phy(struct igc_hw *hw);
s32 igc_read_mac_addr(struct igc_hw *hw);
s32 igc_read_pba_string(struct igc_hw *hw, u8 *pba_num, u32 pba_num_size);
void igc_reload_nvm(struct igc_hw *hw);
s32 igc_update_nvm_checksum(struct igc_hw *hw);
s32 igc_validate_nvm_checksum(struct igc_hw *hw);
s32 igc_read_nvm(struct igc_hw *hw, u16 offset, u16 words, u16 *data);
s32 igc_read_kmrn_reg(struct igc_hw *hw, u32 offset, u16 *data);
s32 igc_write_kmrn_reg(struct igc_hw *hw, u32 offset, u16 data);
s32 igc_write_nvm(struct igc_hw *hw, u16 offset, u16 words, u16 *data);
s32 igc_set_d3_lplu_state(struct igc_hw *hw, bool active);
s32 igc_set_d0_lplu_state(struct igc_hw *hw, bool active);
bool igc_check_mng_mode(struct igc_hw *hw);
bool igc_enable_tx_pkt_filtering(struct igc_hw *hw);
s32 igc_mng_enable_host_if(struct igc_hw *hw);
s32 igc_mng_host_if_write(struct igc_hw *hw, u8 *buffer, u16 length,
			    u16 offset, u8 *sum);
s32 igc_mng_write_cmd_header(struct igc_hw *hw,
			       struct igc_host_mng_command_header *hdr);
s32 igc_mng_write_dhcp_info(struct igc_hw *hw, u8 *buffer, u16 length);



/*
 * TBI_ACCEPT macro definition:
 *
 * This macro requires:
 *      a = a pointer to struct igc_hw
 *      status = the 8 bit status field of the Rx descriptor with EOP set
 *      errors = the 8 bit error field of the Rx descriptor with EOP set
 *      length = the sum of all the length fields of the Rx descriptors that
 *               make up the current frame
 *      last_byte = the last byte of the frame DMAed by the hardware
 *      min_frame_size = the minimum frame length we want to accept.
 *      max_frame_size = the maximum frame length we want to accept.
 *
 * This macro is a conditional that should be used in the interrupt
 * handler's Rx processing routine when RxErrors have been detected.
 *
 * Typical use:
 *  ...
 *  if (TBI_ACCEPT) {
 *      accept_frame = TRUE;
 *      igc_tbi_adjust_stats(adapter, MacAddress);
 *      frame_length--;
 *  } else {
 *      accept_frame = FALSE;
 *  }
 *  ...
 */

/* The carrier extension symbol, as received by the NIC. */
#define CARRIER_EXTENSION   0x0F

#define TBI_ACCEPT(a, status, errors, length, last_byte, \
		   min_frame_size, max_frame_size) \
	(igc_tbi_sbp_enabled_82543(a) && \
	 (((errors) & IGC_RXD_ERR_FRAME_ERR_MASK) == IGC_RXD_ERR_CE) && \
	 ((last_byte) == CARRIER_EXTENSION) && \
	 (((status) & IGC_RXD_STAT_VP) ? \
	  (((length) > ((min_frame_size) - VLAN_TAG_SIZE)) && \
	  ((length) <= ((max_frame_size) + 1))) : \
	  (((length) > (min_frame_size)) && \
	  ((length) <= ((max_frame_size) + VLAN_TAG_SIZE + 1)))))

#define IGC_MAX(a, b) ((a) > (b) ? (a) : (b))
#define IGC_DIVIDE_ROUND_UP(a, b)	(((a) + (b) - 1) / (b)) /* ceil(a/b) */
#endif /* _IGC_API_H_ */
