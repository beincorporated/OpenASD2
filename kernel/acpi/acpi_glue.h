/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026, ASD Project Contributors
 *
 * acpi_glue.h — uACPI integration and SMP discovery for OpenASD2.
 */

#ifndef ACPI_GLUE_H
#define ACPI_GLUE_H

#include <stdint.h>
#include <stddef.h>

#define MAX_SMP_CPUS 16

typedef struct {
    uint8_t  apic_id;
    uint8_t  enabled;
} smp_cpu_info_t;

typedef struct {
    uint32_t       cpu_count;
    uint64_t       lapic_paddr;
    uint32_t       ioapic_paddr;
    uint32_t       ioapic_gsi_base;
    smp_cpu_info_t cpus[MAX_SMP_CPUS];
} smp_info_t;

extern smp_info_t g_smp_info;

/* Early ACPI init & MADT discovery via uACPI.
 * Returns the number of detected online CPUs. */
uint32_t acpi_smp_init(uint64_t rsdp_phys);

/* Get physical base address of the Local APIC */
uint64_t acpi_get_lapic_base(void);

#endif /* ACPI_GLUE_H */
