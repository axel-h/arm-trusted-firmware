/*
 * Copyright (c) 2017-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <debug.h>
#include <platform_def.h>
#include <string.h>

/*
 * Placeholder variables for maintaining information about the next image(s)
 */
static entry_point_info_t bl32_image_ep_info;
static entry_point_info_t bl33_image_ep_info;

/*******************************************************************************
 * Gets SPSR for BL33 entry
 ******************************************************************************/
static uint32_t k3_get_spsr_for_bl33_entry(void)
{
	unsigned long el_status;
	unsigned int mode;
	uint32_t spsr;

	/* Figure out what mode we enter the non-secure world in */
	el_status = read_id_aa64pfr0_el1() >> ID_AA64PFR0_EL2_SHIFT;
	el_status &= ID_AA64PFR0_ELX_MASK;

	mode = (el_status) ? MODE_EL2 : MODE_EL1;

	spsr = SPSR_64(mode, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
	return spsr;
}

/*******************************************************************************
 * Perform any BL3-1 early platform setup, such as console init and deciding on
 * memory layout.
 ******************************************************************************/
void bl31_early_platform_setup(bl31_params_t *from_bl2,
			       void *plat_params_from_bl2)
{
	/* There are no parameters from BL2 if BL31 is a reset vector */
	assert(from_bl2 == NULL);
	assert(plat_params_from_bl2 == NULL);

#ifdef BL32_BASE
	/* Populate entry point information for BL32 */
	SET_PARAM_HEAD(&bl32_image_ep_info, PARAM_EP, VERSION_1, 0);
	bl32_image_ep_info.pc = BL32_BASE;
	bl32_image_ep_info.spsr = SPSR_64(MODE_EL1, MODE_SP_ELX,
					  DISABLE_ALL_EXCEPTIONS);
	SET_SECURITY_STATE(bl32_image_ep_info.h.attr, SECURE);
#endif

	/* Populate entry point information for BL33 */
	SET_PARAM_HEAD(&bl33_image_ep_info, PARAM_EP, VERSION_1, 0);
	bl33_image_ep_info.pc = PRELOADED_BL33_BASE;
	bl33_image_ep_info.spsr = k3_get_spsr_for_bl33_entry();
	SET_SECURITY_STATE(bl33_image_ep_info.h.attr, NON_SECURE);

#ifdef K3_HW_CONFIG_BASE
	/*
	 * According to the file ``Documentation/arm64/booting.txt`` of the
	 * Linux kernel tree, Linux expects the physical address of the device
	 * tree blob (DTB) in x0, while x1-x3 are reserved for future use and
	 * must be 0.
	 */
	bl33_image_ep_info.args.arg0 = (u_register_t)K3_HW_CONFIG_BASE;
	bl33_image_ep_info.args.arg1 = 0U;
	bl33_image_ep_info.args.arg2 = 0U;
	bl33_image_ep_info.args.arg3 = 0U;
#endif
}

void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1,
				u_register_t arg2, u_register_t arg3)
{
	bl31_early_platform_setup((void *)arg0, (void *)arg1);
}

void bl31_plat_arch_setup(void)
{
	/* TODO: Initialize the MMU tables */
}

void bl31_platform_setup(void)
{
	/* TODO: Initialize the GIC CPU and distributor interfaces */
}

void platform_mem_init(void)
{
	/* Do nothing for now... */
}

/*
 * Empty function to prevent the console from being uninitialized after BL33 is
 * started and allow us to see messages from BL31.
 */
void bl31_plat_runtime_setup(void)
{
}

/*******************************************************************************
 * Return a pointer to the 'entry_point_info' structure of the next image
 * for the security state specified. BL3-3 corresponds to the non-secure
 * image type while BL3-2 corresponds to the secure image type. A NULL
 * pointer is returned if the image does not exist.
 ******************************************************************************/
entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	entry_point_info_t *next_image_info;

	assert(sec_state_is_valid(type));
	next_image_info = (type == NON_SECURE) ? &bl33_image_ep_info :
						 &bl32_image_ep_info;
	/*
	 * None of the images on the ARM development platforms can have 0x0
	 * as the entrypoint
	 */
	if (next_image_info->pc)
		return next_image_info;

	NOTICE("Requested nonexistent image\n");
	return NULL;
}
