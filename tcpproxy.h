#define __tcpproxy_h__

#define BACKLOG     1024
#define BUFLEN      255

// Struct that saves the amount read from the host and the client
typedef struct {
  int nreadHost;
  int nreadClient;
  int nwrittenHost;
  int nwrittenClient;
} ProxyBuffers;

// Struct that saves the file descriptors for the host, client, and (proxy) server
typedef struct {
  int host;
  int client;
  int server;
} FileDescriptors;

// Struct that saves the user input
typedef struct {
  char *remote_host;
  char *remote_port;
  char *proxy_server_port;
} Ports;

// Struct holding the buffer for the items read from the client and host
typedef struct {
  char readClient[BUFLEN];
  char readHost[BUFLEN];
} Buffers;

// Reads information from the client
int readClient(FileDescriptors fds, Buffers *buf, struct pollfd pollfds[2], ProxyBuffers *proxy_buffers);

// Writes information to the host
int writeHost(FileDescriptors fds, Buffers *buf, struct pollfd pollfds[2], ProxyBuffers *proxy_buffers);

// Read information from the host
int readHost(FileDescriptors fds, Buffers *buf, struct pollfd pollfds[2], ProxyBuffers *proxy_buffers);

// Write information to the client
int writeClient(FileDescriptors fds, Buffers *buf, struct pollfd pollfds[2], ProxyBuffers *proxy_buffers);

// Makes a socket async
int make_async(int s);
