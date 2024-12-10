#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/init.h>
#include <linux/net.h>
#include <linux/inet.h>
#include <linux/socket.h>
#include <net/sock.h>

#define DEST_IP "192.168.123.79"  // Destination IP address
#define DEST_PORT 1104           // Destination UDP port
struct socket *sock;
struct sockaddr_in remote_addr = {0};
struct msghdr msg = {0};
struct kvec iov;

// int perform_udp_request(void);

static int remote_storage_init(void) {
    printk(KERN_INFO "Network module loaded\n");
    int ret;

    ret = sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);
    if (ret < 0) {
        printk(KERN_ERR "Failed to create socket\n");
        return ret;
    }

    printk(KERN_INFO "Socket created\n");

    // sock->file->f_flags |= O_NONBLOCK;

    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(DEST_PORT); // Set destination port
    remote_addr.sin_addr.s_addr = in_aton(DEST_IP);

    printk(KERN_INFO "UDP Client: Destination address set to %s:%d\n", DEST_IP, DEST_PORT);

    return 0;
}

static void remote_storage_exit(void) {
    sock_release(sock);
    printk(KERN_INFO "Network module unloaded\n");
}

int call_remote_storage(char* filename, size_t size, unsigned long index) {
    char *data;
    data = kmalloc(1024, GFP_KERNEL);
    if (!data) {
        printk(KERN_ERR "Remote: Failed to allocate memory for buffer\n");
        return -ENOMEM;
    }
    sprintf(data, "%s,%ld,%lu", filename, size, index);
    size_t data_len = strlen(data);
    int ret = 0;

    //Initialize the socket
    if (!sock) {
        printk(KERN_INFO "Remote: UDP Client: Socket is not initialized. Initializing now...\n");
        ret = remote_storage_init();
        if (ret < 0) {
            printk(KERN_ERR "Remote: UDP Client: Failed to initialize socket, error %d\n", ret);
            return ret;
        }
    }

    iov.iov_base = data; // Message data
    iov.iov_len = data_len;

    msg.msg_name = &remote_addr; // Set destination address
    msg.msg_namelen = sizeof(remote_addr);

    ret = kernel_sendmsg(sock, &msg, &iov, 1, data_len);
    if (ret < 0) {
        printk(KERN_ERR "Remote: UDP Client: Failed to send message, error %d\n", ret);
    } else {
        printk(KERN_INFO "Remote: UDP Client: Successfully sent %d bytes to %s:%d\n", ret, DEST_IP, DEST_PORT);
    }
    kfree(data);


    char* buffer;
    buffer = kmalloc(1024, GFP_KERNEL);
    if (!buffer) {
        printk(KERN_ERR "Remote: Failed to allocate memory for buffer\n");
        return -ENOMEM;
    }
    memset(buffer, 0, 1024);

    iov.iov_base = buffer;
    iov.iov_len = 1024;

    ret = kernel_recvmsg(sock, &msg, &iov, 1, iov.iov_len, MSG_WAITALL);
    if (ret < 0 && ret != -EAGAIN) {
        printk(KERN_ERR "Remote: kernel_recvmsg failed: %d\n", ret);
        // Handle error
    } else {
        if (ret > 0) {
            buffer[ret] = '\0';
            pr_info("Remote: Received message: %s\n", buffer);
        }
    }
    kfree(buffer);
    // Step 4: Clean up the socket
    remote_storage_exit();
    printk(KERN_INFO "Remote: UDP Client: Socket released\n");

    return 0;
}
EXPORT_SYMBOL(call_remote_storage);

