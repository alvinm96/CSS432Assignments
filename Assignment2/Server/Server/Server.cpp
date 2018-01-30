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
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>

int maxConnections = 5;

static const std::map<int, std::string> STATUS_CODES = {
  { 200, "200 OK" },
  { 400, "400 Bad Request" },
  { 401, "401 Unauthorized" },
  { 403, "403 Forbidden" },
  { 404, "404 Not Found" },
  { 501, "501 Not Implemented" }
};

/*
  creates the response message
*/
void createResponse(std::string &message, int statusCode, std::string body) {
  message = "HTTP/1.1 " + STATUS_CODES.at(statusCode); // status
  message += "\r\nConnection: close";
  message += "\r\n"; // end headers
  message += "\r\n" + body;
  message += "\r\n\r\n";
}

/*
  handles the request
*/
void *handleRequest(void *fd) {
  std::string messageBody;
  std::string messageResponse;
  char messageBuffer[1024];
  int messageSize = 0;

  std::cout << "Created new thread" << std::endl;
  int socket = *(int *)fd;

  if ((messageSize = recv(socket, messageBuffer, sizeof(messageBuffer), 0)) == -1) {
    std::cerr << "Error receiving socket" << std::endl;
  }

  messageBuffer[messageSize] = '\0';
  std::istringstream iss(messageBuffer);
  std::vector<std::string> tokens;
  std::string token;

  while (std::getline(iss, token, ' ')) {
    tokens.push_back(token);
  }

  if (tokens[0] == "GET") {
    std::ifstream inFile;
    std::string fileName = tokens[1];
    std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);
    std::cout << "Client attempting to GET " << fileName << std::endl;

    if (fileName.substr(0, 4) == "./..") { // return 403
      createResponse(messageResponse, 403, messageBody);
    }
    else if (fileName == "./secretfile.html") { // return 401
      createResponse(messageResponse, 401, messageBody);
    }
    else {
      inFile.open(fileName);

      if (!inFile) { // return 404
        createResponse(messageResponse, 404, messageBody);
      }
      else { // return 200
        std::string line;
        while (std::getline(inFile, line)) {
          messageBody += line + "\n";
        }
        createResponse(messageResponse, 200, messageBody);

        inFile.close();
      }
    }
  }
  else if (tokens[0] == "POST" || tokens[0] == "PUT" || tokens[0] == "DELETE") { // not implemented
    createResponse(messageResponse, 501, messageBody);
  }
  else { // return 400
    createResponse(messageResponse, 400, messageBody);
  }

  //std::cout << messageResponse << std::endl;
  // return response message
  if (send(socket, messageResponse.c_str(), messageResponse.length(), 0) == -1) {
    std::cerr << "Error sending file contents" << std::endl;
  }

  close(socket);
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Invalid number of arguments" << std::endl;
    return 1;
  }

  int port = std::stoi(argv[1]);
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
    std::cout << "Started server" << std::endl;
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

    // create new pthread to handle request
    pthread_t newThread;
    int threadVal;
    threadVal = pthread_create(&newThread, NULL, handleRequest, (void*) &newSd);

    pthread_join(newThread, NULL);
  }

  close(serverSd);  // close server socket

  return 0;
}