/*!**************************************************************************
 *  \copyright (c) 2016 Sinbest International Trading PTE LTD., all rights reserved
 *
 *  \brief Requirements Covered:
 *   Using poll() instead of select()
 *   The poll() API is part of the Single Unix Specification and the UNIX® 95/98 standard. The poll() API performs the same API as the existing select() API. The only difference between these two APIs is the interface provided to the caller.
 *   The select() API requires that the application pass in an array of bits in which one bit is used to represent each descriptor number. When descriptor numbers are very large, it can overflow the 30KB allocated memory size, forcing multiple iterations of the process. This overhead can adversely affect performance.
 *   The poll() API allows the application to pass an array of structures rather than an array of bits. Because each pollfd structure can contain up to 8 bytes, the application only needs to pass one structure for each descriptor, even if descriptor numbers are very large.
 *   Socket flow of events: Server that uses poll()
 *   The following calls are used in the example:
 *   1. The socket() API returns a socket descriptor, which represents an endpoint. The statement also identifies that the AF_INET (Internet Protocol) address family with the TCP transport (SOCK_STREAM) is used for this socket.
 *   2. The setsockopt() API allows the application to reuse the local address when the server is restarted before the required wait time expires.
 *   3. The ioctl() API sets the socket to be nonblocking. All of the sockets for the incoming connections are also nonblocking because they inherit that state from the listening socket.
 *   4. After the socket descriptor is created, the bind() API gets a unique name for the socket.
 *   5. The listen() API call allows the server to accept incoming client connections.
 *   6. The poll() API allows the process to wait for an event to occur and to wake up the process when the event occurs. The poll() API might return one of the following values.
 *       0
 *   Indicates that the process times out. In this example, the timeout is set for 3 minutes (in milliseconds).
 *       -1
 *   Indicates that the process has failed.
 *        1
 *   Indicates only one descriptor is ready to be processed, which is processed only if it is the listening socket.
 *        1++
 *   Indicates that multiple descriptors are waiting to be processed. The poll() API allows simultaneous connection with all descriptors in the queue on the listening socket.
 *   7. The accept() and recv() APIs are completed when the EWOULDBLOCK is returned.
 *   8. The send() API echoes the data back to the client.
 *   9. The close() API closes any open socket descriptors.
 *
 *
 *  \cond
 *   Revision History:
 *   Date        Author      Description
 *   ------      --------    --------------
 *   15/11/17    Lu Hongao    Created
 *   16/11/17    Lu Hongao    Resolved incoming data trap problem
 *  \endcond
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>

#define SERVER_PORT  12345

#define TRUE             1
#define FALSE            0

#define DEBUG_PRINTF

#ifdef DEBUG_PRINTF
#define DBG(fmt,arg...) printf(fmt,##arg)
#else
#define DBG(fmt,arg...) do{}while(0);
#endif

#define ANYCONN_RECV_BUFFER_SIZE 80

extern void connection_hook(struct pollfd *fds, int status);
extern int switch_hook(int fd, char *buffer, int len);

static struct pollfd fds[200];
static const struct pollfd *fds_list = &fds[0];

main (int argc, char *argv[])
{
  int    len, rc, on = 1; //non-blocking sock
  int    listen_sd = -1, new_sd = -1;
  int    desc_ready, end_server = FALSE, compress_array = FALSE;
  int    close_conn;
  char   buffer[ANYCONN_RECV_BUFFER_SIZE];
  struct sockaddr_in   addr;
  int    timeout;
  int    nfds = 1, current_size = 0, i, j;

  /*************************************************************/
  /* Create an AF_INET stream socket to receive incoming       */
  /* connections on                                            */
  /*************************************************************/
  listen_sd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_sd < 0)
  {
    perror("socket() failed");
    exit(-1);
  }

  /*************************************************************/
  /* Allow socket descriptor to be reuseable                   */
  /*************************************************************/
  rc = setsockopt(listen_sd, SOL_SOCKET,  SO_REUSEADDR,
                  (char *)&on, sizeof(on));
  if (rc < 0)
  {
    perror("setsockopt() failed");
    close(listen_sd);
    exit(-1);
  }

  /*************************************************************/
  /* Set socket to be nonblocking. All of the sockets for    */
  /* the incoming connections will also be nonblocking since  */
  /* they will inherit that state from the listening socket.   */
  /*************************************************************/
  rc = ioctl(listen_sd, FIONBIO, (char *)&on);
  if (rc < 0)
  {
    perror("ioctl() failed");
    close(listen_sd);
    exit(-1);
  }

  /*************************************************************/
  /* Bind the socket                                           */
  /*************************************************************/
  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port        = htons(SERVER_PORT);
  rc = bind(listen_sd,
            (struct sockaddr *)&addr, sizeof(addr));
  if (rc < 0)
  {
    perror("bind() failed");
    close(listen_sd);
    exit(-1);
  }

  /*************************************************************/
  /* Set the listen back log                                   */
  /*************************************************************/
  rc = listen(listen_sd, 32);
  if (rc < 0)
  {
    perror("listen() failed");
    close(listen_sd);
    exit(-1);
  }

  /*************************************************************/
  /* Initialize the pollfd structure                           */
  /*************************************************************/
  memset(fds, 0 , sizeof(fds));

  /*************************************************************/
  /* Set up the initial listening socket                        */
  /*************************************************************/
  fds[0].fd = listen_sd;
  fds[0].events = POLLIN;
  
  //timeout = (3 * 60 * 1000); //3 minutes
  /* Note that the timeout interval will be rounded up to the system clock
     granularity, and kernel scheduling delays mean that the blocking
     interval may overrun by a small amount.  Specifying a negative value
     in timeout means an infinite timeout.  Specifying a timeout of zero
     causes poll() to return immediately, even if no file descriptors are
     ready.
  */
  timeout = -1; //infinite timeout
  
  /*************************************************************/
  /* Loop waiting for incoming connects or for incoming data   */
  /* on any of the connected sockets.                          */
  /*************************************************************/
  do
  {
    DBG("Waiting on poll()...\n");
    rc = poll(fds, nfds, timeout);

    /***********************************************************/
    /* Check to see if the poll call failed.                   */
    /***********************************************************/
    if (rc < 0)
    {
      perror("  poll() failed");
      break;
    }

    /***********************************************************/
    /* Check to see if the 3 minute time out expired.          */
    /***********************************************************/
    if (rc == 0)
    {
      DBG("  poll() timed out.  End program.\n");
      break;
    }


    /***********************************************************/
    /* One or more descriptors are readable.  Need to          */
    /* determine which ones they are.                          */
    /***********************************************************/
    current_size = nfds;
    for (i = 0; i < current_size; i++)
    {
      /*********************************************************/
      /* Loop through to find the descriptors that returned    */
      /* POLLIN and determine whether it's the listening       */
      /* or the active connection.                             */
      /*********************************************************/
      if(fds[i].revents == 0)
        continue;

      /*********************************************************/
      /* If revents is not POLLIN, it's an unexpected result,  */
      /* log and end the server.                               */
      /*********************************************************/
      if(fds[i].revents != POLLIN)
      {
        DBG("  Error! revents = %d\n", fds[i].revents);
        end_server = TRUE;
        break;

      }
      if (fds[i].fd == listen_sd)
      {
        /*******************************************************/
        /* Listening descriptor is readable.                   */
        /*******************************************************/
        DBG("  fds[%04d]:Listening socket is readable\n", i);
        

        /*******************************************************/
        /* Accept all incoming connections that are            */
        /* queued up on the listening socket before we         */
        /* loop back and call poll again.                      */
        /*******************************************************/
        do
        {
          //hongao: here must have a boundary checking and once the max 
          //connection limitaion is reached all new connections will be 
          //rejected but incoming data on old connections will be received
          //as before.
          if(nfds >= sizeof(fds))
          {
            perror("  No more connection allowed!!!\n");
            break;   
          }
          
          /*****************************************************/
          /* Accept each incoming connection. If              */
          /* accept fails with EWOULDBLOCK, then we            */
          /* have accepted all of them. Any other             */
          /* failure on accept will cause us to end the        */
          /* server.                                           */
          /*****************************************************/
          new_sd = accept(listen_sd, NULL, NULL);
          if (new_sd < 0)
          {
          	DBG("%s:%d:rc=%d\n", __FUNCTION__, __LINE__, rc);
            if (errno != EWOULDBLOCK)
            {
              perror("  accept() failed");
              end_server = TRUE;
            }
            break;
          }
          
          //hongao 15/11/2017: set new_sd to be unblocked to avoid the incoming data trap.
          /*************************************************************/
          /* Set socket to be nonblocking. All of the sockets for    */
          /* the incoming connections will also be nonblocking since  */
          /* they will inherit that state from the listening socket.   */
          /*************************************************************/
          rc = ioctl(new_sd, FIONBIO, (char *)&on);
          if (rc < 0)
          {
          	perror("ioctl() failed");
          	close(new_sd);
          	exit(-1);
					}

          /*****************************************************/
          /* Add the new incoming connection to the            */
          /* pollfd structure                                  */
          /*****************************************************/
          DBG("  New incoming connection - %d\n", new_sd);
          fds[nfds].fd = new_sd;
          fds[nfds].events = POLLIN;
          
          //hongao: customize and monitor connections
          //connect
          connection_hook(&fds[nfds], 0);
          
          nfds++;

          /*****************************************************/
          /* Loop back up and accept another incoming          */
          /* connection                                        */
          /*****************************************************/
        } while (new_sd != -1);
      }

      /*********************************************************/
      /* This is not the listening socket, therefore an        */
      /* existing connection must be readable                  */
      /*********************************************************/

      else
      {
        DBG("  fds[%04d]:Descriptor %d is readable\n", i, fds[i].fd);
        close_conn = FALSE;
        /*******************************************************/
        /* Receive all incoming data on this socket            */
        /* before we loop back and call poll again.            */
        /*******************************************************/

        do
        {
          /*****************************************************/
          /* Receive data on this connection until the         */
          /* recv fails with EWOULDBLOCK. If any other        */
          /* failure occurs, we will close the                 */
          /* connection.                                       */
          /*****************************************************/
          rc = recv(fds[i].fd, buffer, sizeof(buffer), 0);
          if (rc < 0)
          {
          	DBG("%s:%d:rc=%d\n", __FUNCTION__, __LINE__, rc);
          	//hongao: this fd must be unblocked!
            if (errno != EWOULDBLOCK)
            {
              perror("  recv() failed");
              close_conn = TRUE;
            }
            break;
          }

          /*****************************************************/
          /* Check to see if the connection has been           */
          /* closed by the client                              */
          /*****************************************************/
          if (rc == 0)
          {
            DBG("  Connection closed\n");
            close_conn = TRUE;
            break;
          }

          /*****************************************************/
          /* Data was received                                 */
          /*****************************************************/
          len = rc;
          DBG("  %d bytes received\n", len);
          
          //hongao: customize routes
          if(-1 == switch_hook(fds[i].fd, buffer, len))
          {
            close_conn = TRUE;
            break;
          }

          /*****************************************************/
          /* Echo the data back to the client                  */
          /*****************************************************/
          #if 0
          rc = send(fds[i].fd, buffer, len, 0);
          if (rc < 0)
          {
            perror("  send() failed");
            close_conn = TRUE;
            break;
          }
          #endif

        } while(TRUE);

        /*******************************************************/
        /* If the close_conn flag was turned on, we need       */
        /* to clean up this active connection. This           */
        /* clean up process includes removing the              */
        /* descriptor.                                         */
        /*******************************************************/
        if (close_conn)
        {
          close(fds[i].fd);
          
          //hongao: customize and monitor connections
          //disconnect
          connection_hook(&fds[nfds], -1);
          
          fds[i].fd = -1;
          compress_array = TRUE;
        }


      }  /* End of existing connection is readable             */
    } /* End of loop through pollable descriptors              */

    /***********************************************************/
    /* If the compress_array flag was turned on, we need       */
    /* to squeeze together the array and decrement the number  */
    /* of file descriptors. We do not need to move back the    */
    /* events and revents fields because the events will always*/
    /* be POLLIN in this case, and revents is output.          */
    /***********************************************************/
    if (compress_array)
    {
      compress_array = FALSE;
      for (i = 0; i < nfds; i++)
      {
        if (fds[i].fd == -1)
        {
          for(j = i; j < nfds; j++)
          {
            fds[j].fd = fds[j+1].fd;
          }
          nfds--;
        }
      }
    }

  } while (end_server == FALSE); /* End of serving running.    */

  /*************************************************************/
  /* Clean up all of the sockets that are open                  */
  /*************************************************************/
  for (i = 0; i < nfds; i++)
  {
    if(fds[i].fd >= 0)
    {
      close(fds[i].fd);
      //hongao: customize and monitor connections
      //disconnect
      connection_hook(&fds[nfds], -1);
    }
  }
}
