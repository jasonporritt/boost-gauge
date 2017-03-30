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

#define ECU_CAN_ID 0x000007E8U
#define TCU_CAN_ID 0x000007E9U
#define TESTER_CAN_ID 0x000007E0U

#define MANIFOLD_RELATIVE_PRESSURE_DIRECT_ADDRESS 0xFF63B0


#define SSM_ECU_INIT_REQUEST 0xAAU
#define SSM_READ_ADDRESS_REQUEST 0xA8U

#define BUFSIZE 5000 /* size > 4095 to check socket API internal checks */

#define INTERFACE "vcan0"

float format_boost_psi(int boost_reading) {
    return boost_reading * 0.01933677;
}

int main(int argc, char **argv)
{
    int s;
    struct sockaddr_can addr;
    static struct can_isotp_options opts;
    unsigned char request_buffer[BUFSIZE];
    unsigned char response_buffer[BUFSIZE];
    int buflen = 0;
    int nbytes, i;
    int retval = 0;

    addr.can_addr.tp.tx_id = 0x000007E0U;
    addr.can_addr.tp.tx_id |= CAN_EFF_FLAG;
    addr.can_addr.tp.rx_id = 0x000007E8U;
    addr.can_addr.tp.rx_id |= CAN_EFF_FLAG;

    opts.txpad_content = 0x00;
    opts.rxpad_content = 0x00;
    opts.flags |= (CAN_ISOTP_TX_PADDING | CAN_ISOTP_RX_PADDING);

    if ((s = socket(PF_CAN, SOCK_DGRAM, CAN_ISOTP)) < 0) {
	perror("socket");
	exit(1);
    }

    setsockopt(s, SOL_CAN_ISOTP, CAN_ISOTP_OPTS, &opts, sizeof(opts));

    addr.can_family = AF_CAN;
    addr.can_ifindex = if_nametoindex(INTERFACE);

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	perror("bind");
	close(s);
	exit(1);
    }

    request_buffer[0] = SSM_ECU_INIT_REQUEST;
    buflen = 1;

    retval = write(s, request_buffer, buflen);
    if (retval < 0) {
	    perror("write");
	    return retval;
    }

    if (retval != buflen)
	    fprintf(stderr, "wrote only %d from %d byte\n", retval, buflen);

    nbytes = read(s, response_buffer, BUFSIZE);
    if (nbytes > 0 && nbytes < BUFSIZE)
	    for (i=0; i < nbytes; i++)
		    printf("%02X ", response_buffer[i]);
    printf("\n");

    request_buffer[0] = SSM_READ_ADDRESS_REQUEST;
    request_buffer[1] = 0x00U;
    request_buffer[2] = 0xFFU;
    request_buffer[3] = 0x63U;
    request_buffer[4] = 0xB0U;
    buflen = 5;

    // retval = write(s, request_buffer, buflen);
    // if (retval < 0) {
    // 	    perror("write");
    //      return retval;
    // }

    // nbytes = read(s, response_buffer, BUFSIZE);
    // if (nbytes > 0 && nbytes < BUFSIZE)
    //     for (i=0; i < nbytes; i++)
    //         printf("%02X ", response_buffer[i]);
    // printf("\n");

    /* 
     * due to a Kernel internal wait queue the PDU is sent completely
     * before close() returns.
     */
    close(s);

    return 0;
}
