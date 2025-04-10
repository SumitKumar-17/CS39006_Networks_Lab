

## **Socket Programming Flags and Non-Blocking Mode**

### **1. MSG_WAITALL**
- The `MSG_WAITALL` flag ensures that the `recv` call waits until the specified amount of data is received.
- This is useful when we **know exactly how much data** we are expecting.
- However, if the sender fails to send the required data, the `recv` call gets **blocked indefinitely**, potentially leading to a **deadlock**.
- Due to this risk, `MSG_WAITALL` is generally **avoided** in scenarios where the sender’s behavior is uncertain.

---

### **2. MSG_DONTWAIT**
- The `MSG_DONTWAIT` flag makes the `recv` function **non-blocking**.
- If data is available, `recv` returns immediately with the data.
- If no data is available, `recv` **returns immediately** with an error (`EWOULDBLOCK` or `EAGAIN`), instead of blocking the process.

---

### **3. Handling Non-Blocking Mode**
- In a non-blocking socket, if `recv` or `accept` fails due to no available data, it **returns an error** instead of blocking.
- The error codes returned are:
  - `EWOULDBLOCK`
  - `EAGAIN`
- These indicate that the operation should be retried later.

---

### **4. Setting a Socket to Non-Blocking Mode**
By default, socket operations are **blocking**. To make them **non-blocking**, use `fcntl`:

```c
int flag = fcntl(sockfd, F_GETFL, 0);  // Get current socket flags
fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);  // Set the socket to non-blocking mode
```

- Now, **all socket calls (recv, accept, etc.) become non-blocking**.
- If no data is available, `recv` or `accept` **immediately returns** instead of waiting.

---

### **Summary**
| Flag | Behavior |
|------|----------|
| `MSG_WAITALL` | Blocks until all requested data is received (can lead to deadlock). |
| `MSG_DONTWAIT` | Makes `recv` non-blocking (returns immediately if no data is available). |
| `O_NONBLOCK` | Sets all socket calls to non-blocking mode (using `fcntl`). |
| `EWOULDBLOCK` / `EAGAIN` | Error codes indicating that an operation should be retried later. |
