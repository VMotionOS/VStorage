# VRemote Storage: Distributed PageCache for Linux

[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)

## Overview
VRemote Storage is a Linux kernel driver that enables distributed PageCache management across multiple nodes. It implements the MSI (Modified-Shared-Invalid) protocol for cache coherency and operates directly within the Linux kernel as a network device driver.

## Features
- 🔄 Distributed PageCache with MSI protocol
- 🚀 Zero-copy data transfer between nodes
- 🔒 Cache coherency management
- 🛠 User-level remote operation flags (`O_REMOTE`, `O_ORIGIN`)
- 📡 Efficient network communication

## Quick Start

### Prerequisites
- Linux kernel development environment
- Two networked nodes
- Root access on both systems

## Build Instructions

1. Clone the repository:
```bash
git clone https://github.com/VMotionOS/VStorage
cd VStorage
```

2. Configure node IPs in `drivers/net/remote_storage/remote_storage.h`:
```c
#define NODE1_IP "192.168.123.78"
#define NODE2_IP "192.168.123.79"
#define REMOTE_PORT 1104
```

3. Configure kernel:
```bash
make menuconfig
# Enable: 
#   Networking support -> 
#     Remote Storage driver (NEW)
#   Select 'y' for built-in version by default is this
```

4. Build the kernel:
```bash
make -j$(nproc)
```

5. Install modules and kernel:
```bash
sudo make modules_install
sudo make install
```

6. Update GRUB and reboot:
```bash
sudo update-grub
sudo reboot
```

7. After reboot, verify the driver is loaded:
```bash
dmesg | grep "Remote Storage"
```

## Usage Example

```c
#include <fcntl.h>
#include <stdio.h>

#define O_REMOTE   00000020
#define O_ORIGIN   10000000

int main() {
    int fd = open("myfile.txt", O_RDWR | O_REMOTE | O_ORIGIN);
    if (fd < 0) {
        perror("Failed to open file");
        return 1;
    }

    char buffer[512];
    ssize_t bytes = read(fd, buffer, sizeof(buffer));
    printf("Read %zd bytes\n", bytes);
    
    close(fd);
    return 0;
}
```

## Architecture

### Cache States
| State | Description | Actions |
|-------|-------------|---------|
| Modified | Local changes | Needs writeback |
| Shared | Synchronized | Read-only access |
| Invalid | Not available | Must fetch from origin |

### Network Protocol
Message format: `filename,size,index,operator[,data]`

| Operator | Description |
|----------|-------------|
| r | Read request |
| w | Write request |
| i | Invalidate request |

## Implementation

Key components in `drivers/net/remote_storage/`:
```
├── remote_client.c   # Client operations
├── remote_server.c   # Request handling
└── remote_storage.h  # Common definitions
```

Modified kernel files:
- `mm/filemap.c`: PageCache operations
- `fs/fcntl.c`: Remote operation flags

## Troubleshooting

### Common Issues

#### Connection Problems
✔️ Check IP configurations  
✔️ Verify network connectivity  
✔️ Ensure port is open  

#### Performance Issues
✔️ Monitor network latency  
✔️ Check system resources  
✔️ Verify cache states  

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the GPL v2 License - see the [LICENSE](LICENSE) file for details.

## Authors

- Amir Noohi
- Saba Ebrahimi

---

<sub>Made with ❤️ for the Linux kernel community</sub>
