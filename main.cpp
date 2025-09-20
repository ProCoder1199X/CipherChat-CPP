#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>
#include <iomanip>
#include <ctime>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define SOCKET_T SOCKET
    #define INVALID_SOCKET_VAL INVALID_SOCKET
    #define SOCKET_ERROR_VAL SOCKET_ERROR
    #define close_socket closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #define SOCKET_T int
    #define INVALID_SOCKET_VAL -1
    #define SOCKET_ERROR_VAL -1
    #define close_socket close
#endif

// Simple RSA implementation for demonstration (not cryptographically secure)
class SimpleRSA {
private:
    long long n, e, d;
    
    long long gcd(long long a, long long b) {
        if (b == 0) return a;
        return gcd(b, a % b);
    }
    
    long long modPow(long long base, long long exp, long long mod) {
        long long result = 1;
        base = base % mod;
        while (exp > 0) {
            if (exp % 2 == 1)
                result = (result * base) % mod;
            exp = exp >> 1;
            base = (base * base) % mod;
        }
        return result;
    }
    
    long long modInverse(long long a, long long m) {
        for (long long x = 1; x < m; x++) {
            if (((a % m) * (x % m)) % m == 1)
                return x;
        }
        return 1;
    }
    
public:
    void generateKeys() {
        // Small primes for demonstration (use large primes in production)
        long long p = 61, q = 53;
        n = p * q;
        long long phi = (p - 1) * (q - 1);
        e = 17; // Common choice for e
        d = modInverse(e, phi);
    }
    
    std::pair<long long, long long> getPublicKey() { return {n, e}; }
    std::pair<long long, long long> getPrivateKey() { return {n, d}; }
    
    std::vector<long long> encrypt(const std::string& message, long long pub_n, long long pub_e) {
        std::vector<long long> encrypted;
        for (char c : message) {
            encrypted.push_back(modPow(static_cast<long long>(c), pub_e, pub_n));
        }
        return encrypted;
    }
    
    std::string decrypt(const std::vector<long long>& encrypted) {
        std::string decrypted;
        for (long long c : encrypted) {
            decrypted += static_cast<char>(modPow(c, d, n));
        }
        return decrypted;
    }
};

// Simple AES-like encryption for session keys
class SimpleCipher {
private:
    std::string key;
    
public:
    SimpleCipher(const std::string& k) : key(k) {}
    
    std::string encrypt(const std::string& plaintext) {
        std::string encrypted = plaintext;
        for (size_t i = 0; i < encrypted.length(); i++) {
            encrypted[i] ^= key[i % key.length()];
            encrypted[i] = ((encrypted[i] + 13) % 256);
        }
        return encrypted;
    }
    
    std::string decrypt(const std::string& ciphertext) {
        std::string decrypted = ciphertext;
        for (size_t i = 0; i < decrypted.length(); i++) {
            decrypted[i] = ((decrypted[i] - 13 + 256) % 256);
            decrypted[i] ^= key[i % key.length()];
        }
        return decrypted;
    }
};

// Message structure
struct Message {
    std::string sender;
    std::string content;
    std::chrono::system_clock::time_point timestamp;
    bool encrypted;
};

// User class
class User {
public:
    std::string username;
    std::pair<long long, long long> publicKey;
    SOCKET_T socket;
    bool connected;
    
    User(const std::string& name, SOCKET_T sock) 
        : username(name), socket(sock), connected(true) {}
};

// Chat Room class
class ChatRoom {
private:
    std::string roomName;
    std::vector<User*> users;
    std::vector<Message> messageHistory;
    std::mutex roomMutex;
    
public:
    ChatRoom(const std::string& name) : roomName(name) {}
    
    void addUser(User* user) {
        std::lock_guard<std::mutex> lock(roomMutex);
        users.push_back(user);
        std::cout << "[" << roomName << "] " << user->username << " joined the room." << std::endl;
    }
    
    void removeUser(User* user) {
        std::lock_guard<std::mutex> lock(roomMutex);
        users.erase(std::remove(users.begin(), users.end(), user), users.end());
        std::cout << "[" << roomName << "] " << user->username << " left the room." << std::endl;
    }
    
    void broadcastMessage(const Message& msg, User* sender) {
        std::lock_guard<std::mutex> lock(roomMutex);
        messageHistory.push_back(msg);
        
        for (User* user : users) {
            if (user != sender && user->connected) {
                std::string formattedMsg = formatMessage(msg);
                send(user->socket, formattedMsg.c_str(), formattedMsg.length(), 0);
            }
        }
    }
    
    std::vector<User*> getUsers() {
        std::lock_guard<std::mutex> lock(roomMutex);
        return users;
    }
    
    std::string getRoomName() const { return roomName; }
    
private:
    std::string formatMessage(const Message& msg) {
        auto time_t = std::chrono::system_clock::to_time_t(msg.timestamp);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
        return "[" + ss.str() + "] " + msg.sender + ": " + msg.content + "\n";
    }
};

// CipherChat Server
class CipherChatServer {
private:
    SOCKET_T serverSocket;
    std::vector<ChatRoom*> chatRooms;
    std::map<SOCKET_T, User*> connectedUsers;
    std::mutex serverMutex;
    bool running;
    
    void initializeWinsock() {
#ifdef _WIN32
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "WSAStartup failed: " << result << std::endl;
            exit(1);
        }
#endif
    }
    
    void cleanupWinsock() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
    
public:
    CipherChatServer() : running(false) {
        initializeWinsock();
        chatRooms.push_back(new ChatRoom("General"));
        chatRooms.push_back(new ChatRoom("Secure"));
    }
    
    ~CipherChatServer() {
        stop();
        for (auto room : chatRooms) {
            delete room;
        }
        cleanupWinsock();
    }
    
    bool start(int port) {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET_VAL) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }
        
        // Allow socket reuse
        int opt = 1;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, 
                      reinterpret_cast<const char*>(&opt), sizeof(opt)) < 0) {
            std::cerr << "Failed to set socket options" << std::endl;
            return false;
        }
        
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        
        if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), 
                sizeof(serverAddr)) == SOCKET_ERROR_VAL) {
            std::cerr << "Failed to bind socket" << std::endl;
            return false;
        }
        
        if (listen(serverSocket, 10) == SOCKET_ERROR_VAL) {
            std::cerr << "Failed to listen on socket" << std::endl;
            return false;
        }
        
        running = true;
        std::cout << "CipherChat Server started on port " << port << std::endl;
        std::cout << "Available rooms: ";
        for (const auto& room : chatRooms) {
            std::cout << room->getRoomName() << " ";
        }
        std::cout << std::endl;
        
        // Accept connections in a separate thread
        std::thread acceptThread(&CipherChatServer::acceptConnections, this);
        acceptThread.detach();
        
        return true;
    }
    
    void stop() {
        running = false;
        if (serverSocket != INVALID_SOCKET_VAL) {
            close_socket(serverSocket);
            serverSocket = INVALID_SOCKET_VAL;
        }
    }
    
private:
    void acceptConnections() {
        while (running) {
            sockaddr_in clientAddr{};
            socklen_t clientLen = sizeof(clientAddr);
            
            SOCKET_T clientSocket = accept(serverSocket, 
                                         reinterpret_cast<sockaddr*>(&clientAddr), 
                                         &clientLen);
            
            if (clientSocket != INVALID_SOCKET_VAL) {
                std::thread clientThread(&CipherChatServer::handleClient, this, clientSocket);
                clientThread.detach();
            }
        }
    }
    
    void handleClient(SOCKET_T clientSocket) {
        char buffer[1024];
        
        // Receive username
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            close_socket(clientSocket);
            return;
        }
        
        buffer[bytesReceived] = '\0';
        std::string username(buffer);
        
        User* user = new User(username, clientSocket);
        
        {
            std::lock_guard<std::mutex> lock(serverMutex);
            connectedUsers[clientSocket] = user;
        }
        
        // Add user to General room by default
        chatRooms[0]->addUser(user);
        
        // Send welcome message
        std::string welcome = "Welcome to CipherChat, " + username + "!\n";
        welcome += "Available commands:\n";
        welcome += "/join <room> - Join a chat room\n";
        welcome += "/users - List users in current room\n";
        welcome += "/encrypt <message> - Send encrypted message\n";
        welcome += "/quit - Leave the chat\n\n";
        send(clientSocket, welcome.c_str(), welcome.length(), 0);
        
        // Handle client messages
        while (running && user->connected) {
            bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived <= 0) {
                break;
            }
            
            buffer[bytesReceived] = '\0';
            std::string messageContent(buffer);
            
            if (messageContent.empty()) continue;
            
            // Remove newline if present
            if (messageContent.back() == '\n') {
                messageContent.pop_back();
            }
            
            processMessage(user, messageContent);
        }
        
        // Cleanup
        user->connected = false;
        
        // Remove from all rooms
        for (auto room : chatRooms) {
            room->removeUser(user);
        }
        
        {
            std::lock_guard<std::mutex> lock(serverMutex);
            connectedUsers.erase(clientSocket);
        }
        
        close_socket(clientSocket);
        delete user;
    }
    
    void processMessage(User* user, const std::string& messageContent) {
        if (messageContent.empty()) return;
        
        if (messageContent[0] == '/') {
            handleCommand(user, messageContent);
        } else {
            // Regular message - broadcast to current room
            Message msg;
            msg.sender = user->username;
            msg.content = messageContent;
            msg.timestamp = std::chrono::system_clock::now();
            msg.encrypted = false;
            
            // Find user's current room (assume General for now)
            chatRooms[0]->broadcastMessage(msg, user);
            
            // Echo back to sender
            auto time_t = std::chrono::system_clock::to_time_t(msg.timestamp);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
            std::string echo = "[" + ss.str() + "] You: " + messageContent + "\n";
            send(user->socket, echo.c_str(), echo.length(), 0);
        }
    }
    
    void handleCommand(User* user, const std::string& command) {
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "/quit") {
            user->connected = false;
            std::string goodbye = "Goodbye, " + user->username + "!\n";
            send(user->socket, goodbye.c_str(), goodbye.length(), 0);
        }
        else if (cmd == "/users") {
            std::string userList = "Users in room:\n";
            for (const auto& u : chatRooms[0]->getUsers()) {
                userList += "- " + u->username + "\n";
            }
            send(user->socket, userList.c_str(), userList.length(), 0);
        }
        else if (cmd == "/encrypt") {
            std::string encryptedMsg;
            std::getline(iss, encryptedMsg);
            if (!encryptedMsg.empty() && encryptedMsg[0] == ' ') {
                encryptedMsg = encryptedMsg.substr(1);
            }
            
            // Simple encryption demonstration
            SimpleCipher cipher("CipherChatKey123");
            std::string encrypted = cipher.encrypt(encryptedMsg);
            
            Message msg;
            msg.sender = user->username + " [ENCRYPTED]";
            msg.content = "[ENCRYPTED MESSAGE]";
            msg.timestamp = std::chrono::system_clock::now();
            msg.encrypted = true;
            
            chatRooms[0]->broadcastMessage(msg, user);
            
            // Send confirmation to sender
            std::string confirm = "Encrypted message sent: " + encryptedMsg + "\n";
            send(user->socket, confirm.c_str(), confirm.length(), 0);
        }
        else {
            std::string error = "Unknown command: " + cmd + "\n";
            send(user->socket, error.c_str(), error.length(), 0);
        }
    }
};

// CipherChat Client
class CipherChatClient {
private:
    SOCKET_T clientSocket;
    std::string username;
    bool connected;
    std::thread receiveThread;
    SimpleRSA rsa;
    
    void initializeWinsock() {
#ifdef _WIN32
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "WSAStartup failed: " << result << std::endl;
            exit(1);
        }
#endif
    }
    
    void cleanupWinsock() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
    
public:
    CipherChatClient() : clientSocket(INVALID_SOCKET_VAL), connected(false) {
        initializeWinsock();
        rsa.generateKeys();
    }
    
    ~CipherChatClient() {
        disconnect();
        cleanupWinsock();
    }
    
    bool connectToServer(const std::string& host, int port, const std::string& user) {
        username = user;
        
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET_VAL) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }
        
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
            std::cerr << "Invalid server address" << std::endl;
            return false;
        }
        
        if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), 
                   sizeof(serverAddr)) == SOCKET_ERROR_VAL) {
            std::cerr << "Failed to connect to server" << std::endl;
            return false;
        }
        
        // Send username
        send(clientSocket, username.c_str(), username.length(), 0);
        
        connected = true;
        
        // Start receiving messages
        receiveThread = std::thread(&CipherChatClient::receiveMessages, this);
        
        return true;
    }
    
    void disconnect() {
        connected = false;
        if (clientSocket != INVALID_SOCKET_VAL) {
            close_socket(clientSocket);
            clientSocket = INVALID_SOCKET_VAL;
        }
        if (receiveThread.joinable()) {
            receiveThread.join();
        }
    }
    
    void sendMessage(const std::string& message) {
        if (connected && !message.empty()) {
            send(clientSocket, message.c_str(), message.length(), 0);
        }
    }
    
    void startChat() {
        std::string input;
        std::cout << "\n=== CipherChat Client ===" << std::endl;
        std::cout << "Type your messages (or /quit to exit):" << std::endl;
        
        while (connected) {
            std::getline(std::cin, input);
            if (input == "/quit") {
                sendMessage("/quit");
                break;
            }
            if (!input.empty()) {
                sendMessage(input);
            }
        }
    }
    
private:
    void receiveMessages() {
        char buffer[1024];
        while (connected) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived <= 0) {
                connected = false;
                std::cout << "\nDisconnected from server." << std::endl;
                break;
            }
            
            buffer[bytesReceived] = '\0';
            std::cout << buffer << std::flush;
        }
    }
};

// Main function with menu system
void showMenu() {
    std::cout << "\n=== CipherChat-CPP ===" << std::endl;
    std::cout << "1. Start Server" << std::endl;
    std::cout << "2. Connect as Client" << std::endl;
    std::cout << "3. Exit" << std::endl;
    std::cout << "Choose an option: ";
}

int main() {
    std::cout << "Welcome to CipherChat-CPP - Secure Encrypted Messaging" << std::endl;
    std::cout << "======================================================" << std::endl;
    
    int choice;
    while (true) {
        showMenu();
        std::cin >> choice;
        std::cin.ignore(); // Clear input buffer
        
        switch (choice) {
            case 1: {
                std::cout << "\nStarting CipherChat Server..." << std::endl;
                CipherChatServer server;
                
                int port;
                std::cout << "Enter port (default 8080): ";
                std::string portStr;
                std::getline(std::cin, portStr);
                port = portStr.empty() ? 8080 : std::stoi(portStr);
                
                if (server.start(port)) {
                    std::cout << "Server running. Press Enter to stop..." << std::endl;
                    std::cin.get();
                    server.stop();
                }
                break;
            }
            
            case 2: {
                std::cout << "\nConnecting to CipherChat Server..." << std::endl;
                CipherChatClient client;
                
                std::string host, username;
                int port;
                
                std::cout << "Enter server address (default localhost): ";
                std::getline(std::cin, host);
                if (host.empty()) host = "127.0.0.1";
                
                std::cout << "Enter port (default 8080): ";
                std::string portStr;
                std::getline(std::cin, portStr);
                port = portStr.empty() ? 8080 : std::stoi(portStr);
                
                std::cout << "Enter your username: ";
                std::getline(std::cin, username);
                
                if (client.connectToServer(host, port, username)) {
                    std::cout << "Connected successfully!" << std::endl;
                    client.startChat();
                }
                break;
            }
            
            case 3:
                std::cout << "Thanks for using CipherChat-CPP!" << std::endl;
                return 0;
                
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
        }
    }
    
    return 0;
}
