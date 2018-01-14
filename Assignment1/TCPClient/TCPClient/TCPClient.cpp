#include <sys/types.h>    // socket, bind
#include <sys/socket.h>   // socket, bind, listen, inet_ntoa
#include <netinet/in.h>   // htonl, htons, inet_ntoa
#include <arpa/inet.h>    // inet_ntoa
#include <netdb.h>        // gethostbyname
#include <unistd.h>       // read, write, close
#include <strings.h>      // bzero
#include <netinet/tcp.h>  // SO_REUSEADDR
#include <sys/uio.h>      // writev
#include <string>
#include <iostream>
#include <sys/time.h>

int main(int argc, char *argv[])
{ 
  if (argc != 7) {
    std::cerr << "Invalid number of arguments" << std::endl;
    std::cerr << "port repetition nbufs bufsize serverIp type" << std::endl;
    return 1;
  }

  int port = atoi(argv[1]),
      repetition = atoi(argv[2]),
      nbufs = atoi(argv[3]),
      bufsize = atoi(argv[4]),
      type = atoi(argv[6]);

  char *serverIp = argv[5];

  struct hostent* host = gethostbyname(serverIp);

  if (host == NULL) {
    std::cerr << "No host" << std::endl;
    return 1;
  }

  sockaddr_in sendSockAddr;
  bzero((char *)&sendSockAddr, sizeof(sendSockAddr));
  sendSockAddr.sin_family = AF_INET;
  sendSockAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
  sendSockAddr.sin_port = htons(port);

  int clientSd = socket(AF_INET, SOCK_STREAM, 0);

  if (clientSd < 0) {
    std::cerr << "Socket error" << std::endl;
    return 1;
  }

  int conn = connect(clientSd, (sockaddr *)&sendSockAddr, sizeof(sendSockAddr));
  if (conn < 0) {
    std::cerr << "Connection error" << std::endl;
    return 1;
  }

  struct timeval start;

  gettimeofday(&start, NULL);
  
  char databuf[nbufs][bufsize];
  int n;
  for (int i = 0; i < repetition; i++) {
    if (type == 1) {
      for (int j = 0; j < nbufs; j++) {
        n = write(clientSd, databuf[j], bufsize);

        if (n < 0) {
          std::cerr << "Error writing to socket" << std::endl;
        }
      }
    }
    else if (type == 2) {
      struct iovec vector[nbufs];

      for (int j = 0; j < nbufs; j++) {
        vector[j].iov_base = databuf[j];
        vector[j].iov_len = bufsize;
      }
      write(clientSd, vector, nbufs);

      if (n < 0) {
        std::cerr << "Error writing to socket" << std::endl;
      }
    }
    else if (type == 3) {
      write(clientSd, databuf, nbufs * bufsize);

      if (n < 0) {
        std::cerr << "Error writing to socket" << std::endl;
      }
    }
  }
  
  int buffer;

  n = read(clientSd, &buffer, sizeof(buffer));
  if (n < 0) {
    std::cerr << "Error reading from socket" << std::endl;
  }

  close(clientSd);

  return 0;
}