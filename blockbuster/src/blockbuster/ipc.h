#ifndef BLOCKBUSTER_IPC_H
#define BLOCKBUSTER_IPC_H

#define DEFAULT_MASTER_PORT 7011

/* Message passing functions from ipc.c */
extern int MyHostName(char *nameOut, int maxNameLength);
extern int CreatePort(int *port);
extern int AcceptConnection(int socket);
extern int Connect(const char *hostname, int port);
extern int SendData(int socket, const void *data, int bytes);
extern int ReceiveData(int socket, void *data, int bytes);
extern int SendString(int socket, const char *str);
extern int ReceiveString(int socket, char *str, int maxLen);

#endif
