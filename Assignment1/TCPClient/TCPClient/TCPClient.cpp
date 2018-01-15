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

  if ((nbufs * bufsize) != 1500) {
    std::cerr << "databuf size incorrect; should equal to 1500" << std::endl;
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
  struct timeval lap;
  struct timeval end;

  gettimeofday(&start, NULL); // starting time
  
  char databuf[nbufs][bufsize];

  int n;

  for (int i = 0; i < repetition; i++) {
    if (type == 1) { // multiple writes
      for (int j = 0; j < nbufs; j++) {
        n = write(clientSd, databuf[j], bufsize);
        if (n < 0) {
          std::cerr << "Error writing to socket" << std::endl;
          return 1;
        }
      }
    }
    else if (type == 2) { // writev
      struct iovec vector[nbufs];

      for (int j = 0; j < nbufs; j++) {
        vector[j].iov_base = databuf[j];
        vector[j].iov_len = bufsize;
      }
      n = writev(clientSd, vector, nbufs);

      if (n < 0) {
        std::cerr << "Error writing to socket" << std::endl;
        return 1;
      }
    }
    else if (type == 3) { // single write
      n = write(clientSd, databuf, nbufs * bufsize);

      if (n < 0) {
        std::cerr << "Error writing to socket" << std::endl;
        return 1;
      }
    }
  }
  std::cout << "Finished writing" << std::endl;
  gettimeofday(&lap, NULL); // data sending time

  long lapTime = (lap.tv_usec - start.tv_usec);
  int buffer = 0;

  while (true) {
    n = read(clientSd, &buffer, sizeof(buffer));
    if (n == -1) {
      std::cerr << "Error reading from socket" << std::endl;
    }
    else if (n == 0) {
      std::cout << "Finished reading from server" << std::endl;
      break;
    }
  }
  gettimeofday(&end, NULL); // round trip time

  long roundTripTime = end.tv_usec - start.tv_usec;

  std::cout << "Test " << type << ": data-sending time = " << 
    lapTime << "usec, round-trip time = " << 
    roundTripTime << "usec, #reads = " << buffer << std::endl;

  close(clientSd);

  return 0;
}