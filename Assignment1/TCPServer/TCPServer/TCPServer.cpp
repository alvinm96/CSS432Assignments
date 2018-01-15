#include <sys/types.h>    // socket, bind
#include <sys/socket.h>   // socket, bind, listen, inet_ntoa
#include <sys/time.h>
#include <netinet/in.h>   // htonl, htons, inet_ntoa
#include <arpa/inet.h>    // inet_ntoa
#include <netdb.h>        // gethostbyname
#include <unistd.h>       // read, write, close
#include <strings.h>      // bzero
#include <netinet/tcp.h>  // SO_REUSEADDR
#include <sys/uio.h>      // writev
#include <pthread.h>
#include <iostream>

int repetition;
int BUFSIZE = 1500;
int maxConnections = 5;

 /*
  pthread function
 */
void *readDatabuf(void *fd) {
  std::cout << "New thread created" << std::endl;

  char databuf[BUFSIZE];
  struct timeval start;
  struct timeval end;

  gettimeofday(&start, NULL); // start timer

  int count = 0;
  
  for (int i = 0; i < repetition; i++) {
    for (int nRead = 0; (nRead += read(*(int *)fd, databuf, BUFSIZE - nRead)) < BUFSIZE; ++count);
  }

  int send = write(*(int *) fd, &count, sizeof(count));
  if (send == -1) {
    std::cerr << "Error writing to client" << std::endl;
  }
  else {
    std::cout << "Wrote " << send << " bytes to the client" << std::endl;
  }

  gettimeofday(&end, NULL); // end timer

  close(*(int *) fd); // close client socket

  long dataReceivingTime = end.tv_usec - start.tv_usec;

  std::cout << "data-receiving time = " << dataReceivingTime << " usec" << std::endl;

  return 0;
}

int main(int argc, char *argv[])
{
  if (argc != 3) {
    std::cerr << "Invalid number of arguments" << std::endl;
    return 1;
  }

  int port = std::stoi(argv[1]);
  repetition = std::stoi(argv[2]);

  // declare sockadddr_in struct
  sockaddr_in sendSockAddr;
  bzero((char*)&sendSockAddr, sizeof(sendSockAddr));
  sendSockAddr.sin_family = AF_INET;
  sendSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  sendSockAddr.sin_port = htons(port);

  // open socket
  int serverSd = socket(AF_INET, SOCK_STREAM, 0);
  
  const int on = 1;
  setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(int));

  // bind socket to local address
  bind(serverSd, (sockaddr *)&sendSockAddr, sizeof(sendSockAddr));

  // begin listening
  if (listen(serverSd, maxConnections) == 0) {
    std::cout << "Started server..." << std::endl;
  }
  else {
    std::cerr << "There was an error in starting the server" << std::endl;
    return 1;
  }

  // create new socket for incoming connections
  sockaddr_in newSockAddr;
  socklen_t newSockAddrSize = sizeof(newSockAddr);

  while (true) {
    std::cout << "Waiting for client connection..." << std::endl;

    int newSd = accept(serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize);

    if (newSd < 0) {
      std::cerr << "Accept failed" << std::endl;
      return 1;
    }
    else {
      std::cout << "New connection" << std::endl;
    }

    // create new p_thread to read data buffer
    pthread_t newThread;
    int threadVal;
    threadVal = pthread_create(&newThread, NULL, readDatabuf, (void*) &newSd);
    if (threadVal) {
      std::cerr << "Error creating new thread" << std::endl;
    }

    pthread_join(newThread, NULL);
  }

  close(serverSd);  // close server socket

  return 0;
}