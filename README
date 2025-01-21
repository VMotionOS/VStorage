# VRemote Storage: In-kernel multi-node file management

## Overview
VRemote Storage consists of 2 parts: the VRemote Storage [kernel module](https://github.com/sabaebrahimi/VRemoteStorage-submodule) and this kernel code. The kernel module runs with the kernel code to simultaneously handle messages between two nodes. 
The main job is to synchronize the shared PageCache between nodes. 

### Key Features
- **Distributed PageCache Support**: Synchronizes file operations between two nodes using a shared PageCache.
- **User Flags for Remote Operations**: Introduces `O_REMOTE` and `O_ORIGIN` flags for user-level applications to handle remote files seamlessly.
- **Kernel Module (`server_file_module.c`)**: Provides server-side operations to facilitate file handling and synchronization between nodes.

---

## Setup Instructions
### Prerequisites
- A Linux kernel development environment.
- Two nodes with reachable network connectivity.


First, clone this repository, which is the modified kernel version 6.11.6, and the [submodule](https://github.com/sabaebrahimi/VRemoteStorage-submodule). You can change the port and IPs of the nodes by modifying `fs/udp_module.c`:
```
#define VM2_IP {NODE2_IP}
#define VM1_IP {NODE1_IP}  
#define DEST_PORT {YOUR_PORT} 
```
Make the kernel using:
```
make -j$(nproc)
```
Change the directory to the VRemoteStorage-submodule and change the MakeFile KERNEL_SOURCE with your own directory of the modified kernel:
```
KERNEL_SOURCE := {Your/Path/to/VRemoteStorageKernel}
```

Also, you can change the port by modifying server_file_module.c:
```
#define SERVER_PORT {YOUR_PORT}
```

Save this and make a VRemoteStorage submodule. Start your nodes with VRemoteStorage Kernel and copy `server_file_module.ko` in them. Append `server_file_module.ko` to their kernels using:
```
insmod server_file_module.ko
```
You should see the message `UDP Server: Listening on port {YOUR PORT}` if everything is done fine. 

To work with remote storage in your userspace applications, you should add `O_REMOTE` flag when opening the file. Also, for opening the files that originally belonged to the node, add `O_ORIGIN` too. Example:
```c
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#define O_REMOTE   00000020
#define O_ORIGIN   10000000
int main() {
        int fd;
        fd = open("YOURFILE.txt", O_RDWR | O_REMOTE | O_ORIGIN);

        char* buffer = (char* )malloc(512 * sizeof(char));      

        if (fd < 0) {
                perror("Error openning file\n");
                exit(1);
        }
        int fsize = read(fd, buffer, 256);
        printf("file content: %s\n", buffer);
        free(buffer);
}
```

## Repository Structure

### 1. Main Kernel Repository
Contains the modified Linux kernel source code:
- Changes to the PageCache mechanism for distributed file operations.
- New flags `O_REMOTE` and `O_ORIGIN` for file management.
- Network-based file synchronization implementation (`fs/udp_module.c`).

#### Key Modified Files
- `fs/ext4/inode.c`: Enhancements for writing to the distributed PageCache.
- `fs/fcntl.c`: Support for additional open flags.
- `fs/udp_module.c`: Handles communication and file synchronization between nodes.
- `mm/filemap.c`: Manages PageCache-related operations and remote synchronization.

### 2. Submodule Repository
Contains `server_file_module.c`, a kernel module that:
- Acts as a server for remote file synchronization.
- Handles UDP-based communication between nodes.
- Manages file-related metadata and operations.

---
