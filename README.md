# CS302-SP25 Signal Project (xv6-sustech)

A RISC-V 64-bit POSIX-style signal subsystem implemented on top of `xv6-sustech` (a modified `xv6-riscv` running in S-mode on QEMU / OpenSBI with SMP support).

## Features

- **Signal Types**: `SIGUSR0`–`SIGUSR2`, `SIGKILL`, `SIGTERM`, `SIGCHLD`, `SIGSTOP`, `SIGCONT`, `SIGSEGV`, `SIGINT`, `SIGALRM`
- **System Calls**: `sigaction`, `sigreturn`, `sigprocmask`, `sigkill`, `sigpending`, `alarm`
- **Signal Delivery & Context**:
  - Signal masking and pending signal tracking (`sigmask`, `sigpending`)
  - Default (`SIG_DFL`), ignore (`SIG_IGN`), and custom user-space handlers with `siginfo_t` and `ucontext` save/restore
  - Signal inheritance and reset across `fork` and `exec`
  - Process job control (`SIGSTOP` / `SIGCONT`), child exit notification (`SIGCHLD`), timer alarms (`SIGALRM`), and fault handling (`SIGSEGV`)

## Project Structure

- `os/` — Kernel source (`os/signal/` contains `ksignal.c`, `ksignal.h`, `signal.h`)
- `user/` — User-space library (`user/lib/`) and test programs (`user/src/`)
- `scripts/` — Build helper scripts (`pack.py`)

## Build & Run

### Prerequisites

- `riscv64-unknown-elf-gcc` toolchain (`gcc`, `ld`, `objcopy`, `objdump`)
- `qemu-system-riscv64`
- `python3`, `perl`, `make`

### Commands

```bash
# Build kernel and user programs
make build

# Run in QEMU (single core)
make run

# Run in QEMU (4 cores SMP)
make runsmp

# Clean build artifacts
make clean
```

## Testing

Inside the QEMU shell, run the signal test suite or standalone test binaries:

```bash
# Run the main signal test suite
signal

# Or run a specific test case (e.g. basic1, sigstop_sigcont)
signal basic1

# Standalone signal tests
alarm_test
sigchld_test
stop_cont
test_siginfo
```

## License

This project is licensed under the GNU General Public License v3.0. See [LICENSE](LICENSE) for details.
