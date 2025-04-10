/*
=====================================
Assignment 5 Submission
Name: Sumit Kumar
Roll number: 22CS30056
=====================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define MAX_OPEN_CONNECTIONS 10

#define MAX_TASKS 1000
#define BUFFER_SIZE 1024
#define CONNECTION_TIMEOUT 10

// Semaphore operations for mutex implementation
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// Key for the semaphore
#define SEM_KEY 0x1234

// Semaphore ID
int sem_id;

// Task Details
typedef struct {
    char task[BUFFER_SIZE];
    int assigned;
    int completed;
    pid_t assigned_to;
    time_t assigned_time;
} Task;

// Client info present to the server
typedef struct {
    int socket;
    int has_task;
    int task_id;
    pid_t pid_handler;
    time_t last_active;
} ClientInfo;

// Shared memory for the tasks
Task *tasks;
int task_count = 0;

// Client information
ClientInfo clients[MAX_CLIENTS];
int client_count = 0;
int client_id_counter = 0;

// Initialize semaphore for mutex
int init_semaphore() {
    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (sem_id < 0) {
        perror("semget failed");
        return -1;
    }

    union semun arg;
    arg.val = 1; // Initialize to 1 - mutex is available
    if (semctl(sem_id, 0, SETVAL, arg) < 0) {
        perror("semctl failed");
        return -1;
    }

    printf("Semaphore initialized successfully\n");
    return 0;
}

// Lock the mutex
void lock_mutex() {
    struct sembuf sb = {0, -1, 0}; // Decrement by 1 - obtain the mutex
    if (semop(sem_id, &sb, 1) < 0) {
        perror("semop lock failed");
        exit(1);
    }
}

// Unlock the mutex
void unlock_mutex() {
    struct sembuf sb = {0, 1, 0}; // Increment by 1 - release the mutex
    if (semop(sem_id, &sb, 1) < 0) {
        perror("semop unlock failed");
        exit(1);
    }
}

// Setup shared memory for tasks
int setup_shared_memory() {
    // Create shared memory segment
    int shm_id = shmget(IPC_PRIVATE, MAX_TASKS * sizeof(Task), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget failed");
        return -1;
    }

    // Attach shared memory segment
    tasks = (Task *)shmat(shm_id, NULL, 0);
    if (tasks == (Task *)-1) {
        perror("shmat failed");
        return -1;
    }

    printf("Shared memory setup successfully\n");
    return 0;
}

int load_tasks(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open the config or the tasks file.\n");
        return -1;
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file) && task_count < MAX_TASKS) {
        line[strcspn(line, "\n")] = 0;

        if (strlen(line) == 0) continue; // skip empty lines
        
        strcpy(tasks[task_count].task, line);
        tasks[task_count].assigned = 0;
        tasks[task_count].completed = 0;
        tasks[task_count].assigned_to = 0;
        task_count++;
    }

    fclose(file);
    printf("Loaded %d tasks to be performed from the file: %s\n", task_count, filename);
    return task_count;
}

int set_nonblocking(int socket_fd) {
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        return -1;
    }

    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK failed");
        return -1;
    }

    return 0;
}

void handle_sigchld(int sig) {
    (void) sig;

    pid_t pid;
    int status;

    // WNOHANG with waitpid makes non blocking
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("Child process %d terminated\n", pid);

        lock_mutex();

        // Update client and status if child is handling the client
        int found = 0;
        for (int i = 0; i < client_count; i++) {
            if (clients[i].pid_handler == pid) {
                found = 1;
                printf("Removing client handler for socket %d (index %d)\n", clients[i].socket, i);

                // Mark client task as unassigned
                if (clients[i].has_task) {
                    int task_id = clients[i].task_id;
                    if (task_id >= 0 && task_id < task_count) {
                        // Only unassign if the task is not yet completed
                        if (!tasks[task_id].completed) {
                            tasks[task_id].assigned = 0;
                            tasks[task_id].assigned_to = 0;
                            printf("Task '%s' returned to queue\n", tasks[task_id].task);
                        }
                    }
                }

                close(clients[i].socket);

                for (int j = i; j < client_count - 1; j++) {
                    clients[j] = clients[j + 1];
                }
                client_count--;
                break;
            }
        }

        unlock_mutex();

        if (!found) {
            printf("Warning: Terminated child process %d was not found in client array\n", pid);
        }
    }
}

void check_timeouts() {
    time_t current_time = time(NULL);

    lock_mutex();

    for (int i = 0; i < client_count; i++) {
        if (current_time - clients[i].last_active > CONNECTION_TIMEOUT) {
            printf("Client on socket %d timed out. Closing connection.\n", clients[i].socket);

            // Unassign the client from the task
            if (clients[i].has_task) {
                int task_id = clients[i].task_id;
                if (task_id >= 0 && task_id < task_count) {
                    // Only unassign if the task is not yet completed
                    if (!tasks[task_id].completed) {
                        tasks[task_id].assigned = 0;
                        tasks[task_id].assigned_to = 0;
                        printf("Task '%s' returned to queue\n", tasks[task_id].task);
                    }
                }
            }

            //terminate the client process handling client
            //NOTE: here as i will be running everything on my local system so i can have all the preocess access
            //from my terminal. 
            //i am sending a kill signal manually by the kill syscall
            if (clients[i].pid_handler > 0) {
                printf("Sending termination signal to process %d\n", clients[i].pid_handler);
                char cmd[100];
                sprintf(cmd, "kill -15 %d 2>/dev/null", clients[i].pid_handler);
                system(cmd);
            }

            close(clients[i].socket);

            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            i--;
        }
    }

    unlock_mutex();
}

int all_tasks_completed() {
    lock_mutex();
    
    for (int i = 0; i < task_count; i++) {
        if (!tasks[i].completed) {
            unlock_mutex();
            return 0;
        }
    }
    
    unlock_mutex();
    return 1;
}

// Function which handles client after forking 
void handle_client(int client_socket, int client_id) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Searching the client index in the global Clients array
    int client_idx = -1;
    
    lock_mutex();
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket == client_socket) {
            client_idx = i;
            break;
        }
    }
    unlock_mutex();

    if (client_idx == -1) {
        return;
    }

    // Read data from client
    bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Received from client: %s\n", buffer);

        lock_mutex();
        clients[client_idx].last_active = time(NULL);
        unlock_mutex();

        // GET_TASK request
        if (strncmp(buffer, "GET_TASK", 8) == 0) {
            printf("Client %d requested a task\n", client_id);

            lock_mutex();
            
            // If the client has already taken a task do not assign it
            // In this implementation we are assuming that each client at a time is given either a single task or it has no task at all
            if (clients[client_idx].has_task) {
                unlock_mutex();
                const char* msg = "Error: You already have a task assigned. Complete it first.\n";
                send(client_socket, msg, strlen(msg), 0);
                printf("Client %d already has a task assigned\n", client_id);
                return;
            }

            // Find a task that is neither assigned nor completed
            int task_id = -1;
            for (int i = 0; i < task_count; i++) {
                if (!tasks[i].assigned && !tasks[i].completed) {
                    task_id = i;
                    break;
                }
            }

            if (task_id >= 0) {
                tasks[task_id].assigned = 1;
                tasks[task_id].assigned_to = getpid();
                tasks[task_id].assigned_time = time(NULL);

                // Update client status
                clients[client_idx].has_task = 1;
                clients[client_idx].task_id = task_id;
                
                unlock_mutex();

                char task_msg[BUFFER_SIZE];
                // Ensure we don't exceed buffer size
                int max_task_len = BUFFER_SIZE - 10; // Space for "Task: ", "\n", and null terminator
                char truncated_task[max_task_len];
                
                lock_mutex();
                strncpy(truncated_task, tasks[task_id].task, max_task_len - 1);
                unlock_mutex();
                
                truncated_task[max_task_len - 1] = '\0';

                snprintf(task_msg, BUFFER_SIZE, "Task: %s\n", truncated_task);
                int sent = send(client_socket, task_msg, strlen(task_msg), 0);

                printf("Assigned task '%s' to client %d (bytes sent: %d)\n", truncated_task, client_id, sent);
            } else {
                unlock_mutex();
                
                // No tasks available
                const char* msg = "No tasks available\n";
                send(client_socket, msg, strlen(msg), 0);
                printf("No tasks available for client %d\n", client_id);
            }
        }
        // "RESULT" from client
        else if (strncmp(buffer, "RESULT", 6) == 0) {
            lock_mutex();
            
            // Check if client has a task assigned
            if (!clients[client_idx].has_task) {
                unlock_mutex();
                const char* msg = "Error: You don't have a task assigned.\n";
                send(client_socket, msg, strlen(msg), 0);
                return;
            }

            int task_id = clients[client_idx].task_id;

            // Mark task as completed
            tasks[task_id].completed = 1;
            printf("Task '%s' completed. Result: %s\n", tasks[task_id].task, buffer);
            
            clients[client_idx].has_task = 0;
            clients[client_idx].task_id = -1;
            
            unlock_mutex();

            // Acknowledge result
            const char* msg = "Result received. You can request another task.\n";
            send(client_socket, msg, strlen(msg), 0);
        }
        // Client exit 
        else if (strncmp(buffer, "exit", 4) == 0) {
            printf("Client %d requested to exit\n", client_id);
            
            lock_mutex();
            
            // If client had a task, mark it as unassigned
            if (clients[client_idx].has_task) {
                int task_id = clients[client_idx].task_id;
                if (task_id >= 0 && task_id < task_count) {
                    // Only unassign if the task is not yet completed
                    if (!tasks[task_id].completed) {
                        tasks[task_id].assigned = 0;
                        tasks[task_id].assigned_to = 0;
                    }
                }
            }
            
            unlock_mutex();

            // Send acknowledgment
            const char* msg = "Goodbye!\n";
            send(client_socket, msg, strlen(msg), 0);
            
            // Exit the child process
            exit(0);
        }
    } else if (bytes_read == 0) {
        // Client disconnected
        printf("Client %d disconnected\n", client_id);
    
        lock_mutex();
        
        // If client had a task, mark it as unassigned
        if (clients[client_idx].has_task) {
            int task_id = clients[client_idx].task_id;
            if (task_id >= 0 && task_id < task_count) {
                // Only unassign if the task is not yet completed
                if (!tasks[task_id].completed) {
                    tasks[task_id].assigned = 0;
                    tasks[task_id].assigned_to = 0;
                }
            }
        }
        
        unlock_mutex();
        
        exit(0);
    } else if (bytes_read == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("recv");
            exit(1);
        }
        // No data available, just return
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <task_file>\n", argv[0]);
        return 1;
    }

    // Initialize semaphore for mutex
    if (init_semaphore() < 0) {
        fprintf(stderr, "Failed to initialize semaphore\n");
        return 1;
    }

    // Setup shared memory for tasks
    if (setup_shared_memory() < 0) {
        fprintf(stderr, "Failed to setup shared memory\n");
        return 1;
    }

    if (load_tasks(argv[1]) <= 0) {
        printf("No tasks loaded. Exiting.\n");
        return 1;
    }

    signal(SIGCHLD, handle_sigchld);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Server socket() creation failure\n");
        return 1;
    }

    int opt = 1;
    // Reuse address socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt error()");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind() failed");
        return 1;
    }

    if (listen(server_fd, MAX_OPEN_CONNECTIONS) < 0) {
        perror("listen() failed");
        return 1;
    }

    printf("Task Queue Server started on port %d\n", PORT);
    printf("Waiting for connections...\n");

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = -1;
        clients[i].pid_handler = -1;
        clients[i].has_task = 0;
        clients[i].task_id = -1;
        clients[i].last_active = 0;
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        check_timeouts();

        // Check if all tasks are completed and no clients are connected
        if (all_tasks_completed() && client_count == 0) {
            printf("All tasks completed and no clients connected. Shutting down server.\n");
            break;
        }

        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_socket >= 0) {
            printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
            if (set_nonblocking(client_socket) < 0) {
                close(client_socket);
                continue;
            }

            lock_mutex();
            
            if (client_count >= MAX_CLIENTS) {
                unlock_mutex();
                printf("Too many clients. Rejecting connection.\n");
                const char* msg = "Server is busy. Try again later.\n";
                send(client_socket, msg, strlen(msg), 0);
                close(client_socket);
                continue;
            }

            //giving an ID to each client
            //AT here we should generate a more secure unrandom number
            //not just simple incrementation
            int client_id = client_id_counter++;

            int client_idx = client_count;
            clients[client_idx].socket = client_socket;
            clients[client_idx].has_task = 0;
            clients[client_idx].task_id = -1;
            clients[client_idx].last_active = time(NULL);
            client_count++;
            
            unlock_mutex();

            // Handle client by forking
            pid_t pid = fork();

            if (pid < 0) {
                perror("fork() in client failed");
                close(client_socket);
                
                lock_mutex();
                client_count--;
                unlock_mutex();
                
                continue;
            }

            if (pid == 0) {
                close(server_fd);

                lock_mutex();
                
                for (int i = 0; i < client_count; i++) {
                    if (clients[i].socket == client_socket) {
                        clients[i].pid_handler = getpid();
                        break;
                    }
                }
                
                unlock_mutex();

                printf("Child process %d handling client %d on socket %d\n", getpid(), client_id, client_socket);
                
                while (1) {
                    handle_client(client_socket, client_id);
                    sleep(1);
                }

                // This should end at up only
                close(client_socket);
                exit(0);
            } else {
                lock_mutex();
                clients[client_idx].pid_handler = pid;
                unlock_mutex();
                
                printf("Spawned child process %d to handle client %d (index %d)\n", pid, client_id, client_idx);
            }
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept() failed");
            // continue with the server loop
        }
        sleep(1);
    }
    
    // Clean up semaphore
    semctl(sem_id, 0, IPC_RMID, 0);
    
    // Detach from shared memory
    shmdt(tasks);
    
    return 0;
}