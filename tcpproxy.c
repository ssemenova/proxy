#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include "utils.h"
#include "sockutils.h"
#include "tcpproxy.h"

int main(int argc, char **argv) {
  // Print out usage instructions if not enough arguments passed
  if (argc < 4) {
    printf("Usage: tcpproxy remote_host remote_port proxy_server_port");
    return 1;
  }

  // Save user input into Ports struct
  // Create FileDescriptors struct to save client, host, and proxy server file descriptors
  Ports userInputPorts;
  memset(&userInputPorts, 0, sizeof(Ports));
  userInputPorts.remote_host = argv[1];
  userInputPorts.remote_port = argv[2];
  userInputPorts.proxy_server_port = argv[3];
  FileDescriptors fds;

  // Create struct that contains 2 empty buffers
  // for the information read from the client and host servers
  Buffers buf;
  memset(&buf, 0, sizeof(Buffers));
  memset(buf.readClient, '\0', BUFLEN);
  memset(buf.readHost, '\0', BUFLEN);

  // Create ProxyBuffers struct to keep track of amount written and read on each remote server
  ProxyBuffers proxy_buffers;
  proxy_buffers.nreadClient = 0;
  proxy_buffers.nwrittenClient = 0;
  proxy_buffers.nreadHost = 0;
  proxy_buffers.nwrittenHost = 0;

  // Make proxy server and create addrinfo object (from the provided code)
  if ((fds.server = make_server(userInputPorts.proxy_server_port, BACKLOG)) < 0)
      die("Cannot listen on port %s\n", userInputPorts.proxy_server_port);
  struct addrinfo *info = make_addrinfo(userInputPorts.remote_host, userInputPorts.remote_port);

  // Start listening for clients
  while (1) {
    // Connect to host and server and make both async
    if ((fds.client = server_accept(fds.server)) < 0)
      die("Error accepting client");
    fds.host = host_connect(info);
    make_async(fds.host);
    make_async(fds.client);

    // Create polls struct
    struct pollfd pollfds[2];
    pollfds[0].fd = fds.client;
    pollfds[0].events = pollfds[0].revents = 0;
    pollfds[1].fd = fds.host;
    pollfds[1].events = pollfds[1].revents = 0;

    // Set intent in the events member of each pollfd struct
    pollfds[0].events |= (POLLIN | POLLOUT);
    pollfds[1].events |= (POLLIN | POLLOUT);
    while (1) {
      if (poll(pollfds, 2, -1) < 0)
          pdie("poll");
      if (pollfds[0].revents & POLLIN)
        // If there is an error, break out of the loop and disconnect
        if (readClient(fds, &buf, pollfds, &proxy_buffers) < 0)
          break;
      if (pollfds[0].revents & POLLOUT)
        // If there is an error, break out of the loop and disconnect
        if (writeClient(fds, &buf, pollfds, &proxy_buffers) < 0)
          break;
      if (pollfds[1].revents & POLLIN) {
        int readResults = readHost(fds, &buf, pollfds, &proxy_buffers);
        // If there is an error, break out of the loop and disconnect
        if (readResults < 0)
          break;
        // If nothing was read and the host is no longer waiting to send information,
        // we have completed the cycle, so we break and disconnect
        else if (readResults == 0 && (pollfds[1].revents & ~POLLOUT))
          break;
      }
      if (pollfds[1].revents & POLLOUT)
        // If there is an error, break out of the loop and disconnect
        if (writeHost(fds, &buf, pollfds, &proxy_buffers) < 0)
          break;
    }

    // close everything and reset values for the buffers and the amount read from each remote server
    close(fds.client);
    close(fds.host);
    memset(buf.readClient, '\0', BUFLEN);
    memset(buf.readHost, '\0', BUFLEN);
    proxy_buffers.nreadClient = 0;
    proxy_buffers.nreadHost = 0;
    proxy_buffers.nwrittenClient = 0;
    proxy_buffers.nwrittenHost = 0;
  }

  free_addrinfo(info);
  return 0;
}

// Reads from the client
int readClient(FileDescriptors fds, Buffers *buf, struct pollfd pollfds[2], ProxyBuffers *proxy_buffers) {
  int currentRead;
  if ((currentRead = read(pollfds[0].fd, buf->readClient + proxy_buffers->nreadClient, BUFLEN - proxy_buffers->nreadClient)) > 0) {
    proxy_buffers->nreadClient += currentRead;
  }
  if (currentRead < 0) {
    fprintf(stderr, "Could not read from client: %d\n", errno);
    return -1;
  }
  return 1;
}

// Reads from the host
int readHost(FileDescriptors fds, Buffers *buf, struct pollfd pollfds[2], ProxyBuffers *proxy_buffers) {
  int currentRead;
  if ((currentRead = read(pollfds[1].fd, buf->readHost + proxy_buffers->nreadHost, BUFLEN - proxy_buffers->nreadHost)) > 0 &&
      proxy_buffers->nreadHost < BUFLEN - 1) {
    proxy_buffers->nreadHost += currentRead;
    return 1;
  }
  if (currentRead < 0) {
    fprintf(stderr, "Could not read from server\n");
    return -1;
  }
  return 0;
}

// Writes to the client
int writeClient(FileDescriptors fds, Buffers *buf, struct pollfd pollfds[2], ProxyBuffers *proxy_buffers) {
  int currentWritten;
  while ((pollfds[0].revents & POLLOUT) &&
        (currentWritten = write(pollfds[0].fd, buf->readHost, proxy_buffers->nreadHost)) > 0 &&
        proxy_buffers->nwrittenClient < proxy_buffers->nreadHost) {
    proxy_buffers->nwrittenClient += currentWritten;
  }
  if (currentWritten < 0) {
    fprintf(stderr, "Could not write to client\n");
    return -1;
  }
  memset(buf->readHost, '\0', BUFLEN);
  proxy_buffers->nreadHost = 0;
  return 1;
}

// Writes to the host
int writeHost(FileDescriptors fds, Buffers *buf, struct pollfd pollfds[2], ProxyBuffers *proxy_buffers) {
  int currentWritten;
  while ((pollfds[1].revents & POLLOUT) &&
        (currentWritten = write(pollfds[1].fd, buf->readClient, proxy_buffers->nreadClient)) > 0 &&
        proxy_buffers->nwrittenHost < proxy_buffers->nreadClient) {
    proxy_buffers->nwrittenHost += currentWritten;
  }
  if (currentWritten < 0) {
    fprintf(stderr, "Could not write to server: %d\n", errno);
    return -1;
  }
  return 1;
}

// Make a port async
int make_async(int s) {
    int n;
    if((n = fcntl(s, F_GETFL)) == -1 || fcntl(s, F_SETFL, n | O_NONBLOCK) == -1)
        perr("fcntl");
    n = 1;
    if(setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &n, sizeof(n)) == -1)
        perr("setsockopt");
    return 0;
  err:
    return -1;
}
