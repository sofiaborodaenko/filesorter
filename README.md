# File Sorter -- Concurrent File Organization System

A multi-process file organization system implemented in C using pipes, process synchronization, and asynchronous communication with `select()`.

The program scans a user-provided directory, distributes file processing tasks across a worker pool, categorizes files based on filename information, computes file statistics, and organizes files into structured directories.

---

## Overview

This project implements a Category 1 multi-process concurrent application that uses a parent-worker architecture to efficiently process and organize files.

The parent process acts as the coordinator by scanning the provided directory, validating filenames, and distributing jobs among multiple worker processes using a round-robin scheduling strategy. Each worker independently processes assigned files, performs file analysis, moves files into their target directories, and returns the results back to the parent process.

Communication between processes is implemented using UNIX pipes. The parent sends job requests through dedicated job pipes, while workers return processed results through separate result pipes. The parent uses `select()` to asynchronously monitor worker responses, preventing any single worker from blocking the entire system.

This project demonstrates process creation, inter-process communication, concurrent execution, synchronization, and robust error handling in a Linux environment.

---

## Features

- **Multi-Process Worker Pool** — Uses `fork()` to create multiple worker processes that execute file-processing tasks concurrently.
- **Pipe-Based Communication** — Implements bidirectional communication between parent and workers using dedicated job and result pipes.
- **Round-Robin Job Distribution** — Assigns files evenly across workers using a deterministic scheduling strategy:
  ```c
  worker = i % WORKER_COUNT;
