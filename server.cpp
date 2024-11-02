#include<iostream>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<tchar.h>
#include<thread>
#include<vector>
#include<algorithm> // Required for std::find
using namespace std;

#pragma comment(lib, "ws2_32.lib")

bool Initialize()
{
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
}

void interactWithClient(SOCKET clientSocket, vector<SOCKET>& clients)
{
    cout << "Client connected" << endl;
    char buffer[4096];

    while (true)
    {
        int bytercvd = recv(clientSocket, buffer, sizeof(buffer) - 1, 0); // Leave space for null-termination
        if (bytercvd <= 0)
        {
            cout << "Client disconnected" << endl;
            break;
        }

        buffer[bytercvd] = '\0'; // Properly terminate the received data

        string message(buffer);
        cout << "Client says: " << message << endl;

        // Broadcast message to all clients except the sender
        for (SOCKET client : clients)
        {
            if (client != clientSocket)
            {
                int bytesSent = send(client, message.c_str(), message.size(), 0);
                if (bytesSent == SOCKET_ERROR)
                {
                    cout << "Failed to send message to a client." << endl;
                }
            }
        }
    }

    // Remove client from the list
    auto it = find(clients.begin(), clients.end(), clientSocket);
    if (it != clients.end())
        clients.erase(it);

    closesocket(clientSocket);
}

int main()
{
    if (!Initialize())
    {
        cout << "Failed to initialize winsock" << endl;
        return -1;
    }
    cout << "Server" << endl;

    SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET)
    {
        cout << "Failed to create socket" << endl;
        WSACleanup();
        return -1;
    }

    // Create address structure
    int port = 12345;
    sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);

    // Convert the IP address (0.0.0.0)
    if (InetPton(AF_INET, _T("0.0.0.0"), &serveraddr.sin_addr) != 1)
    {
        cout << "Failed to convert IP address" << endl;
        closesocket(ListenSocket);
        WSACleanup();
        return -1;
    }

    // Bind the socket to the IP address and port
    if (bind(ListenSocket, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR)
    {
        cout << "Failed to bind the socket" << endl;
        closesocket(ListenSocket);
        WSACleanup();
        return -1;
    }

    // Listen on the socket
    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        cout << "Failed to listen to the socket" << endl;
        closesocket(ListenSocket);
        WSACleanup();
        return -1;
    }
    cout << "Server is listening on port: " << port << endl;

    vector<SOCKET> clients;

    // Accept connections
    while (true)
    {
        SOCKET clientSocket = accept(ListenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET)
        {
            cout << "Failed to accept a connection" << endl;
            closesocket(ListenSocket);
            WSACleanup();
            return -1;
        }

        clients.push_back(clientSocket);
        thread t1(interactWithClient, clientSocket, ref(clients));
        t1.detach(); // Detach the thread to handle multiple clients
    }

    closesocket(ListenSocket);
    WSACleanup();
    return 0;
}
