/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026. All Rights Reserved.
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/errno.h>

#include <kpm_utils.h>

KPM_NAME("vhe_probe");
KPM_VERSION(VHE_PROBE_VERSION);
KPM_LICENSE("GPL v2");
KPM_AUTHOR("local");
KPM_DESCRIPTION("Probe VHE capability and EL2 runtime state.");

/* ID_AA64MMFR1_EL1.VH field bits [11:8] */
#define VHE_SHIFT 8
#define VHE_MASK 0xf

static inline u8 vhe_probe_current_el(void)
{
    u64 current_el;

    asm volatile("mrs %0, CurrentEL" : "=r"(current_el));
    return (current_el >> 2) & 0x3;
}

static bool is_vhe_enabled(void)
{
    u64 mmfr1;
    int vhe_support;
    u8 el;

    asm volatile("mrs %0, id_aa64mmfr1_el1" : "=r"(mmfr1));
    vhe_support = (mmfr1 >> VHE_SHIFT) & VHE_MASK;
    el = vhe_probe_current_el();

    pr_info("VHE_PROBE: Hardware VHE Support: %s, Current EL: %u\n",
            vhe_support ? "YES" : "NO", el);

    return el == 2;
}

static u64 read_hcr_el2(void)
{
    u64 hcr = 0;

    if (vhe_probe_current_el() == 2) {
        asm volatile("mrs %0, hcr_el2" : "=r"(hcr));
    }

    return hcr;
}

static long vhe_probe_init(const char *args, const char *event, void *__user reserved)
{
    (void)args;
    (void)event;
    (void)reserved;

    pr_info("VHE_PROBE: KPM Module Loaded.\n");
    pr_info("VHE_PROBE: Kernel Version: %x, Kernel Patch Version: %x\n", kver, kpver);

    if (is_vhe_enabled()) {
        u64 hcr = read_hcr_el2();

        pr_info("VHE_PROBE: Kernel is running in EL2 (VHE Mode active).\n");
        pr_info("VHE_PROBE: Current HCR_EL2 value: 0x%llx\n", (unsigned long long)hcr);
        pr_info("VHE_PROBE: Ready for Stage-2 page table prototype logic.\n");
    } else {
        pr_err("VHE_PROBE: Kernel is in EL1. Standard Page Fault Hooking is required.\n");
    }

    return 0;
}

static long vhe_probe_control0(const char *args, char *__user out_msg, int outlen)
{
    if (args && strncmp(args, "status", 6) != 0) {
        writeOutMsg(out_msg, &outlen, "VHE_PROBE: unsupported ctl arg, use: status");
        return -EINVAL;
    }

    if (is_vhe_enabled()) {
        u64 hcr = read_hcr_el2();
        pr_info("VHE_PROBE: ctl status, HCR_EL2=0x%llx\n", (unsigned long long)hcr);
        writeOutMsg(out_msg, &outlen, "VHE_PROBE: EL2 (VHE active)");
    } else {
        writeOutMsg(out_msg, &outlen, "VHE_PROBE: EL1 (VHE inactive)");
    }

    return 0;
}

static long vhe_probe_exit(void *__user reserved)
{
    (void)reserved;

    pr_info("VHE_PROBE: KPM Module Unloaded.\n");
    return 0;
}

KPM_INIT(vhe_probe_init);
KPM_CTL0(vhe_probe_control0);
KPM_EXIT(vhe_probe_exit);
