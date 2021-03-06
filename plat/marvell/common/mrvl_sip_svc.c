/*
 * Copyright (C) 2018 Marvell International Ltd.
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 * https://spdx.org/licenses
 */

#include <ap_setup.h>
#include <cache_llc.h>
#include <debug.h>
#include <marvell_plat_priv.h>
#include <plat_marvell.h>
#include <runtime_svc.h>
#include <smcc.h>
#include "comphy/phy-comphy-cp110.h"

/* #define DEBUG_COMPHY */
#ifdef DEBUG_COMPHY
#define debug(format...) NOTICE(format)
#else
#define debug(format, arg...)
#endif

/* Comphy related FID's */
#define MV_SIP_COMPHY_POWER_ON	0x82000001
#define MV_SIP_COMPHY_POWER_OFF	0x82000002
#define MV_SIP_COMPHY_PLL_LOCK	0x82000003
#define MV_SIP_COMPHY_XFI_TRAIN	0x82000004
#define MV_SIP_COMPHY_DIG_RESET	0x82000005

/* Miscellaneous FID's' */
#define MV_SIP_DRAM_SIZE	0x82000010
#define MV_SIP_LLC_ENABLE	0x82000011
#define MV_SIP_PMU_IRQ_ENABLE	0x82000012
#define MV_SIP_PMU_IRQ_DISABLE	0x82000013

#define MAX_LANE_NR		6
#define MVEBU_COMPHY_OFFSET	0x441000
#define MVEBU_SD_OFFSET		0x120000

/* This macro is used to identify COMPHY related calls from SMC function ID */
#define is_comphy_fid(fid)	\
	((fid) >= MV_SIP_COMPHY_POWER_ON && (fid) <= MV_SIP_COMPHY_DIG_RESET)


uintptr_t mrvl_sip_smc_handler(uint32_t smc_fid,
			       u_register_t x1,
			       u_register_t x2,
			       u_register_t x3,
			       u_register_t x4,
			       void *cookie,
			       void *handle,
			       u_register_t flags)
{
	u_register_t ret;
	int i;

	debug("%s: got SMC (0x%x) x1 0x%lx, x2 0x%lx, x3 0x%lx\n",
						 __func__, smc_fid, x1, x2, x3);
	if (is_comphy_fid(smc_fid)) {

		/* some systems passes SD phys address instead of COMPHY phys
		 * address - convert it
		 */
		if (x1 & MVEBU_SD_OFFSET)
			x1 = (x1 & ~0xffffff) + MVEBU_COMPHY_OFFSET;

		if ((x1 & 0xffffff) != MVEBU_COMPHY_OFFSET) {
			ERROR("%s: Wrong smc (0x%x) address: %lx\n",
			      __func__, smc_fid, x1);
			SMC_RET1(handle, SMC_UNK);
		}

		if (x2 >= MAX_LANE_NR) {
			ERROR("%s: Wrong smc (0x%x) lane nr: %lx\n",
			      __func__, smc_fid, x2);
			SMC_RET1(handle, SMC_UNK);
		}
	}

	switch (smc_fid) {

	/* Comphy related FID's */
	case MV_SIP_COMPHY_POWER_ON:
		/* x1:  comphy_base, x2: comphy_index, x3: comphy_mode */
		ret = mvebu_cp110_comphy_power_on(x1, x2, x3);
		SMC_RET1(handle, ret);
	case MV_SIP_COMPHY_POWER_OFF:
		/* x1:  comphy_base, x2: comphy_index */
		ret = mvebu_cp110_comphy_power_off(x1, x2);
		SMC_RET1(handle, ret);
	case MV_SIP_COMPHY_PLL_LOCK:
		/* x1:  comphy_base, x2: comphy_index */
		ret = mvebu_cp110_comphy_is_pll_locked(x1, x2);
		SMC_RET1(handle, ret);
	case MV_SIP_COMPHY_XFI_TRAIN:
		/* x1:  comphy_base, x2: comphy_index */
		ret = mvebu_cp110_comphy_xfi_rx_training(x1, x2);
		SMC_RET1(handle, ret);
	case MV_SIP_COMPHY_DIG_RESET:
		/* x1:  comphy_base, x2: comphy_index, x3: mode, x4: command */
		ret = mvebu_cp110_comphy_digital_reset(x1, x2, x3, x4);
		SMC_RET1(handle, ret);

	/* Miscellaneous FID's' */
	case MV_SIP_DRAM_SIZE:
		/* x1:  ap_base_addr */
		ret = mvebu_get_dram_size(x1);
		SMC_RET1(handle, ret);
	case MV_SIP_LLC_ENABLE:
		for (i = 0; i < ap_get_count(); i++)
			llc_runtime_enable(i);

		SMC_RET1(handle, 0);
#ifdef MVEBU_PMU_IRQ_WA
	case MV_SIP_PMU_IRQ_ENABLE:
		mvebu_pmu_interrupt_enable();
		SMC_RET1(handle, 0);
	case MV_SIP_PMU_IRQ_DISABLE:
		mvebu_pmu_interrupt_disable();
		SMC_RET1(handle, 0);
#endif

	default:
		ERROR("%s: unhandled SMC (0x%x)\n", __func__, smc_fid);
		SMC_RET1(handle, SMC_UNK);
	}
}

/* Define a runtime service descriptor for fast SMC calls */
DECLARE_RT_SVC(
	marvell_sip_svc,
	OEN_SIP_START,
	OEN_SIP_END,
	SMC_TYPE_FAST,
	NULL,
	mrvl_sip_smc_handler
);
