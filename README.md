# Jabberr Kernel 🚀

**Jabberr Kernel** is a lightweight, bare-metal x86 operating system built from scratch in C and Assembly. It features an interactive **Forth interpreter**, built-in retro games, a block file system, and custom kernel utilities—all running directly on hardware without external dependencies.

---

## 🌟 Key Features

* **Built-in Forth Interpreter:** Interactively define words, execute stack math, evaluate string literals, and inspect system memory.
* **Core Kernel Games:**
  * **3D Maze Game:** Low-level pseudo-3D viewport renderer running straight out of `kernel.c`.
  * **Minesweeper:** Retro logic game built into the kernel core.
* **Block File System:** Manage simple inode-based files directly from the shell (`ls`, `touch`, `cat`, `rm`).
* **Hardware & System Inspection:** Built-in tools to scan PCI network hardware (`nic`), inspect system time/date, view kernel settings, and monitor data stacks.

---

## 📁 Repository Structure

```text
JabberrKernel/
├── boot.s       # Assembly bootloader (handles early startup and jumps to C)
├── kernel.c     # Core kernel code (shell, Forth engine, 3D maze, Minesweeper, file system)
└── makefile     # Build system script
