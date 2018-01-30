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
#include <fstream>
#include <sys/time.h>
#include <sstream>
#include <vector>

void createRequest(std::string &message, std::string pathToFile, std::string serverHost) 
{
  message = "GET ." + pathToFile + " HTTP/1.1";
  message += "\r\nHost: " + serverHost;
  message += "\r\n\r\n";
}

void handleResponse(std::vector<std::string> response, std::string fileName) {
  std::istringstream iss(response[0]); // first line should be status header
  std::vector<std::string> tokens;
  std::string token;

  while (std::getline(iss, token, ' ')) {
    tokens.push_back(token);
  }
  
  // second element should be the status code
  if (tokens[1] == "200") {

    if (fileName != "") {
      std::ofstream file(fileName);

      for (int i = 1; i < response.size(); i++) {
        std::cout << response[i] << std::endl;
        file << response[i] << std::endl;
      }
      std::cout << "Done writing to file" << std::endl;
    }
  }
  else { // print the error
    std::cout << response[0] << std::endl;
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Invalid number of arguments" << std::endl;
    return 1;
  }

  char* serverIp = argv[1]; // server
  std::string fileName;

  std::istringstream requestStream(serverIp);
  std::vector<std::string> requestDetails;
  std::string token;

  while (std::getline(requestStream, token, '/')) {
    requestDetails.push_back(token);
  }

  for (int i = 1; i < requestDetails.size(); i++) {
    fileName += "/" + requestDetails[i];
  }

  std::istringstream urlStream(requestDetails[0]);
  std::vector<std::string> urlDetails;

  while (std::getline(urlStream, token, ':')) {
    urlDetails.push_back(token);
  }

  struct hostent *host = gethostbyname(urlDetails[0].c_str());

  if (host == NULL) {
    std::cerr << "No host" << std::endl;
    return 1;
  }

  sockaddr_in sendSockAddr;
  bzero((char *)&sendSockAddr, sizeof(sendSockAddr));
  sendSockAddr.sin_family = AF_INET;
  sendSockAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*) *host->h_addr_list));
  sendSockAddr.sin_port = htons(stoi(urlDetails[1]));

  
  int clientSd;
  int conn;

  if ((clientSd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    std::cerr << "Socket error" << std::endl;
    return 1;
  }

  if ((conn = connect(clientSd, (sockaddr *)&sendSockAddr, sizeof(sendSockAddr))) < 0) {
    std::cerr << "Connection error" << std::endl;
    return 1;
  }
  else {
    std::cout << "Connected to " << requestDetails[0].c_str() << std::endl;
  }

  
  std::string request = "";
  createRequest(request, fileName, urlDetails[0].c_str());
  std::cout << request << std::endl;

  conn = send(clientSd, request.c_str(), sizeof(request), 0);
  if (conn == -1) {
    std::cerr << "Error sending request" << std::endl;
  }

  char responseBuffer[1024];
  int responseSize = 0;
  std::string response;

  while (true) {
    if ((responseSize = recv(clientSd, responseBuffer, sizeof(responseBuffer), 0)) == -1) {
      std::cerr << "Received error" << std::endl;
    }
    response = responseBuffer;

    if (responseSize == 0) {
      break;
    }
  }

  std::istringstream iss(response);
  std::vector<std::string> lines;

  while (std::getline(iss, token)) {
    lines.push_back(token);
  }

  handleResponse(lines, fileName);
  
  return 0;
}