#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <linux/can.h>
#include <linux/can/isotp.h>

#define BUFSIZE 5000
#define ECU_CAN_ID 0x000007E8
#define TCU_CAN_ID 0x000007E9
#define TESTER_CAN_ID 0x000007E0

#define ECU_INIT_DATA = 0xAA

int main()
{
  struct sockaddr_can addr;
  static struct can_isotp_options opts;
  // static struct can_isotp_fc_options fcopts;

  int s;
  unsigned char msg[BUFSIZE];
  int numbytes;

  unsigned char ecu_init_cmd = {ECU_INIT_DATA};

  addr.can_addr.tp.tx_id = ECU_CAN_ID;
  addr.can_addr.tp.tx_id |= CAN_EFF_FLAG;

  addr.can_addr.tp.rx_id = TESTER_CAN_ID;
  addr.can_addr.tp.xx_id |= CAN_EFF_FLAG;

  if ((s = socket(PF_CAN, SOCK_DGRAM, CAN_ISOTP)) < 0) {
    perror("socket");
    exit(1);
  }

  opts.flags |= (CAN_ISOTP_TX_PADDING | CAN_ISOTP_RX_PADDING);
  setsockopt(s, SOL_CAN_ISOTP, CAN_ISOTP_OPTS, &opts, sizeof(opts));

  addr.can_family = AF_CAN;
  addr.can_ifindex = if_nametoindex("can0");

  if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    close(s);
    exit(1);
  }

  retval = write(s, ecu_init_cmd, 1);
  if (retval < 0) {
    perror("write");
    return retval;
  }

  nbytes = read(s, msg, BUFSIZE);
  if (nbytes > 0 && nbytes < BUFSIZE)
    for (i=0; i < nbytes; i++)
      printf("%02X ", msg[i]);
  printf("\n");

  close(s);

}
