/*
 * tcp_con.c
 *
 *  Created on: 28 mai 2011
 *      Author: Matlo
 *
 *  License: GPLv3
 */
#ifndef WIN32
#include <sys/ioctl.h>
#include <err.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pwd.h>
#else
#include <winsock2.h>
#define MSG_DONTWAIT 0
#endif
#include "config.h"
#include "dump.h"

/*
 * Controller are listening from TCP_PORT to TCP_PORT+MAX_CONTROLLERS-1
 */
#define TCP_PORT 21313

/*
 * Sockets to controllers.
 */
static int sockfd[MAX_CONTROLLERS];

extern s_controller controller[MAX_CONTROLLERS];
extern struct sixaxis_state state[MAX_CONTROLLERS];
extern int display;

/*
 * Connect to all responding controllers.
 */
int tcp_connect(void)
{
    int fd;
    int i;
    int ret = -1;
    struct sockaddr_in addr;

    for(i=0; i<MAX_CONTROLLERS; ++i)
    {
#ifdef WIN32
      WSADATA wsadata;

      if (WSAStartup(MAKEWORD(1,1), &wsadata) == SOCKET_ERROR)
        err(1, "WSAStartup");

      if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err(1, "socket");
#else
      if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        err(1, "socket");
#endif
      memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(TCP_PORT+i);
#ifdef WIN32
      addr.sin_addr.s_addr = inet_addr(ip);
#else
      addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#endif
      if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
      {
        fd = 0;
      }
      else
      {
        ret = 1;
        printf("connected to emu %d\n", i);
      }

#ifdef WIN32
      // Set the socket I/O mode; iMode = 0 for blocking; iMode != 0 for non-blocking
      int iMode = 1;
      ioctlsocket(fd, FIONBIO, (u_long FAR*) &iMode);
#endif

      sockfd[i] = fd;

    }

    return ret;
}


int send_single(int c_id, const char* buf, int length)
{
  if(!sockfd[c_id]) return 0;

  return send(sockfd[c_id], buf, length, MSG_DONTWAIT);
}

/*
 * Send a command to each controller that has its status changed.
 */
void tcp_send(int force_update)
{
  int i;
  unsigned char buf[48];

  for (i = 0; i < MAX_CONTROLLERS; ++i)
  {
    if (force_update || controller[i].send_command)
    {
      if (assemble_input_01(buf, sizeof(buf), state + i) < 0)
      {
        printf("can't assemble\n");
      }
      send_single(i, (const char*)buf, 48);

      if (controller[i].send_command)
      {
        if(display)
        {
          sixaxis_dump_state(state + i, i);
        }

        controller[i].send_command = 0;
      }
    }
  }
}
