#include <sys/types.h>    // socket, bind
#include <sys/socket.h>   // socket, bind, listen, inet_ntoa
#include <netinet/in.h>   // htonl, htons, inet_ntoa
#include <arpa/inet.h>    // inet_ntoa
#include <netdb.h>        // gethostbyname
#include <unistd.h>       // read, write, close
#include <strings.h>      // bzero
#include <netinet/tcp.h>  // SO_REUSEADDR
#include <sys/uio.h>      // writev
#include <iostream>
#include <string>

void send(std::string message) {

}

int main(int argc, char *argv[])
{
  if (argc != 3) {
    std::cerr << "Invalid number of arguments" << std::endl;
    return 1;
  }
   
  int port = std::stoi(argv[1]);
  int repetition = std::stoi(argv[2]);
  int maxConnections = 5;

  sockaddr_in sendSockAddr;
  bzero((char*)&sendSockAddr, sizeof(sendSockAddr));
  sendSockAddr.sin_family = AF_INET;
  sendSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  sendSockAddr.sin_port = htons(port);

  int serverSd = socket(AF_INET, SOCK_STREAM, 0);
  
  const int on = 1;
  setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(int));

  bind(serverSd, (sockaddr *)&sendSockAddr, sizeof(sendSockAddr));

  if (listen(serverSd, maxConnections) == 0) {
    std::cout << "Started server..." << std::endl;
  }
  else {
    std::cerr << "There was an error in starting the server" << std::endl;
    return 1;
  }

  sockaddr_in newSockAddr;
  socklen_t newSockAddrSize = sizeof(newSockAddr);
  int newSd = accept(serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize);

  if (newSd < 0) {
    std::cerr << "Accept failed" << std::endl;
    return 1;
  }
  else {
    std::cout << "New connection" << std::endl;
  }



  return 0;
}