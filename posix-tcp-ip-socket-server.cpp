#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddress = {};
    serverAddress.sin_family = AF_INET; serverAddress.sin_port = htons(8080); serverAddress.sin_addr.s_addr = INADDR_ANY;
    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    listen(serverSocket, 5);
    int clientSocket = accept(serverSocket, nullptr, nullptr);
    std::string msg = "Hello, client!";
    send(clientSocket, msg.c_str(), msg.size(), 0);
    close(clientSocket); close(serverSocket);
    return 0;
}
