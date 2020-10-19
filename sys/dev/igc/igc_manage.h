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

#ifndef _IGC_MANAGE_H_
#define _IGC_MANAGE_H_

bool igc_enable_mng_pass_thru(struct igc_hw *hw);
u8 igc_calculate_checksum(u8 *buffer, u32 length);
s32 igc_host_interface_command(struct igc_hw *hw, u8 *buffer, u32 length);

enum igc_mng_mode {
	igc_mng_mode_none = 0,
	igc_mng_mode_asf,
	igc_mng_mode_pt,
	igc_mng_mode_ipmi,
	igc_mng_mode_host_if_only
};

#define IGC_FACTPS_MNGCG			0x20000000

#define IGC_FWSM_MODE_MASK			0xE
#define IGC_FWSM_MODE_SHIFT			1

#define IGC_MNG_DHCP_COOKIE_STATUS_VLAN	0x2

#define IGC_HI_MAX_BLOCK_BYTE_LENGTH		1792 /* Num of bytes in range */
#define IGC_HI_MAX_BLOCK_DWORD_LENGTH		448 /* Num of dwords in range */
#define IGC_HI_COMMAND_TIMEOUT		500 /* Process HI cmd limit */
#define IGC_HICR_EN			0x01  /* Enable bit - RO */
/* Driver sets this bit when done to put command in RAM */
#define IGC_HICR_C			0x02
#define IGC_HICR_SV			0x04  /* Status Validity */
#endif
