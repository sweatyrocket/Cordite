#pragma once
#define WIN32_LEAN_AND_MEAN
#define SECURITY_WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sspi.h>
#include <schannel.h>
#include <vector>
#include <iostream>

class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();
    bool Connect(const std::string& host, const std::string& port, const std::string& path);
    void Send(const std::string& payload);
    std::string Receive();
    void Disconnect();
    void SendPing();
    bool IsConnected() const { return sock != INVALID_SOCKET; }

private:
    SOCKET sock;
    CredHandle hCred;
    CtxtHandle hContext;
    SecPkgContext_StreamSizes streamSizes;
    std::vector<char> frameBuffer;
    std::vector<char> receiveBuffer;

    bool PerformClientHandshake(const std::string& host);
    bool SendEncrypted(const char* data, int len);
    std::string ReceiveEncrypted();
    void SendFrame(const std::string& payload, uint8_t opcode = 0x1);
    std::string ParseFrame(const std::vector<char>& buffer, int received);
    std::string GenerateWebSocketKey();
};
