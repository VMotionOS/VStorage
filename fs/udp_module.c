#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/init.h>
#include <linux/net.h>
#include <linux/inet.h>
#include <linux/socket.h>
#include <net/sock.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#define DEST_IP "192.168.123.79"  // Destination IP address
#define DEST_PORT 1104           // Destination UDP port
struct socket *sock;
struct sockaddr_in remote_addr = {0};
struct msghdr msg = {0};
struct kvec iov;

static int remote_storage_init(void) {
    printk(KERN_INFO "Network module loaded\n");
    int ret;

    ret = sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);
    if (ret < 0) {
        printk(KERN_ERR "Failed to create socket\n");
        return ret;
    }

    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(DEST_PORT); // Set destination port
    remote_addr.sin_addr.s_addr = in_aton(DEST_IP);

    return 0;
}

static void remote_storage_exit(void) {
    sock_release(sock);
    if (sock) sock = NULL;
    printk(KERN_INFO "Network module unloaded\n");
}

int call_remote_storage(struct remote_request request) {
    char *data;
    data = kmalloc(1024, GFP_KERNEL);
    if (!data) {
        printk(KERN_ERR "Remote: Failed to allocate memory for buffer\n");
        return -ENOMEM;
    }
    sprintf(data, "%s,%ld,%llu,%c,%s", request.filename, request.size,
             request.index, request.operator, request.buffer[0] == '\0' ? "\0" : request.buffer);

    printk(KERN_INFO "Remote: sending data: %s\n", data);
    size_t data_len = strlen(data);
    int ret = 0;

    //Initialize the socket
    if (!sock) {
        printk(KERN_INFO "Socket not initialized \n");
        ret = remote_storage_init();
        if (ret < 0) {
            printk(KERN_ERR "Remote: UDP Client: Failed to initialize socket, error %d\n", ret);
            return ret;
        }
    }
    printk(KERN_INFO "After Socker Initialization \n");

    iov.iov_base = data; // Message data
    iov.iov_len = data_len;

    msg.msg_name = &remote_addr; // Set destination address
    msg.msg_namelen = sizeof(remote_addr);

    ret = kernel_sendmsg(sock, &msg, &iov, 1, data_len);
    pr_info("Remote: Sent message \n");

    if (ret < 0) {
        printk(KERN_ERR "Remote: UDP Client: Failed to send message, error %d\n", ret);
        goto error;
    }

    iov.iov_base = request.buffer;
    iov.iov_len = 16;

    ret = kernel_recvmsg(sock, &msg, &iov, 1, iov.iov_len, MSG_WAITALL);
    if (ret < 0 && ret != -EAGAIN) {
        printk(KERN_ERR "Remote: kernel_recvmsg failed: %d\n", ret);
        goto error;
    } else {
        if (ret > 0) {
            request.buffer[ret] = '\0';
        }
    }
error:
    kfree(data);
    remote_storage_exit();
    return ret;
}
EXPORT_SYMBOL(call_remote_storage);

