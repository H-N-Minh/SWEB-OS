# sweb
[![Build Status](https://travis-ci.org/IAIK/sweb.svg?branch=main)](https://travis-ci.org/IAIK/sweb)

SWEB Educational OS

Please have a look at https://www.iaik.tugraz.at/os


# Operating Systems Kernel Development in SWEB

This repository contains our solutions for the practical assignments of an Operating Systems course. The project involves extending the SWEB educational kernel with core, modern OS features, moving it from a simple single-threaded system to a more capable, concurrent operating system.

### Technical Overview

*   **Environment:** SWEB Operating System Framework
*   **Language:** C/C++
*   **Core Concepts Covered:** Kernel Development, Multithreading, Process Management, Virtual Memory (Paging, Swapping, Copy-on-Write), System Calls (Syscalls), and Concurrency/Synchronization.

---

### Assignment 1: Concurrency and Process Management

This assignment laid the groundwork for a multitasking operating system by introducing threads and processes.

*   **Multithreading:** The SWEB kernel was upgraded from a single-threaded to a fully multithreaded environment. This involved implementing the underlying kernel mechanisms to manage thread contexts, individual stacks, and registers, while sharing the process's address space. Key POSIX-compliant syscalls (`pthread_create`, `pthread_join`, `pthread_exit`) were implemented to expose this functionality to user-space programs.

*   **Process Creation (`fork`):** The `fork` system call was implemented to enable a process to create a new child process. This required deep duplication of process structures, including page tables and file descriptors, to create an almost identical copy of the parent.

---

### Assignment 2: Advanced Virtual Memory Management

This assignment focused on building a robust virtual memory system capable of handling memory pressure gracefully.

*   **Swapping:** A swapping mechanism was implemented to allow the system to use more memory than is physically available. This involved creating a kernel thread to select memory pages and write them to a swap partition on disk. A page replacement algorithm (e.g., Aging, WSClock) was used to intelligently decide which pages to swap out, and the page fault handler was extended to swap pages back into RAM on demand.

*   **Copy-on-Write (CoW):** The `fork` system call was optimized using Copy-on-Write. Instead of immediately duplicating all memory pages for a new process, parent and child processes now share read-only pages. A true copy is only made at the moment a write occurs, significantly speeding up process creation and reducing memory consumption.

---

### Key Challenges & Learnings

A major focus throughout this project was on **correct synchronization**. Great care was taken to implement locking mechanisms properly to prevent race conditions, deadlocks, and other concurrency-related bugs, which are notoriously difficult to debug in a kernel environment. The project provided deep, hands-on experience with the low-level mechanics that underpin all modern operating systems.```
