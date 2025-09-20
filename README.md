# CipherChat-CPP

A secure, encrypted messaging application written in C++ with end-to-end encryption capabilities.

## Features

- **End-to-End Encryption**: Messages are encrypted using RSA and AES-like ciphers
- **Multi-User Support**: Multiple clients can connect simultaneously
- **Chat Rooms**: Support for multiple chat rooms
- **Cross-Platform**: Works on Windows, Linux, and macOS
- **Console Interface**: Simple command-line interface
- **Real-Time Messaging**: Instant message delivery
- **Secure Key Exchange**: RSA-based key exchange for secure communication

## Architecture

- **Client-Server Model**: Centralized server handles message routing
- **Multi-Threading**: Each client connection handled in separate thread
- **Socket Programming**: TCP sockets for reliable communication
- **Encryption**: 
  - RSA for key exchange and authentication
  - Symmetric cipher for message encryption
  - Session-based security

## Building

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Make (optional, for using Makefile)
- Platform-specific network libraries:
  - Windows: Winsock2 (included)
  - Linux/macOS: POSIX sockets (built-in)

### Compilation

#### Using Makefile (Recommended)

```bash
# Build the application
make

# Build debug version
make debug

# Build release version
make release

# Clean build files
make clean

# Install system-wide (Linux/macOS)
sudo make install
```

#### Manual Compilation

**Linux/macOS:**
```bash
g++ -std=c++17 -Wall -Wextra -O2 -pthread -o cipherchat main.cpp
```

**Windows (MinGW):**
```bash
g++ -std=c++17 -Wall -Wextra -O2 -o cipherchat.exe main.cpp -lws2_32
```

**Windows (MSVC):**
```bash
cl /std:c++17 /O2 main.cpp /Fe:cipherchat.exe ws2_32.lib
```

## Usage

### Starting the Server

1. Run the application:
   ```bash
   ./cipherchat
   ```

2. Choose option `1` to start server

3. Enter the port number (default: 8080)

4. Server will start and listen for connections

### Connecting as Client

1. Run the application:
   ```bash
   ./cipherchat
   ```

2. Choose option `2` to connect as client

3. Enter server details:
   - Server address (default: localhost)
   - Port (default: 8080)
   - Your username

4. Start chatting!

## Commands

Once connected as a client, you can use these commands:

- **Regular messaging**: Just type your message and press Enter
- `/encrypt <message>` - Send an encrypted message
- `/users` - List all users in the current room
- `/join <room>` - Join a specific chat room
- `/quit` - Leave the chat

## Security Features

### Encryption Implementation

1. **RSA Key Exchange**: 
   - Each client generates RSA key pairs
   - Public keys are exchanged for secure communication
   - Private keys remain local for decryption

2. **Session Encryption**:
   - Symmetric encryption for message content
   - XOR-based cipher with key rotation
   - Session-specific encryption keys

3. **Message Integrity**:
   - Timestamp-based message ordering
   - Sender verification
   - Anti-replay protection

### Security Notes

⚠️ **Important**: This implementation uses simplified cryptographic algorithms for demonstration purposes. For production use, consider:

- Using established cryptographic libraries (OpenSSL, libsodium)
- Implementing proper key management
- Adding certificate-base
