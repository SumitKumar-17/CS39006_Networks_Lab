# Task Queue Server for Distributed Job Processing

## Overview
A distributed job processing system with a central Task Queue Server and multiple Worker Clients. The server manages a queue of computational tasks, assigns them to clients, and collects results. 

This implementation uses System V semaphores and shared memory to ensure proper synchronization and prevent race conditions.

## Components

### Task Queue Server
- **Manages Tasks**: Loads arithmetic tasks from a config file and assigns them to clients.
- **Non-blocking I/O**: Uses `fcntl(O_NONBLOCK)` for handling connections.
- **Process-Based Concurrency**: Uses `fork()` to create child processes for client handling.
- **Client Tracking**: Maintains client details, assigned tasks, and connection status.
- **Timeout & Cleanup**: Detects idle clients, prevents zombie processes via `SIGCHLD`.
- **Task Locking**: Uses System V semaphores to ensure a task is assigned to only one client at a time.
- **Shared Memory**: Uses System V shared memory for task data to ensure consistency across processes.

### Worker Client
- **Requests & Processes Tasks**: Fetches tasks, computes results, and returns them.
- **Non-blocking Communication**: Uses `fcntl(O_NONBLOCK)` for socket operations.
- **Retry Mechanism**: Polls for responses from the server.
- **Graceful Termination**: Handles both client-initiated and server-initiated shutdowns.

## Design & Implementation

### Architecture
- **Client-Server Model**: Process-based concurrency using `fork()`.
- **Shared Memory**: The task queue is stored in a shared memory segment accessible to all child processes.
- **Inter-Process Synchronization**: System V semaphores provide mutex locking across processes.
- **Unique Client Identification**: Tracks clients with unique IDs.

### Server Workflow
1. **Initialization**
   - Initialize semaphores and shared memory for the task queue
   - Load tasks from the configuration file
   - Setup `SIGCHLD` handler, create non-blocking server socket
   - Initialize client tracking
2. **Accepting Clients**
   - Accept connections, assign unique IDs, fork child process for each client
   - Child processes inherit access to semaphores and shared memory
3. **Handling Clients**
   - Read requests (non-blocking), acquire mutex lock before task assignment
   - Verify task is neither assigned nor completed before assignment
   - Process results, updating task status with proper mutex protection
   - Release mutex lock after critical sections
4. **Timeout Management**
   - Acquire mutex lock before checking timeouts
   - Close idle connections, reassign unfinished tasks
   - Release mutex lock after updates
5. **SIGCHLD Handling**
   - Clean up terminated child processes
   - Acquire mutex lock before reclaiming unfinished tasks
   - Release mutex lock after cleanup

### Client Workflow
1. **Initialization**
   - Connect to server, set non-blocking mode, setup `SIGINT` handler
2. **Task Processing**
   - Request tasks, compute results, send responses
   - Retry on communication failures
3. **Graceful Exit**
   - Inform server before disconnecting

### Communication Protocol
- **Text-based messaging** (`GET_TASK`, `RESULT <value>`, `exit`)
- **Non-blocking Send/Receive** with retries for robustness

### Synchronization
- **Mutex Implementation**: System V semaphores provide cross-process mutex locking
- **Lock/Unlock Operations**: Explicit lock_mutex() and unlock_mutex() functions
- **Critical Sections**: All shared data access is protected by mutex locks
- **Task Assignment**: Tasks are assigned exclusively and atomically
- **Status Updates**: Task status changes (assigned/completed) are protected by mutex locks

### Shared Memory Implementation
- **Task Array**: Stored in shared memory segment accessible to all child processes
- **Attachment**: Each forked child process inherits the shared memory attachment
- **Proper Cleanup**: Shared memory is detached and removed on server shutdown

### Error Handling
- **Semaphore Initialization Failures**: Clear error reporting and cleanup
- **Shared Memory Failures**: Robust error handling and cleanup
- **Process Synchronization**: Child processes safely access shared data through mutex locks
- **Graceful Socket Closure**: Ensures clean disconnection
- **Zombie Prevention**: Uses `waitpid(WNOHANG)` for cleaning up child processes

## Race Condition Resolution

### Previous Issue
The original implementation suffered from a race condition where multiple clients could be assigned the same task due to a lack of proper synchronization between forked processes. This occurred because:
- Each child process had its own copy of the task array
- No atomic locking mechanism existed for task assignment
- Multiple processes could check task availability simultaneously

### Solution Implemented
The updated implementation resolves this issue through:
1. **System V Semaphores**: Provides a process-wide mutex lock for safe access to shared resources
2. **Shared Memory**: Ensures all processes see the same, consistent view of the task array
3. **Critical Section Protection**: All task modifications are protected by mutex locks
4. **Double-Checking Task Status**: Verifies both assigned and completed status within a locked section

### Key Synchronization Points
- **Task Assignment**: Lock, check availability, assign, unlock
- **Task Completion**: Lock, update status, unlock
- **Client Disconnection**: Lock, reclaim task if necessary, unlock
- **Timeout Processing**: Lock, check timeouts, reclaim tasks, unlock

## Key Functions Added

### Synchronization Functions
- `init_semaphore()`: Creates and initializes the System V semaphore
- `lock_mutex()`: Acquires the mutex lock
- `unlock_mutex()`: Releases the mutex lock

### Shared Memory Functions
- `setup_shared_memory()`: Creates and attaches shared memory for tasks
- Cleanup code in `main()` for proper resource release

## References
- Semaphores: [Linux Manual Page](https://man7.org/linux/man-pages/man7/sem_overview.7.html)
- Shared Memory: [Linux Manual Page](https://man7.org/linux/man-pages/man7/shm_overview.7.html)
- `waitpid(WNOHANG)`: [StackOverflow](https://stackoverflow.com/questions/33508997/waitpid-wnohang-wuntraced-how-do-i-use-these)