/*
 * @(#)checkOW.c	1.5 93/08/10

 * Check to see if there is a server already running. We should not be 
 * able to bind to a socket which is already in use. dpynum is the 
 * display to check. Expects to be called from inside openwin, and
 * returns exit status: 1 for success and -1 for faliure.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>

#define TCPPORT 6000
#define UNIX_SOCKET_PATH "/tmp/.X11-unix/X"

TestTCP(int dpynum)
{
  int fd;

  if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) > 0) {
    struct sockaddr_in addr;
    char       *hostname;
    long        strtol();
    int mi = 1;
    
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&mi, sizeof(int));
    /* Must be a TCP port */
    memset((char *) &addr, 0, sizeof(addr) );
    addr.sin_family = AF_INET;
    addr.sin_port = htons (TCPPORT+dpynum);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (struct sockaddr *) &addr, sizeof addr) < 0)
      return -1;
  }
  return 1;
}

int
open_socket(char *name)
{
  register char *p = name;
  register    fd = -1;
  struct sockaddr_un unsock;
  int         namelen = strlen(p);
   
  unsock.sun_family = AF_UNIX;
  if (namelen >= sizeof(unsock.sun_path))
    return -1;
  (void) strcpy(unsock.sun_path, p);
  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    /* If we can't create a socket, does not mean anything */
    return 0;
 
  if (access(p, F_OK) == 0) {
    int err, sock;
    char        foo[1];
 
#ifdef SYSV
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (!connect(sock,(struct sockaddr *)&unsock,namelen+2)) {
      errno = EADDRINUSE;
      return -1;
    }
    (void) unlink(unsock.sun_path);
    (void) close(sock);
#else
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    err = sendto(sock, foo, 1, 0, &unsock,
                 sizeof(struct sockaddr_un));
    if (err)    {
      switch(errno)     {
      case ECONNREFUSED:
        (void) unlink(unsock.sun_path);
        (void) close(sock);
        break;
      case EPROTOTYPE:
      default:
        (void) close(sock);
        errno = EADDRINUSE;
        return -1;
      }
    }
#endif SYSV
  }
 
  return 1;
}

main(int argc, char *argv[])
{

  char sock_name[40], *p;
  int dpynum;
  
  if (argc < 2)
    exit(-1);
  sscanf(argv[1],":%d",&dpynum);

  sprintf(sock_name, "%s%d", UNIX_SOCKET_PATH, dpynum);
  if (open_socket(sock_name) == -1)
      exit(-1);
  else if (TestTCP(dpynum) == -1)
      exit(-1);
  else
      exit(1);
}
