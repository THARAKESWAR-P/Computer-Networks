#include <stdio.h>
#include <errno.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(a) ((a) < 0 ? (-(a)) : (a))

#define MAX_HOPS 64
#define MIN_DATA_SIZE 100
#define MAX_DATA_SIZE 500
#define PACKET_SIZE 4096
#define MAX_WAIT_TIME 2
#define TIMEOUT 1000
#define ICMP_HDR_SIZE sizeof(struct icmp)

typedef struct
{
    struct sockaddr_in addr;
    int sock_fd;
    int ttl;
    int received_count;
} ping_probe;

unsigned short calculate_checksum(unsigned short *paddress, int len);
void send_ping(ping_probe *probe, struct sockaddr_in addr, int packet_size);
void send_packet(int sockfd, struct sockaddr_in dest_addr, int ttl, int packet_size);
int receive_packet(int sockfd, struct sockaddr_in *addr, char *recv_buf);

void print_header(struct icmphdr *icmph, int bytes)
{
    printf("ICMP Header: type %d, code %d, checksum %X, id %d, sequence %d, bytes %d\n",
           icmph->type, icmph->code, ntohs(icmph->checksum),
           ntohs(icmph->un.echo.id), ntohs(icmph->un.echo.sequence), bytes);
}

void print_data(char *buf, int bytes)
{
    struct icmp *hdr = (struct icmp *)buf;
    for (int i = sizeof(struct iphdr) + sizeof(struct icmphdr); i < bytes; i++)
    {
        printf("%c", hdr->icmp_data[i]);
    }

    printf("\n");
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Usage: %s <address> <packet_count> <time_difference>\n", argv[0]);
        return 1;
    }

    char *address = argv[1];
    int n = atoi(argv[2]);
    int T = atoi(argv[3]);

    struct hostent *host = gethostbyname(address);
    if (!host)
    {
        printf("Could not resolve %s\n", address);
        return 1;
    }

    printf("Pinging %s (%s):\n", address, inet_ntoa(*(struct in_addr *)host->h_addr_list[0]));

    int sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock_fd < 0)
    {
        perror("socket creation failed\n");
        return 1;
    }

    printf(" %d max hops, %d bytes packets\n\n", MAX_HOPS, PACKET_SIZE);

    struct timeval timeout = {MAX_WAIT_TIME, 0};
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

    int ttl = 1;
    setsockopt(sock_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));

    int hop_count = 0, reached = 0;

    ping_probe probe = {0};
    probe.sock_fd = sock_fd;
    probe.ttl = ttl;

    struct sockaddr_in dest_addr = {0}, addr, prev_addr = {0}, node_addr = {0};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr = *(struct in_addr *)host->h_addr_list[0];
    probe.addr = dest_addr;
    node_addr = dest_addr;
    prev_addr = dest_addr;
    inet_aton("127.0.0.1", &prev_addr.sin_addr);

    double old_rtt[10] = {0.0};

    while (ttl <= MAX_HOPS && !reached)
    {
        probe.received_count = 0;

        probe.addr = dest_addr;
        send_ping(&probe, addr, 0);

        printf("\n");

        if (probe.received_count == 0)
        {
            ttl++;
            hop_count++;
            setsockopt(sock_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
            probe.ttl = ttl;
            continue;
        }

        node_addr.sin_addr = probe.addr.sin_addr;

        printf("LINK \t %s *\t*\t* ", inet_ntoa(prev_addr.sin_addr));
        printf("%s\t\t\t\t\t\t\t<------(link x-y)\n\n", inet_ntoa(node_addr.sin_addr));

        sleep(1);

        prev_addr = probe.addr;

        // Initialize variables for calculating latency and bandwidth
        double min_latency = DBL_MAX, max_latency = 0.0, total_latency = 0.0;
        double total_bandwidth = 0.0;
        double new_rtt = 0.0, new_rtt0 = 0.0;

        // Send ICMP packets of increasing data sizes and estimate latency and bandwidth for each size
        for (int i = 0; i <= MAX_DATA_SIZE; i += 100)
        {
            char send_buf[PACKET_SIZE];
            char recv_buf[PACKET_SIZE];
            int seq_num = 0;
            int num_received = 0;

            printf("\nDatasize: %d", i);
            
            // Send num_probes packets of current size
            for (int j = 0; j < n; j++)
            {
                // Set ICMP header fields
                struct icmp *icmp_hdr = (struct icmp *)send_buf;
                icmp_hdr->icmp_type = ICMP_ECHO;
                icmp_hdr->icmp_code = 0;
                icmp_hdr->icmp_id = getpid() & 0xffff;
                icmp_hdr->icmp_seq = seq_num++;
                memset(icmp_hdr->icmp_data, 0xa5, i); // fill with arbitrary data
                icmp_hdr->icmp_cksum = 0;
                icmp_hdr->icmp_cksum = calculate_checksum((uint16_t *)icmp_hdr, sizeof(struct icmp) + i);

                for (int i = 0; i < PACKET_SIZE; i++)
                    recv_buf[i] = '\0';

                // Get current time for calculating RTT
                struct timeval start_time, end_time;
                gettimeofday(&start_time, NULL);

                // Send ICMP packet to destination address
                int bytes_sent = sendto(sock_fd, send_buf, sizeof(struct icmp) + i, 0, (struct sockaddr *)&probe.addr, sizeof(struct sockaddr));
                if (bytes_sent < 0)
                {
                    perror("Error sending packet");
                    continue;
                }

                // Receive ICMP reply packets
                int bytes_received = receive_packet(sock_fd, &addr, recv_buf);
                if (bytes_received <= 0)
                {
                    continue;
                }

                // Get current time for calculating RTT
                gettimeofday(&end_time, NULL);

                printf("\n%d. ", j + 1);
                // Process ICMP time exceeded response
                struct iphdr *ip_hdr = (struct iphdr *)recv_buf;
                int ip_hdr_len = ip_hdr->ihl * 4;
                struct icmphdr *icmp_hdr1 = (struct icmphdr *)(recv_buf + ip_hdr_len);
                print_header(icmp_hdr1, bytes_received);

                if (icmp_hdr1->type == ICMP_TIME_EXCEEDED && icmp_hdr1->code == ICMP_EXC_TTL)
                {
                    // print_header(icmp_hdr1, recv_bytes);
                    // return recv_bytes;
                }
                // If destination responds with an ICMP Echo reply, record the response
                else if (icmp_hdr1->type == ICMP_ECHOREPLY && icmp_hdr1->code == 0)
                {
                    // print_header(icmp_hdr1, recv_bytes);
                    // return recv_bytes;
                }
                else
                {
                    print_data(recv_buf, bytes_received);
                }

                // Calculate RTT and update min/max values
                double rtt = (double)(end_time.tv_sec - start_time.tv_sec) * 1000.0 + (double)(end_time.tv_usec - start_time.tv_usec) / 1000.0;
                if (j == 0)
                    new_rtt = rtt;
                else
                {
                    new_rtt = 0.8 * new_rtt + 0.2 * rtt;
                }

                printf("rtt: %f\n", rtt);
                // if (rtt < min_latency)
                // {
                //     min_latency = rtt;
                // }
                // if (rtt > max_latency)
                // {
                //     max_latency = rtt;
                // }
                // total_latency += rtt;

                // if(i == MIN_DATA_SIZE)
                // {
                //     prev_rtt = rtt;
                //     num_received++;
                //     sleep(T);
                //     continue;
                // }

                // // Calculate bandwidth using RTT and data size and update min/max values
                // double bandwidth = (double)(i*4) / ((rtt-prev_rtt) / 1000000.0 * 1024.0 * 1024.0);
                // if (bandwidth < min_bandwidth)
                // {
                //     min_bandwidth = bandwidth;
                // }
                // if (bandwidth > max_bandwidth)
                // {
                //     max_bandwidth = bandwidth;
                // }
                // total_bandwidth += bandwidth;

                num_received++;
                sleep(T);
            }

            if (num_received == 0)
                continue;

            double prev_rtt = old_rtt[(i / MIN_DATA_SIZE)];
            old_rtt[(i / MIN_DATA_SIZE)] = new_rtt;

            if (i == 0)
            {
                new_rtt0 = new_rtt;
                total_latency = ABS(new_rtt - prev_rtt) / 2.0;
                continue;
            }

            double bandwidth = (double)(i * 8) / ABS((ABS(new_rtt - prev_rtt) / 2.0 - total_latency) / 1000.0 * 1024.0 * 1024.0);
            if (bandwidth > total_bandwidth)
                total_bandwidth = bandwidth;
        }

        printf("\n\nLatency: %f ms\n", total_latency);
        printf("Bandwidth: %f mbps\n\n", total_bandwidth);

        if (node_addr.sin_addr.s_addr == dest_addr.sin_addr.s_addr)
        {
            printf("DESTINATION REACHED\n\n");
            break;
        }

        ttl++;
        hop_count++;
        setsockopt(sock_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
        probe.ttl = ttl;
    }

    return 0;
}

unsigned short calculate_checksum(unsigned short *paddress, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = paddress;
    unsigned short answer = 0;

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        *(unsigned char *)(&answer) = *(unsigned char *)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}

void send_ping(ping_probe *probe, struct sockaddr_in addr, int data_size)
{

    int flag, msg_count = 0, msg_received_count = 0;
    struct timeval start_time, end_time;

    // Send num_probes packets of current size
    for (int j = 0; j < 5; j++)
    {
        sleep(1);
        struct icmp icmp_hdr;
        int packet_size = ICMP_HDR_SIZE + data_size;
        char packet[packet_size];
        int sent_bytes;

        // Initialize ICMP header
        icmp_hdr.icmp_type = ICMP_ECHO;
        icmp_hdr.icmp_code = 0;
        icmp_hdr.icmp_id = getpid();
        icmp_hdr.icmp_seq = msg_count++;
        memset(&icmp_hdr.icmp_dun, 0, sizeof(icmp_hdr.icmp_dun));

        // Copy ICMP header and data into packet buffer
        memcpy(packet, &icmp_hdr, ICMP_HDR_SIZE);
        memset(packet + ICMP_HDR_SIZE, 'A', data_size);

        // Calculate ICMP checksum and put it in header
        icmp_hdr.icmp_cksum = calculate_checksum((unsigned short *)packet, packet_size);
        memcpy(packet, &icmp_hdr, ICMP_HDR_SIZE);
        flag = 1;

        // Send packet to destination address with TTL set in IP header
        setsockopt(probe->sock_fd, IPPROTO_IP, IP_TTL, &probe->ttl, sizeof(probe->ttl));

        // Send packet to destination address
        gettimeofday(&start_time, NULL);
        sent_bytes = sendto(probe->sock_fd, packet, packet_size, 0, (struct sockaddr *)&probe->addr, sizeof(struct sockaddr));
        if (sent_bytes <= 0)
        {
            perror("sendto error");
            flag = 0;
        }

        int bytes = 0;
        char recv_buf[PACKET_SIZE] = {0};

        socklen_t len = sizeof(struct sockaddr_in);
        // printf("Probe from: %s\n\n", inet_ntoa((struct in_addr) addr->sin_addr));

        //     if ((recv_bytes = recvfrom(probe->sock_fd, recv_buf, PACKET_SIZE, 0, (struct sockaddr *) &addr, &len)) < 0)
        //     {
        //         if (errno == EWOULDBLOCK)
        //         {
        //             printf("** ");
        //         }
        //         else
        //         {
        //             perror("recvfrom failed\n");
        //         }
        //     }
        //     else
        //     {
        //         // if packet was not sent, don't receive
        //         if(flag)
        //         {
        //             gettimeofday(&end_time, NULL);

        //             printf("Packet %d sent:\n", msg_count);
        //             print_header(&icmp_hdr, sent_bytes);

        //             printf("Packet received:\n");
        //             struct icmp *pckt = (struct icmp *)recv_buf;
        //             if(!((pckt->icmp_type == 8 || pckt->icmp_type == 0 || pckt->icmp_type == 11 || pckt->icmp_type == 69) && pckt->icmp_code==0))
        //             {
        //                 printf("Error..Packet received with ICMP type %d code %d bytes %d\n",
        //                     pckt->icmp_type, pckt->icmp_code, recv_bytes);

        //                 printf("%d bytes from %s msg_seq=%d ttl=%d.\n",
        //                     recv_bytes, inet_ntoa((struct in_addr) addr.sin_addr), msg_count, probe->ttl);

        //             }
        //             else
        //             {
        //                 printf("%d bytes from %s msg_seq=%d ttl=%d.\n",
        //                     recv_bytes, inet_ntoa((struct in_addr) addr.sin_addr), msg_count, probe->ttl);

        //                 msg_received_count++;
        //                 break;
        //             }
        //         }
        //     }

        // Wait for ICMP time exceeded response
        char recv_packet[PACKET_SIZE];
        struct sockaddr_in recv_addr = {0};
        socklen_t addr_len = sizeof(recv_addr);

        int recv_bytes = recvfrom(probe->sock_fd, recv_packet, PACKET_SIZE, 0, (struct sockaddr *)&recv_addr, &addr_len);
        if (recv_bytes <= 0)
        {
            printf("%d.\t*\n", probe->ttl);
            continue;
        }

        struct in_addr source_ip, dest_ip; // source ip address

        // Process ICMP time exceeded response
        struct iphdr *ip_hdr = (struct iphdr *)recv_packet;
        int ip_hdr_len = ip_hdr->ihl * 4;
        source_ip.s_addr = ip_hdr->saddr;
        dest_ip.s_addr = ip_hdr->daddr;

        printf("Ip src: %s,\t", inet_ntoa(source_ip));
        printf("Ip dest: %s,\tttl: %d\n", inet_ntoa(dest_ip), ip_hdr->ttl);
        struct icmphdr *icmp_hdr1 = (struct icmphdr *)(recv_packet + ip_hdr_len);
        print_header(icmp_hdr1, recv_bytes);

        if (icmp_hdr1->type == ICMP_TIME_EXCEEDED && icmp_hdr1->code == ICMP_EXC_TTL)
        {
            printf("%d.\t%s\n", probe->ttl, inet_ntoa(recv_addr.sin_addr));
            // print_header(icmp_hdr1, recv_bytes);
        }

        // If destination responds with an ICMP Echo reply, record the response
        // icmp_hdr1 = (struct icmphdr *)recv_packet;
        if (icmp_hdr1->type == ICMP_ECHOREPLY && icmp_hdr1->code == 0)
        {
            printf("%d.\t%s\n", probe->ttl, inet_ntoa(recv_addr.sin_addr));
            // print_header(icmp_hdr1, recv_bytes);

            msg_received_count++;
            // if (recv_addr.sin_addr.s_addr == probe->addr.sin_addr.s_addr)
            // {
            //     flag = 0;
            //     gettimeofday(&end_time, NULL);
            //     printf("Target responded in %ld.%ld ms\n",
            //            (end_time.tv_sec - start_time.tv_sec) * 1000 +
            //                (end_time.tv_usec - start_time.tv_usec) / 1000,
            //            (end_time.tv_usec - start_time.tv_usec) % 1000);
            // }
            // break;
        }

        if (icmp_hdr1->type == ICMP_DEST_UNREACH && icmp_hdr1->code == 3)
        {
            printf("%d.\t%s\n", probe->ttl, inet_ntoa(recv_addr.sin_addr));
            // print_header(icmp_hdr1, recv_bytes);

            break;
        }
        probe->addr = recv_addr;
        probe->received_count++;
        break;
    }
}

void send_packet(int sockfd, struct sockaddr_in dest_addr, int ttl, int packet_size)
{
    struct icmp *icmp_packet = (struct icmp *)malloc(sizeof(struct icmp) + packet_size);
    icmp_packet->icmp_type = 8;
    icmp_packet->icmp_code = 0;
    icmp_packet->icmp_id = htons(getpid());
    icmp_packet->icmp_seq = htons(ttl);
    memset(icmp_packet->icmp_data, 0xa5, packet_size);
    memset(icmp_packet->icmp_data, 'A', packet_size);
    icmp_packet->icmp_cksum = calculate_checksum((unsigned short *)icmp_packet, sizeof(struct icmp) + packet_size);

    int sendbytes;
    // setsockopt(sock_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));

    // printf("Probe to: %s\n", inet_ntoa((struct in_addr) dest_addr.sin_addr));
    // printf("size of icmp: %d\n", sizeof(struct icmp));
    if ((sendbytes = sendto(sockfd, icmp_packet, sizeof(struct icmp) + packet_size, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr))) <= 0)
    {
        perror("sendto failed\n");
    }
    printf(" %d bytes sent to %s\n", sendbytes, inet_ntoa((struct in_addr)dest_addr.sin_addr));

    free(icmp_packet);
}

/*
void send_ping(ping_probe *probe, struct sockaddr_in dest_addr, int packet_size)
{
    struct icmp *icmp_packet = (struct icmp *)malloc(sizeof(struct icmp) + packet_size);
    icmp_packet->icmp_type = 8;
    icmp_packet->icmp_code = 0;
    icmp_packet->icmp_id = htons(getpid());
    icmp_packet->icmp_seq = htons(probe->ttl);
    memset(icmp_packet->icmp_data, 0xa5, packet_size);
    memset(icmp_packet->icmp_data, 'A', packet_size);
    icmp_packet->icmp_cksum = calculate_checksum((unsigned short *)icmp_packet, sizeof(struct icmp) + packet_size);

    probe->addr = dest_addr;
    int sendbytes;

    // printf("Probe to: %s\n", inet_ntoa((struct in_addr) dest_addr.sin_addr));
    // printf("size of icmp: %d\n", sizeof(struct icmp));
    if ((sendbytes = sendto(probe->sock_fd, icmp_packet, sizeof(struct icmp) + packet_size, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr))) <= 0)
    {
        perror("sendto failed\n");
    }
    printf(" %d bytes sent from %s\n", sendbytes, inet_ntoa((struct in_addr) dest_addr.sin_addr));

    free(icmp_packet);
}
*/
int receive_packet(int sockfd, struct sockaddr_in *addr, char *recv_buf)
{
    int recv_bytes = 0;

    socklen_t len = sizeof(struct sockaddr_in);
    // printf("Probe from: %s\n\n", inet_ntoa((struct in_addr) addr->sin_addr));

    if ((recv_bytes = recvfrom(sockfd, recv_buf, PACKET_SIZE, 0, (struct sockaddr *)addr, &len)) < 0)
    {
        if (errno == EWOULDBLOCK)
        {
            printf("Request timed out.\n");
        }
        else
        {
            perror("recvfrom failed\n");
        }
        // return recv_bytes;
    }

    // // Process ICMP time exceeded response
    // struct iphdr *ip_hdr = (struct iphdr *)recv_buf;
    // int ip_hdr_len = ip_hdr->ihl * 4;
    // struct icmphdr *icmp_hdr1 = (struct icmphdr *)(recv_buf + ip_hdr_len);
    // print_header(icmp_hdr1, recv_bytes);

    // if (icmp_hdr1->type == ICMP_TIME_EXCEEDED && icmp_hdr1->code == ICMP_EXC_TTL)
    // {
    //     // print_header(icmp_hdr1, recv_bytes);
    //     return recv_bytes;
    // }

    // // If destination responds with an ICMP Echo reply, record the response
    // // icmp_hdr1 = (struct icmphdr *)recv_packet;
    // if (icmp_hdr1->type == ICMP_ECHOREPLY && icmp_hdr1->code == 0)
    // {
    //     // print_header(icmp_hdr1, recv_bytes);
    //     return recv_bytes;
    // }

    // print_data(recv_buf, recv_bytes);

    return recv_bytes;
}