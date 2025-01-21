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
#include <linux/inetdevice.h>
#include <linux/jiffies.h>

#define VM2_IP "192.168.123.79"
#define VM1_IP "192.168.123.78"  
#define DEST_PORT 1104           // Destination UDP port
struct socket *sock;
struct sockaddr_in remote_addr = {0};
struct msghdr msg = {0};
struct kvec iov;

static uint32_t vmsg_ip_to_uint32(char *ip)
{
    uint32_t a, b, c, d;  // Variables to hold each octet of the IP address
    char ch;              // Variable to store the separator characters (dots)

    /* Parse the IP address string into four integers. The expected format is
     * "xxx.xxx.xxx.xxx", where each "xxx" is a number between 0 and 255.
     * The sscanf function reads the string and extracts the numbers, placing
     * them into a, b, c, and d. The 'ch' variable is used to ensure the correct
     * number of dots are present.
     */
    if (sscanf(ip, "%u%c%u%c%u%c%u", &a, &ch, &b, &ch, &c, &ch, &d) != 7) {
        pr_err("vmsg_ip_to_uint32: Invalid IP address format: %s\n", ip);  // Log an error if parsing fails
        return 0;  // Return 0 to indicate an invalid IP address
    }

    /* Validate each octet to ensure it is within the range [0, 255].
     * If any octet is out of range, log an error and return 0.
     */
    if (a > 255 || b > 255 || c > 255 || d > 255) {
        pr_err("vmsg_ip_to_uint32: IP address octet out of range: %s\n", ip);  // Log an error if any octet is invalid
        return 0;  // Return 0 for invalid input
    }

    /* Combine the four octets into a single 32-bit integer.
     * The result is a 32-bit value where each byte represents an octet
     * of the IP address, in network byte order (big-endian).
     */
    return (a << 24) | (b << 16) | (c << 8) | d;
}

static int is_local_ip(uint32_t ip)
{
    struct net_device *dev;         // Pointer to a network device structure
    struct in_device *in_dev;       // Pointer to an in_device structure for IPv4 configuration
    struct in_ifaddr *if_info;      // Pointer to an in_ifaddr structure for interface addresses
    int is_local = 0;               // Flag to indicate if the IP is local
    uint32_t ip_network_order = htonl(ip);  // Convert IP to network byte order

    rtnl_lock();  // Lock the network device list to ensure thread safety during iteration

    /* Iterate over each network device (network interface) in the system.
     * The for_each_netdev macro simplifies the iteration process.
     */
    for_each_netdev(&init_net, dev) {
        in_dev = __in_dev_get_rtnl(dev);  // Get the IPv4 configuration for the device

        /* If the device has an IPv4 configuration, check its assigned IP addresses. */
        if (in_dev) {
            in_dev_for_each_ifa_rtnl(if_info, in_dev) {
                /* Compare each IP address on the interface with the input IP.
                 * If a match is found, set the is_local flag and break out of the loop.
                 */
                if (if_info->ifa_local == ip_network_order) {
                    is_local = 1;  // Mark the IP as local
                    pr_info("is_local_ip: Match found for IP %pI4 on interface %s\n",
                            &ip_network_order, dev->name);  // Log the matching interface
                    goto out;  // Exit the loop early if a match is found
                }
            }
        }
    }

out:
    rtnl_unlock();  // Unlock the network device list
    return is_local;  // Return whether the IP is local or not
}


static int remote_storage_init(void) {
    printk(KERN_INFO "Network module loaded\n");
    int ret;

    ret = sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);
    if (ret < 0) {
        printk(KERN_ERR "Failed to create socket\n");
        return ret;
    }
    char *dest_ip = VM2_IP;
    if (is_local_ip(vmsg_ip_to_uint32(VM2_IP))) 
        dest_ip = VM1_IP;
    
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(DEST_PORT); // Set destination port
    remote_addr.sin_addr.s_addr = in_aton(dest_ip);

    return 0;
}

static void remote_storage_exit(void) {
    sock_release(sock);
    if (sock) sock = NULL;
    printk(KERN_INFO "Network module unloaded\n");
}

int call_remote_storage(struct remote_request request) {
    char *data;
    long timeout = msecs_to_jiffies(1000);

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
        struct sock *sk = sock->sk;
        sk->sk_rcvtimeo = timeout;

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
    iov.iov_len = request.size;

    if(request.operator != 'w') {
        ret = kernel_recvmsg(sock, &msg, &iov, 1, iov.iov_len, 0);
        pr_info("received message: %s", request.buffer);
        
        if (ret < 0 && ret != -EAGAIN) {
            printk(KERN_ERR "Remote: kernel_recvmsg failed: %d\n", ret);
            goto error;
        } else {
            if (ret > 0) {
                request.buffer[ret] = '\0';
            }
        }
    }

error:
    kfree(data);
    remote_storage_exit();
    return ret;
}
EXPORT_SYMBOL(call_remote_storage);

