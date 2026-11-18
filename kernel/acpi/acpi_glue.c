/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026, ASD Project Contributors
 *
 * acpi_glue.c — uACPI host implementation and SMP enumeration for OpenASD2.
 */

#include "acpi_glue.h"
#include "../mm/mm.h"
#include <uacpi/uacpi.h>
#include <uacpi/tables.h>
#include <uacpi/acpi.h>
#include <uacpi/kernel_api.h>

extern void serial_puts(const char *s);
extern void serial_putu(uint64_t val);
extern uint64_t asd_time_ns(void);

smp_info_t g_smp_info;
static uint64_t g_rsdp_phys = 0;

static inline uint8_t in8(uint16_t port) {
    uint8_t v;
    __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static inline uint16_t in16(uint16_t port) {
    uint16_t v;
    __asm__ volatile("inw %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static inline uint32_t in32(uint16_t port) {
    uint32_t v;
    __asm__ volatile("inl %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static inline void out8(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void out16(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline void out32(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

/* ------------------------------------------------------------------ */
/* uACPI Kernel API Implementation                                     */
/* ------------------------------------------------------------------ */

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
    if (!out_rsdp_address) return UACPI_STATUS_INVALID_ARGUMENT;
    if (g_rsdp_phys == 0) return UACPI_STATUS_NOT_FOUND;
    *out_rsdp_address = g_rsdp_phys;
    return UACPI_STATUS_OK;
}

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
    (void)len;
    return amm_phys_access((paddr_t)addr);
}

void uacpi_kernel_unmap(void *addr, uacpi_size len) {
    (void)addr;
    (void)len;
}

void uacpi_kernel_log(uacpi_log_level level, const uacpi_char *msg) {
    (void)level;
    if (msg) serial_puts(msg);
}

void *uacpi_kernel_alloc(uacpi_size size) {
    return kmalloc(size);
}

void uacpi_kernel_free(void *mem) {
    if (mem) kfree(mem);
}

uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
    return asd_time_ns();
}

void uacpi_kernel_stall(uacpi_u8 usec) {
    /* Rough busy-wait stall */
    volatile uint64_t count = (uint64_t)usec * 100ULL;
    while (count--) __asm__ volatile("pause");
}

void uacpi_kernel_sleep(uacpi_u64 msec) {
    volatile uint64_t count = msec * 100000ULL;
    while (count--) __asm__ volatile("pause");
}

/* PCI config access via 0xCF8 / 0xCFC */
static uint32_t pci_addr(uacpi_pci_address address, uacpi_size offset) {
    return (1u << 31) |
           ((uint32_t)address.bus << 16) |
           ((uint32_t)address.device << 11) |
           ((uint32_t)address.function << 8) |
           ((uint32_t)offset & 0xFC);
}

uacpi_status uacpi_kernel_pci_device_open(uacpi_pci_address address, uacpi_handle *out_handle) {
    uint32_t addr = (uint32_t)(address.bus << 16 | address.device << 8 | address.function);
    *out_handle = (uacpi_handle)(uintptr_t)(addr + 1);
    return UACPI_STATUS_OK;
}

void uacpi_kernel_pci_device_close(uacpi_handle handle) {
    (void)handle;
}

uacpi_status uacpi_kernel_pci_read8(uacpi_handle device, uacpi_size offset, uacpi_u8 *value) {
    uint32_t dev = (uint32_t)(uintptr_t)device - 1;
    uacpi_pci_address addr = { .segment = 0, .bus = (dev >> 16) & 0xFF, .device = (dev >> 8) & 0x1F, .function = dev & 0x7 };
    out32(0xCF8, pci_addr(addr, offset));
    *value = in8(0xCFC + (offset & 3));
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_read16(uacpi_handle device, uacpi_size offset, uacpi_u16 *value) {
    uint32_t dev = (uint32_t)(uintptr_t)device - 1;
    uacpi_pci_address addr = { .segment = 0, .bus = (dev >> 16) & 0xFF, .device = (dev >> 8) & 0x1F, .function = dev & 0x7 };
    out32(0xCF8, pci_addr(addr, offset));
    *value = in16(0xCFC + (offset & 2));
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_read32(uacpi_handle device, uacpi_size offset, uacpi_u32 *value) {
    uint32_t dev = (uint32_t)(uintptr_t)device - 1;
    uacpi_pci_address addr = { .segment = 0, .bus = (dev >> 16) & 0xFF, .device = (dev >> 8) & 0x1F, .function = dev & 0x7 };
    out32(0xCF8, pci_addr(addr, offset));
    *value = in32(0xCFC);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write8(uacpi_handle device, uacpi_size offset, uacpi_u8 value) {
    uint32_t dev = (uint32_t)(uintptr_t)device - 1;
    uacpi_pci_address addr = { .segment = 0, .bus = (dev >> 16) & 0xFF, .device = (dev >> 8) & 0x1F, .function = dev & 0x7 };
    out32(0xCF8, pci_addr(addr, offset));
    out8(0xCFC + (offset & 3), value);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write16(uacpi_handle device, uacpi_size offset, uacpi_u16 value) {
    uint32_t dev = (uint32_t)(uintptr_t)device - 1;
    uacpi_pci_address addr = { .segment = 0, .bus = (dev >> 16) & 0xFF, .device = (dev >> 8) & 0x1F, .function = dev & 0x7 };
    out32(0xCF8, pci_addr(addr, offset));
    out16(0xCFC + (offset & 2), value);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write32(uacpi_handle device, uacpi_size offset, uacpi_u32 value) {
    uint32_t dev = (uint32_t)(uintptr_t)device - 1;
    uacpi_pci_address addr = { .segment = 0, .bus = (dev >> 16) & 0xFF, .device = (dev >> 8) & 0x1F, .function = dev & 0x7 };
    out32(0xCF8, pci_addr(addr, offset));
    out32(0xCFC, value);
    return UACPI_STATUS_OK;
}

/* SystemIO Port API */
uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle) {
    (void)len;
    *out_handle = (uacpi_handle)(uintptr_t)base;
    return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle handle) {
    (void)handle;
}

uacpi_status uacpi_kernel_io_read8(uacpi_handle handle, uacpi_size offset, uacpi_u8 *out_value) {
    *out_value = in8((uint16_t)((uintptr_t)handle + offset));
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read16(uacpi_handle handle, uacpi_size offset, uacpi_u16 *out_value) {
    *out_value = in16((uint16_t)((uintptr_t)handle + offset));
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read32(uacpi_handle handle, uacpi_size offset, uacpi_u32 *out_value) {
    *out_value = in32((uint16_t)((uintptr_t)handle + offset));
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write8(uacpi_handle handle, uacpi_size offset, uacpi_u8 in_value) {
    out8((uint16_t)((uintptr_t)handle + offset), in_value);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write16(uacpi_handle handle, uacpi_size offset, uacpi_u16 in_value) {
    out16((uint16_t)((uintptr_t)handle + offset), in_value);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write32(uacpi_handle handle, uacpi_size offset, uacpi_u32 in_value) {
    out32((uint16_t)((uintptr_t)handle + offset), in_value);
    return UACPI_STATUS_OK;
}

/* Synchronization Stubs */
typedef struct {
    volatile uint32_t val;
} k_sem_t;

uacpi_handle uacpi_kernel_create_semaphore(uacpi_u32 initial_units) {
    k_sem_t *s = (k_sem_t *)kmalloc(sizeof(k_sem_t));
    if (s) s->val = initial_units;
    return (uacpi_handle)s;
}

void uacpi_kernel_free_semaphore(uacpi_handle h) {
    if (h) kfree(h);
}

uacpi_status uacpi_kernel_wait_for_semaphore(uacpi_handle h, uacpi_u16 timeout) {
    k_sem_t *s = (k_sem_t *)h;
    if (!s) return UACPI_STATUS_INVALID_ARGUMENT;
    if (s->val > 0) {
        s->val--;
        return UACPI_STATUS_OK;
    }
    (void)timeout;
    return UACPI_STATUS_OK;
}

void uacpi_kernel_signal_semaphore(uacpi_handle h) {
    k_sem_t *s = (k_sem_t *)h;
    if (s) s->val++;
}

uacpi_handle uacpi_kernel_create_spinlock(void) {
    uint32_t *l = (uint32_t *)kmalloc(sizeof(uint32_t));
    if (l) *l = 0;
    return (uacpi_handle)l;
}

void uacpi_kernel_free_spinlock(uacpi_handle h) {
    if (h) kfree(h);
}

uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle h) {
    uint32_t *l = (uint32_t *)h;
    uint64_t rflags;
    __asm__ volatile("pushfq; popq %0; cli" : "=r"(rflags));
    if (l) {
        while (__atomic_test_and_set(l, __ATOMIC_ACQUIRE))
            __asm__ volatile("pause");
    }
    return (uacpi_cpu_flags)rflags;
}

void uacpi_kernel_unlock_spinlock(uacpi_handle h, uacpi_cpu_flags flags) {
    uint32_t *l = (uint32_t *)h;
    if (l) __atomic_clear(l, __ATOMIC_RELEASE);
    __asm__ volatile("pushq %0; popfq" : : "r"((uint64_t)flags) : "memory");
}

uacpi_thread_id uacpi_kernel_get_thread_id(void) {
    return (uacpi_thread_id)1;
}

uacpi_interrupt_state uacpi_kernel_disable_interrupts(void) {
    uint64_t rflags;
    __asm__ volatile("pushfq; popq %0; cli" : "=r"(rflags));
    return (uacpi_interrupt_state)rflags;
}

void uacpi_kernel_restore_interrupts(uacpi_interrupt_state state) {
    __asm__ volatile("pushq %0; popfq" : : "r"((uint64_t)state) : "memory");
}

uacpi_status uacpi_kernel_install_interrupt_handler(
    uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx,
    uacpi_handle *out_irq_handle
) {
    (void)irq; (void)handler; (void)ctx;
    *out_irq_handle = (uacpi_handle)1;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(
    uacpi_interrupt_handler handler, uacpi_handle irq_handle
) {
    (void)handler; (void)irq_handle;
    return UACPI_STATUS_OK;
}

uacpi_handle uacpi_kernel_create_work_item(void) {
    return (uacpi_handle)1;
}

void uacpi_kernel_free_work_item(uacpi_handle h) {
    (void)h;
}

void uacpi_kernel_schedule_work(
    uacpi_work_type type, uacpi_handle work_item, uacpi_work_handler handler,
    uacpi_handle ctx
) {
    (void)type; (void)work_item;
    if (handler) handler(ctx);
}

uacpi_status uacpi_kernel_wait_for_work_completion(void) {
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *req) {
    (void)req;
    return UACPI_STATUS_OK;
}

/* ------------------------------------------------------------------ */
/* SMP & MADT Discovery via uACPI                                      */
/* ------------------------------------------------------------------ */

static uint8_t s_early_tbl_buf[8192];

uint64_t acpi_get_lapic_base(void) {
    return g_smp_info.lapic_paddr ? g_smp_info.lapic_paddr : 0xFEE00000ULL;
}

uint32_t acpi_smp_init(uint64_t rsdp_phys) {
    g_rsdp_phys = rsdp_phys;
    g_smp_info.cpu_count = 0;
    g_smp_info.lapic_paddr = 0xFEE00000ULL;
    g_smp_info.ioapic_paddr = 0;

    if (!rsdp_phys) {
        serial_puts("  [ACPI] No RSDP found from UEFI; assuming 1 CPU.\n");
        g_smp_info.cpu_count = 1;
        g_smp_info.cpus[0].apic_id = 0;
        g_smp_info.cpus[0].enabled = 1;
        return 1;
    }

    uacpi_status st = uacpi_setup_early_table_access(s_early_tbl_buf, sizeof(s_early_tbl_buf));
    if (st != UACPI_STATUS_OK) {
        serial_puts("  [uACPI] early table access setup failed; assuming 1 CPU.\n");
        g_smp_info.cpu_count = 1;
        g_smp_info.cpus[0].apic_id = 0;
        g_smp_info.cpus[0].enabled = 1;
        return 1;
    }

    uacpi_table tbl;
    st = uacpi_table_find_by_signature(ACPI_MADT_SIGNATURE, &tbl);
    if (st != UACPI_STATUS_OK) {
        serial_puts("  [uACPI] MADT table not found; assuming 1 CPU.\n");
        g_smp_info.cpu_count = 1;
        g_smp_info.cpus[0].apic_id = 0;
        g_smp_info.cpus[0].enabled = 1;
        return 1;
    }

    struct acpi_madt *madt = (struct acpi_madt *)tbl.ptr;
    if (madt->local_interrupt_controller_address)
        g_smp_info.lapic_paddr = madt->local_interrupt_controller_address;

    uint8_t *cur = (uint8_t *)madt->entries;
    uint8_t *end = (uint8_t *)madt + madt->hdr.length;

    while (cur + sizeof(struct acpi_entry_hdr) <= end) {
        struct acpi_entry_hdr *hdr = (struct acpi_entry_hdr *)cur;
        if (hdr->length == 0 || cur + hdr->length > end) break;

        if (hdr->type == ACPI_MADT_ENTRY_TYPE_LAPIC) {
            struct acpi_madt_lapic *lapic = (struct acpi_madt_lapic *)cur;
            if ((lapic->flags & ACPI_PIC_ENABLED) && g_smp_info.cpu_count < MAX_SMP_CPUS) {
                g_smp_info.cpus[g_smp_info.cpu_count].apic_id = lapic->id;
                g_smp_info.cpus[g_smp_info.cpu_count].enabled = 1;
                g_smp_info.cpu_count++;
            }
        } else if (hdr->type == ACPI_MADT_ENTRY_TYPE_IOAPIC) {
            struct acpi_madt_ioapic *io = (struct acpi_madt_ioapic *)cur;
            g_smp_info.ioapic_paddr = io->address;
            g_smp_info.ioapic_gsi_base = io->gsi_base;
        } else if (hdr->type == ACPI_MADT_ENTRY_TYPE_LAPIC_ADDRESS_OVERRIDE) {
            struct acpi_madt_lapic_address_override *ovr =
                (struct acpi_madt_lapic_address_override *)cur;
            g_smp_info.lapic_paddr = ovr->address;
        }

        cur += hdr->length;
    }

    uacpi_table_unref(&tbl);

    if (g_smp_info.cpu_count == 0) {
        g_smp_info.cpu_count = 1;
        g_smp_info.cpus[0].apic_id = 0;
        g_smp_info.cpus[0].enabled = 1;
    }

    serial_puts("  [uACPI] MADT: LAPIC=0x");
    serial_putu(g_smp_info.lapic_paddr);
    serial_puts(" detected CPUs=");
    serial_putu(g_smp_info.cpu_count);
    serial_puts(" IOAPIC=0x");
    serial_putu(g_smp_info.ioapic_paddr);
    serial_puts("\n");

    return g_smp_info.cpu_count;
}
