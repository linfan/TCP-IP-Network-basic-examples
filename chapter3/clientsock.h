#ifndef _CLIENT_SOCK_H_
#define _CLIENT_SOCK_H_

#define DEFARG(name, defval) ((#name[0]) ? (name + 0) : defval)
#define clientsock(host, service, transport, sockaddr_in) \
    _clientsock(host, service, transport, DEFARG(sockaddr_in, NULL))

#endif
