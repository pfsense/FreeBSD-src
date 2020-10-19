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

#ifndef _IGC_MAC_H_
#define _IGC_MAC_H_

void igc_init_mac_ops_generic(struct igc_hw *hw);
void igc_null_mac_generic(struct igc_hw *hw);
s32  igc_null_ops_generic(struct igc_hw *hw);
s32  igc_null_link_info(struct igc_hw *hw, u16 *s, u16 *d);
bool igc_null_mng_mode(struct igc_hw *hw);
void igc_null_update_mc(struct igc_hw *hw, u8 *h, u32 a);
void igc_null_write_vfta(struct igc_hw *hw, u32 a, u32 b);
int  igc_null_rar_set(struct igc_hw *hw, u8 *h, u32 a);
s32  igc_blink_led_generic(struct igc_hw *hw);
s32  igc_check_for_copper_link_generic(struct igc_hw *hw);
s32  igc_check_for_fiber_link_generic(struct igc_hw *hw);
s32  igc_check_for_serdes_link_generic(struct igc_hw *hw);
s32  igc_cleanup_led_generic(struct igc_hw *hw);
s32  igc_config_fc_after_link_up_generic(struct igc_hw *hw);
s32  igc_disable_pcie_master_generic(struct igc_hw *hw);
s32  igc_force_mac_fc_generic(struct igc_hw *hw);
s32  igc_get_auto_rd_done_generic(struct igc_hw *hw);
s32  igc_get_bus_info_pcie_generic(struct igc_hw *hw);
void igc_set_lan_id_single_port(struct igc_hw *hw);
s32  igc_get_hw_semaphore_generic(struct igc_hw *hw);
s32  igc_get_speed_and_duplex_copper_generic(struct igc_hw *hw, u16 *speed,
					       u16 *duplex);
s32  igc_get_speed_and_duplex_fiber_serdes_generic(struct igc_hw *hw,
						     u16 *speed, u16 *duplex);
s32  igc_id_led_init_generic(struct igc_hw *hw);
s32  igc_led_on_generic(struct igc_hw *hw);
s32  igc_led_off_generic(struct igc_hw *hw);
void igc_update_mc_addr_list_generic(struct igc_hw *hw,
				       u8 *mc_addr_list, u32 mc_addr_count);
int igc_rar_set_generic(struct igc_hw *hw, u8 *addr, u32 index);
s32  igc_set_fc_watermarks_generic(struct igc_hw *hw);
s32  igc_setup_fiber_serdes_link_generic(struct igc_hw *hw);
s32  igc_setup_led_generic(struct igc_hw *hw);
s32  igc_setup_link_generic(struct igc_hw *hw);
s32  igc_validate_mdi_setting_crossover_generic(struct igc_hw *hw);

u32  igc_hash_mc_addr_generic(struct igc_hw *hw, u8 *mc_addr);

void igc_clear_hw_cntrs_base_generic(struct igc_hw *hw);
void igc_clear_vfta_generic(struct igc_hw *hw);
void igc_init_rx_addrs_generic(struct igc_hw *hw, u16 rar_count);
void igc_pcix_mmrbc_workaround_generic(struct igc_hw *hw);
void igc_put_hw_semaphore_generic(struct igc_hw *hw);
s32  igc_check_alt_mac_addr_generic(struct igc_hw *hw);
void igc_reset_adaptive_generic(struct igc_hw *hw);
void igc_set_pcie_no_snoop_generic(struct igc_hw *hw, u32 no_snoop);
void igc_update_adaptive_generic(struct igc_hw *hw);
void igc_write_vfta_generic(struct igc_hw *hw, u32 offset, u32 value);

#endif
