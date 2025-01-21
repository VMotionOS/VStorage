# VRemote Storage: In-kernel multi-node file management

## Overview
VRemote Storage consists of 2 parts: the VRemote Storage [kernel module](https://github.com/sabaebrahimi/VRemoteStorage-submodule) and this kernel code. The kernel module runs with the kernel code to simultaneously handle messages between two nodes. 
The main job is to synchronize the shared PageCache between nodes. 

## How to get started
First, clone this repository, which is the modified kernel version 6.11.6, and the [submodule](https://github.com/sabaebrahimi/VRemoteStorage-submodule). You can change the port and IPs of the nodes by modifying `fs/udp_module.c`:
```
#define VM2_IP {NODE2_IP}
#define VM1_IP {NODE1_IP}  
#define DEST_PORT {YOUR_PORT} 
```
Make the kernel using:
```
make -j 50
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
