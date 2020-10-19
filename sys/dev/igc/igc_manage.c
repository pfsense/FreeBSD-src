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

#include "igc_api.h"
#include "igc_manage.h"

/**
 *  igc_calculate_checksum - Calculate checksum for buffer
 *  @buffer: pointer to EEPROM
 *  @length: size of EEPROM to calculate a checksum for
 *
 *  Calculates the checksum for some buffer on a specified length.  The
 *  checksum calculated is returned.
 **/
u8 igc_calculate_checksum(u8 *buffer, u32 length)
{
	u32 i;
	u8 sum = 0;

	DEBUGFUNC("igc_calculate_checksum");

	if (!buffer)
		return 0;

	for (i = 0; i < length; i++)
		sum += buffer[i];

	return (u8) (0 - sum);
}


/**
 *  igc_enable_mng_pass_thru - Check if management passthrough is needed
 *  @hw: pointer to the HW structure
 *
 *  Verifies the hardware needs to leave interface enabled so that frames can
 *  be directed to and from the management interface.
 **/
bool igc_enable_mng_pass_thru(struct igc_hw *hw)
{
	u32 manc;
	u32 fwsm, factps;

	DEBUGFUNC("igc_enable_mng_pass_thru");

	if (!hw->mac.asf_firmware_present)
		return FALSE;

	manc = IGC_READ_REG(hw, IGC_MANC);

	if (!(manc & IGC_MANC_RCV_TCO_EN))
		return FALSE;

	if (hw->mac.has_fwsm) {
		fwsm = IGC_READ_REG(hw, IGC_FWSM);
		factps = IGC_READ_REG(hw, IGC_FACTPS);

		if (!(factps & IGC_FACTPS_MNGCG) &&
		    ((fwsm & IGC_FWSM_MODE_MASK) ==
		     (igc_mng_mode_pt << IGC_FWSM_MODE_SHIFT)))
			return TRUE;
	} else if ((manc & IGC_MANC_SMBUS_EN) &&
		   !(manc & IGC_MANC_ASF_EN)) {
		return TRUE;
	}

	return FALSE;
}

/**
 *  igc_host_interface_command - Writes buffer to host interface
 *  @hw: pointer to the HW structure
 *  @buffer: contains a command to write
 *  @length: the byte length of the buffer, must be multiple of 4 bytes
 *
 *  Writes a buffer to the Host Interface.  Upon success, returns IGC_SUCCESS
 *  else returns IGC_ERR_HOST_INTERFACE_COMMAND.
 **/
s32 igc_host_interface_command(struct igc_hw *hw, u8 *buffer, u32 length)
{
	u32 hicr, i;

	DEBUGFUNC("igc_host_interface_command");

	if (!(hw->mac.arc_subsystem_valid)) {
		DEBUGOUT("Hardware doesn't support host interface command.\n");
		return IGC_SUCCESS;
	}

	if (!hw->mac.asf_firmware_present) {
		DEBUGOUT("Firmware is not present.\n");
		return IGC_SUCCESS;
	}

	if (length == 0 || length & 0x3 ||
	    length > IGC_HI_MAX_BLOCK_BYTE_LENGTH) {
		DEBUGOUT("Buffer length failure.\n");
		return -IGC_ERR_HOST_INTERFACE_COMMAND;
	}

	/* Check that the host interface is enabled. */
	hicr = IGC_READ_REG(hw, IGC_HICR);
	if (!(hicr & IGC_HICR_EN)) {
		DEBUGOUT("IGC_HOST_EN bit disabled.\n");
		return -IGC_ERR_HOST_INTERFACE_COMMAND;
	}

	/* Calculate length in DWORDs */
	length >>= 2;

	/* The device driver writes the relevant command block
	 * into the ram area.
	 */
	for (i = 0; i < length; i++)
		IGC_WRITE_REG_ARRAY_DWORD(hw, IGC_HOST_IF, i,
					    *((u32 *)buffer + i));

	/* Setting this bit tells the ARC that a new command is pending. */
	IGC_WRITE_REG(hw, IGC_HICR, hicr | IGC_HICR_C);

	for (i = 0; i < IGC_HI_COMMAND_TIMEOUT; i++) {
		hicr = IGC_READ_REG(hw, IGC_HICR);
		if (!(hicr & IGC_HICR_C))
			break;
		msec_delay(1);
	}

	/* Check command successful completion. */
	if (i == IGC_HI_COMMAND_TIMEOUT ||
	    (!(IGC_READ_REG(hw, IGC_HICR) & IGC_HICR_SV))) {
		DEBUGOUT("Command has failed with no status valid.\n");
		return -IGC_ERR_HOST_INTERFACE_COMMAND;
	}

	for (i = 0; i < length; i++)
		*((u32 *)buffer + i) = IGC_READ_REG_ARRAY_DWORD(hw,
								  IGC_HOST_IF,
								  i);

	return IGC_SUCCESS;
}
