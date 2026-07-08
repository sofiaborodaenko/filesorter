# File Sorter — Concurrent File Organization System

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
- **Round-Robin Job Distribution** — Assigns files evenly across workers using a deterministic scheduling strategy
- **Filename Validation** — Filters invalid files before processing by ensuring filenames contain:
    - At least one underscore (_)
    - Exactly one period (.)
    - No whitespace characters
-**Automated File Organization** — Cleans filenames, determines categories, creates target directories, and moves files automatically.
- **File Statistics Collection** — Calculates metadata including:
    - Line count
    - Word count
    - File size
- **Asynchronous Result Handling** — Uses select() to monitor multiple worker pipes simultaneously without blocking.
- **Robust Error Handling** — Handles invalid directories, failed communication, missing valid files, and worker failures gracefully.
- **Summary Reporting** — Displays a final report containing processed files, categories, destination directories, and collected statistics.

--- 
## Architecture

The application follows a parent-worker architecture:

**Parent Process Responsibilities**
- Reads the target directory from the user
- Validates filenames
- Creates worker processes
- Assigns jobs using round-robin scheduling
- Monitors worker responses using select()
- Collects results and prints the final summary
- Cleans up child processes using wait()
- Worker Process Responsibilities

**Each worker independently:**

- Receives a filename from its job pipe
- Cleans the filename
- Determines the target directory
- Categorizes the file
- Calculates file statistics
- Creates/moves files into directories
- Sends the processed result back to the parent

--- 
## Concurrency Model

The project uses a fixed worker pool concurrency model.

At startup:

1. Pipes are created for communication.
2. The parent creates multiple workers using fork().
3. Each worker closes unused file descriptors.
4. The parent distributes files among workers.

---
## Build Instructions

Compile the project using:
```c
make
```
This builds the source files and generates the executable.

Run the program with:
```c
./a3
```
The program will prompt:
```c
Enter the directory to organize:
```
Provide a directory path relative to the project location.

Example:
```c
messy_folder
```
The included test directory contains sample files that demonstrate the organization process.

---
## What I Learned / Challenges

The biggest challenge in this project was designing a reliable communication system between multiple processes while ensuring that no worker could block the entire application.

Implementing the worker pool required careful management of file descriptors, process creation, and synchronization. Using pipes allowed each worker to operate independently, while select() enabled the parent process to asynchronously handle results from multiple workers.

This project strengthened my understanding of operating system concepts including process management, inter-process communication, concurrency models, and robust error handling.

---

## Contact

**Sofia Borodaenko**

Portfolio • [LinkedIn](https://www.linkedin.com/in/sofia-borodaenko/) • [GitHub](https://github.com/sofiaborodaenko)
