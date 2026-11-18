<div align="center">
  <img src="documentation/logo.png" alt="OpenASD2 Logo" width="200" />
  <br><br>
  <p><b>A modern, experimental x86-64 microkernel operating system</b></p>
  
  <a href="https://t.me/openasd_ru"><img src="https://img.shields.io/badge/Telegram-2CA5E0?style=for-the-badge&logo=telegram&logoColor=white" alt="Telegram" /></a>
  <br><br>
  
  [![License: BSD 2-Clause](https://img.shields.io/badge/License-BSD_2--Clause-blue.svg)](https://opensource.org/licenses/BSD-2-Clause)
  [![Platform: x86-64](https://img.shields.io/badge/Platform-x86--64-lightgrey.svg)](#)
  [![Language: C11](https://img.shields.io/badge/Language-C11-00599C.svg)](#)
  [![Language: Assembly](https://img.shields.io/badge/Language-Assembly-4EAA25.svg)](#)
</div>

---

> **Fork of the original [OpenASD](https://github.com/komarufan/OpenASD), which is no longer maintained.**

**OpenASD2** is a research microkernel operating system built from scratch for the x86-64 architecture. Written entirely in freestanding C and assembly, it aims to explore modern OS concepts while remaining minimalistic and highly educational.

From its custom UEFI bootloader to a robust VFS and a growing userspace, OpenASD2 is a complete system that runs on bare metal.

## Core Features

- **Custom Bootloader**  
  Native UEFI boot process (`asdboot`) that sets up the hardware environment before handing off to the kernel.
- **Advanced Scheduling & SMP**  
  Implements a CFS-style (Completely Fair Scheduler) with per-CPU run queues and dynamic load balancing. Multi-core processor discovery and Local APIC enumeration powered by integrated **uACPI** parsing the ACPI MADT.
- **Robust VFS & File Systems**  
  Supports FAT16/FAT32 (read-write) for boot and storage media, and a fully functional `ramfs` (read-write) for temporary data.
- **Secure Foundation**  
  - Passwords are hashed using a salted, iterated mixing function (FNV-1a + Murmur-style finalization).
  - Strict pointer validation across all system calls to prevent userland from corrupting kernel space.
- **Port-based IPC**  
  Inter-process communication is handled via high-performance message ports and ring buffers.
- **TUI Installer**  
  A pseudo-graphical installer featuring disk selection, hostname configuration, and automated user account creation.

## Architecture

OpenASD2 follows a clean separation of concerns, dividing the system into distinct functional layers:

```text
boot/        — UEFI bootloader (asdboot)
kernel/      — The core kernel
  ├─ arch/   — Hardware specifics (GDT, IDT, ISR, PIC, PIT, Syscall, ELF)
  ├─ block/  — Block device layer and GPT parsing
  ├─ drv/    — Device drivers (virtio-blk, PS/2 keyboard, ADF)
  ├─ ipc/    — Port-based IPC mechanism
  ├─ mm/     — Physical buddy allocator & virtual memory maps
  ├─ sched/  — CFS-style scheduler
  └─ vfs/    — Virtual File System (FAT32, ramfs)
init/        — PID 1 (asdinit), TUI installer, login, and kernel shell
userland/    — Bare-metal userspace environment
  ├─ libasd/ — Minimal freestanding libc
  └─ sh/     — asdsh: The system shell
```

## Under the Hood

### System V AMD64 ABI
The kernel strictly adheres to the System V AMD64 ABI. It correctly constructs the initial stack (`argc`, `argv`, `envp`) for userland processes, ensuring seamless compatibility and standard behavior for compiled C programs.

### Syscall Interface
OpenASD2 provides a minimal POSIX-like system call interface that closely mirrors the Linux x86-64 ABI. This compatibility layer allows for straightforward porting of existing simple C programs into the OpenASD2 ecosystem.

- `rax` holds the system call number.
- `rdi`, `rsi`, `rdx`, `r10`, `r8`, `r9` carry the arguments.
- Returns are passed back via `rax`.

## Documentation

For more in-depth technical details on various subsystems, check out the `documentation/` directory in this repository. It covers everything from writing filesystem drivers to using the ASD Package Manager (APM).

## License

OpenASD2 is open-source software licensed under the **BSD 2-Clause License**. See the [LICENSE](LICENSE) file for more details.
