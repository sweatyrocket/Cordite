#include "core/WebSocketClient.h"
#include <random>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "crypt32.lib")

#ifndef UNISP_NAME_W
#define UNISP_NAME_W L"Microsoft Unified Security Protocol Provider"
#endif

std::string base64_encode(const unsigned char* data, size_t len) {
    const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve(((len + 2) / 3) * 4);

    for (size_t i = 0; i < len; i += 3) {
        unsigned int b = (data[i] << 16) | ((i + 1 < len ? data[i + 1] : 0) << 8) | (i + 2 < len ? data[i + 2] : 0);
        result.push_back(base64_chars[(b >> 18) & 0x3F]);
        result.push_back(base64_chars[(b >> 12) & 0x3F]);
        result.push_back(i + 1 < len ? base64_chars[(b >> 6) & 0x3F] : '=');
        result.push_back(i + 2 < len ? base64_chars[b & 0x3F] : '=');
    }
    return result;
}

WebSocketClient::WebSocketClient() : sock(INVALID_SOCKET), hCred{ 0 }, hContext{ 0 }, streamSizes{ 0 } {
    WSADATA wsaData;
    int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaResult != 0) {
        std::cerr << "WSAStartup failed: " << wsaResult << std::endl;
        throw std::runtime_error("WSAStartup failed");
    }
}

WebSocketClient::~WebSocketClient() {
    Disconnect();
    WSACleanup();
}

std::string WebSocketClient::GenerateWebSocketKey() {
    unsigned char key[16];
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (int i = 0; i < 16; ++i) {
        key[i] = static_cast<unsigned char>(dis(gen));
    }
    return base64_encode(key, sizeof(key));
}

bool WebSocketClient::PerformClientHandshake(const std::string& host) {
    std::wstring whost(host.begin(), host.end());

    DWORD flags = ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY |
        ISC_REQ_USE_SESSION_KEY | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM;

    SecBufferDesc outBufferDesc, inBufferDesc;
    SecBuffer outBuffers[1], inBuffers[2];
    DWORD contextReq = 0;
    SECURITY_STATUS status = SEC_E_INTERNAL_ERROR;
    bool firstCall = true;

    std::vector<WCHAR> targetName(whost.size() + 1);
    wcscpy_s(targetName.data(), targetName.size(), whost.c_str());

    outBuffers[0].pvBuffer = nullptr;
    outBuffers[0].BufferType = SECBUFFER_TOKEN;
    outBuffers[0].cbBuffer = 0;
    outBufferDesc.ulVersion = SECBUFFER_VERSION;
    outBufferDesc.cBuffers = 1;
    outBufferDesc.pBuffers = outBuffers;

    std::vector<char> tempBuffer(16384);
    size_t tempBufferUsed = 0;

    while (true) {
        if (firstCall) {
            status = InitializeSecurityContextW(
                &hCred,
                nullptr,
                targetName.data(),
                flags,
                0,
                0,
                nullptr,
                0,
                &hContext,
                &outBufferDesc,
                &contextReq,
                nullptr
            );
            firstCall = false;
        }
        else {
            status = InitializeSecurityContextW(
                &hCred,
                &hContext,
                nullptr,
                flags,
                0,
                0,
                &inBufferDesc,
                0,
                nullptr,
                &outBufferDesc,
                &contextReq,
                nullptr
            );
        }

        if (status != SEC_I_CONTINUE_NEEDED && status != SEC_E_OK) {
            std::cerr << "InitializeSecurityContext failed with status: 0x" << std::hex << status << std::dec << std::endl;
            if (outBuffers[0].pvBuffer) {
                FreeContextBuffer(outBuffers[0].pvBuffer);
            }
            return false;
        }

        if (outBuffers[0].cbBuffer != 0 && outBuffers[0].pvBuffer != nullptr) {
            char* tokenBuffer = static_cast<char*>(outBuffers[0].pvBuffer);
            int bytesToSend = (int)outBuffers[0].cbBuffer;
            int totalSent = 0;

            std::cout << "Attempting to send handshake token (" << bytesToSend << " bytes)..." << std::endl;

            while (totalSent < bytesToSend) {
                int sent = send(sock, tokenBuffer + totalSent, bytesToSend - totalSent, 0);

                if (sent == SOCKET_ERROR) {
                    int error = WSAGetLastError();
                    std::cerr << "send failed during handshake token send. Error: " << error << std::endl;
                    FreeContextBuffer(outBuffers[0].pvBuffer);
                    outBuffers[0].pvBuffer = nullptr;
                    return false;
                }
                if (sent == 0) {
                    std::cerr << "send returned 0 during handshake token send (connection closed?)." << std::endl;
                    FreeContextBuffer(outBuffers[0].pvBuffer);
                    outBuffers[0].pvBuffer = nullptr;
                    return false;
                }

                totalSent += sent;
            }

            std::cout << "Successfully sent handshake token (" << totalSent << " bytes)." << std::endl;

            FreeContextBuffer(outBuffers[0].pvBuffer);
            outBuffers[0].pvBuffer = nullptr;
        }


        if (status == SEC_E_OK) {
            break;
        }

        if (tempBufferUsed == tempBuffer.size()) {
            std::cerr << "Handshake buffer overflow." << std::endl;
            return false;
        }
        int received = recv(sock, tempBuffer.data() + tempBufferUsed, (int)(tempBuffer.size() - tempBufferUsed), 0);
        if (received == SOCKET_ERROR || received == 0) {
            std::cerr << "recv failed during handshake: " << WSAGetLastError() << std::endl;
            return false;
        }
        tempBufferUsed += received;

        inBuffers[0].pvBuffer = tempBuffer.data();
        inBuffers[0].cbBuffer = (unsigned long)tempBufferUsed;
        inBuffers[0].BufferType = SECBUFFER_TOKEN;
        inBuffers[1].pvBuffer = nullptr;
        inBuffers[1].cbBuffer = 0;
        inBuffers[1].BufferType = SECBUFFER_EMPTY;
        inBufferDesc.ulVersion = SECBUFFER_VERSION;
        inBufferDesc.cBuffers = 2;
        inBufferDesc.pBuffers = inBuffers;

        outBuffers[0].pvBuffer = nullptr;
        outBuffers[0].cbBuffer = 0;
        outBuffers[0].BufferType = SECBUFFER_TOKEN;
        outBufferDesc.ulVersion = SECBUFFER_VERSION;
        outBufferDesc.cBuffers = 1;
        outBufferDesc.pBuffers = outBuffers;

        if (inBuffers[1].BufferType == SECBUFFER_EXTRA && inBuffers[1].cbBuffer > 0) {
            memmove(tempBuffer.data(),
                tempBuffer.data() + (tempBufferUsed - inBuffers[1].cbBuffer),
                inBuffers[1].cbBuffer);
            tempBufferUsed = inBuffers[1].cbBuffer;
        }
        else {
            tempBufferUsed = 0;
        }
    }

    status = QueryContextAttributesW(&hContext, SECPKG_ATTR_STREAM_SIZES, &streamSizes);
    if (status != SEC_E_OK) {
        std::cerr << "QueryContextAttributes (StreamSizes) failed: 0x" << std::hex << status << std::dec << std::endl;
        return false;
    }

    std::cout << "Schannel handshake completed successfully." << std::endl;
    return true;
}


bool WebSocketClient::SendEncrypted(const char* data, int len) {
    if (!IsConnected()) return false;

    std::vector<char> messageData(data, data + len);
    std::vector<char> headerBuffer(streamSizes.cbHeader);
    std::vector<char> trailerBuffer(streamSizes.cbTrailer);

    SecBuffer buffers[4] = {
        { streamSizes.cbHeader,  SECBUFFER_STREAM_HEADER, headerBuffer.data() },
        { (unsigned long)len,    SECBUFFER_DATA,          messageData.data() },
        { streamSizes.cbTrailer, SECBUFFER_STREAM_TRAILER,trailerBuffer.data() },
        { 0,                     SECBUFFER_EMPTY,         nullptr }
    };
    SecBufferDesc bufferDesc = { SECBUFFER_VERSION, 4, buffers };

    SECURITY_STATUS status = EncryptMessage(&hContext, 0, &bufferDesc, 0);
    if (status != SEC_E_OK) {
        std::cerr << "EncryptMessage failed: 0x" << std::hex << status << std::dec << std::endl;
        return false;
    }

    std::vector<char> sendBuffer;
    sendBuffer.reserve(buffers[0].cbBuffer + buffers[1].cbBuffer + buffers[2].cbBuffer);
    sendBuffer.insert(sendBuffer.end(), headerBuffer.data(), headerBuffer.data() + buffers[0].cbBuffer);
    sendBuffer.insert(sendBuffer.end(), messageData.data(), messageData.data() + buffers[1].cbBuffer);
    sendBuffer.insert(sendBuffer.end(), trailerBuffer.data(), trailerBuffer.data() + buffers[2].cbBuffer);

    int bytesToSend = (int)sendBuffer.size();
    int totalSent = 0;
    const char* sendDataPtr = sendBuffer.data();

    while (totalSent < bytesToSend) {
        int sent = send(sock, sendDataPtr + totalSent, bytesToSend - totalSent, 0);
        if (sent == SOCKET_ERROR) {
            int error = WSAGetLastError();
            std::cerr << "send failed during SendEncrypted. Error: " << error << std::endl;
            return false;
        }
        if (sent == 0) {
            std::cerr << "send returned 0 during SendEncrypted (connection closed?)." << std::endl;
            return false;
        }
        totalSent += sent;
    }

    return true;
}

std::string WebSocketClient::ReceiveEncrypted() {
    if (!IsConnected()) return "DISCONNECTED";

    std::string decryptedPayload;

    while (true) {
        if (!receiveBuffer.empty()) {
            SecBuffer buffers[4] = {
                { (unsigned long)receiveBuffer.size(), SECBUFFER_DATA, receiveBuffer.data() },
                { 0, SECBUFFER_EMPTY, nullptr }, { 0, SECBUFFER_EMPTY, nullptr }, { 0, SECBUFFER_EMPTY, nullptr }
            };
            SecBufferDesc bufferDesc = { SECBUFFER_VERSION, 4, buffers };
            ULONG qop = 0;

            SECURITY_STATUS status = DecryptMessage(&hContext, &bufferDesc, 0, &qop);

            if (status == SEC_E_OK) {
                SecBuffer* dataBuffer = nullptr;
                SecBuffer* extraBuffer = nullptr;
                for (int i = 0; i < 4; ++i) {
                    if (dataBuffer == nullptr && buffers[i].BufferType == SECBUFFER_DATA) dataBuffer = &buffers[i];
                    if (extraBuffer == nullptr && buffers[i].BufferType == SECBUFFER_EXTRA) extraBuffer = &buffers[i];
                }

                if (dataBuffer && dataBuffer->cbBuffer > 0) {
                    decryptedPayload.append(static_cast<char*>(dataBuffer->pvBuffer), dataBuffer->cbBuffer);
                }

                if (extraBuffer && extraBuffer->cbBuffer > 0) {
                    std::vector<char> tempExtra(static_cast<char*>(extraBuffer->pvBuffer),
                        static_cast<char*>(extraBuffer->pvBuffer) + extraBuffer->cbBuffer);
                    receiveBuffer = std::move(tempExtra);
                }
                else {
                    receiveBuffer.clear();
                }
                return decryptedPayload;

            }
            else if (status == SEC_E_INCOMPLETE_MESSAGE) {
                ;
            }
            else if (status == SEC_I_CONTEXT_EXPIRED) {
                std::cerr << "Schannel context expired." << std::endl;
                Disconnect();
                return "DISCONNECTED";
            }
            else if (status == SEC_I_RENEGOTIATE) {
                std::cerr << "Schannel renegotiation requested (not implemented)." << std::endl;
                Disconnect();
                return "DISCONNECTED";
            }
            else {
                std::cerr << "DecryptMessage failed: 0x" << std::hex << status << std::dec << std::endl;
                Disconnect();
                return "DISCONNECTED";
            }
        }

        std::vector<char> tempRecvBuffer(16384);
        int received = recv(sock, tempRecvBuffer.data(), (int)tempRecvBuffer.size(), 0);

        if (received == SOCKET_ERROR) {
            int error = WSAGetLastError();
            std::cerr << "recv failed: " << error << std::endl;
            Disconnect();
            return "DISCONNECTED";
        }
        else if (received == 0) {
            std::cout << "Connection closed by peer during ReceiveEncrypted." << std::endl;
            Disconnect();
            return "DISCONNECTED";
        }
        else {
            receiveBuffer.insert(receiveBuffer.end(), tempRecvBuffer.data(), tempRecvBuffer.data() + received);
        }
    }
}


bool WebSocketClient::Connect(const std::string& host, const std::string& port, const std::string& path) {
    Disconnect();

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        return false;
    }

    ADDRINFOA hints = { 0 };
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    ADDRINFOA* result = nullptr;

    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &result) != 0) {
        std::cerr << "getaddrinfo failed for " << host << ":" << port << " - Error: " << WSAGetLastError() << std::endl;
        closesocket(sock); sock = INVALID_SOCKET;
        return false;
    }

    bool connected = false;
    for (ADDRINFOA* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        if (connect(sock, ptr->ai_addr, (int)ptr->ai_addrlen) == 0) {
            connected = true; break;
        }
    }
    freeaddrinfo(result);

    if (!connected) {
        std::cerr << "Connect failed to " << host << ":" << port << " - Error: " << WSAGetLastError() << std::endl;
        closesocket(sock); sock = INVALID_SOCKET;
        return false;
    }
    std::cout << "TCP connection established to " << host << ":" << port << std::endl;

    SCHANNEL_CRED schannelCred = { 0 };
    schannelCred.dwVersion = SCHANNEL_CRED_VERSION;
    schannelCred.grbitEnabledProtocols = SP_PROT_TLS1_2_CLIENT | SP_PROT_TLS1_3_CLIENT;

    TimeStamp tsExpiry;
    const wchar_t* packageNameLiteral = UNISP_NAME_W;
    std::vector<WCHAR> packageName(wcslen(packageNameLiteral) + 1);
    wcscpy_s(packageName.data(), packageName.size(), packageNameLiteral);

    SECURITY_STATUS status = AcquireCredentialsHandleW(
        nullptr, packageName.data(), SECPKG_CRED_OUTBOUND, nullptr,
        &schannelCred, nullptr, nullptr, &hCred, &tsExpiry);

    if (status != SEC_E_OK) {
        std::cerr << "AcquireCredentialsHandle failed: 0x" << std::hex << status << std::dec << std::endl;
        closesocket(sock); sock = INVALID_SOCKET;
        return false;
    }
    std::cout << "Acquired Schannel credentials handle." << std::endl;

    if (!PerformClientHandshake(host)) {
        std::cerr << "Schannel handshake failed." << std::endl;
        Disconnect();
        return false;
    }

    std::string webSocketKey = GenerateWebSocketKey();
    std::string handshakeRequest = "GET " + path + " HTTP/1.1\r\n"
        "Host: " + host + "\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: " + webSocketKey + "\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n";

    std::cout << "Sending WebSocket handshake request..." << std::endl;
    if (!SendEncrypted(handshakeRequest.c_str(), (int)handshakeRequest.length())) {
        std::cerr << "Failed to send WebSocket handshake request." << std::endl;
        Disconnect();
        return false;
    }

    std::cout << "Receiving WebSocket handshake response..." << std::endl;
    std::string response = ReceiveEncrypted();
    if (response.empty() || response == "DISCONNECTED") {
        std::cerr << "Failed to receive WebSocket handshake response or disconnected." << std::endl;
        if (IsConnected()) Disconnect();
        return false;
    }

    std::cout << "Validating WebSocket handshake response..." << std::endl;

    size_t headersStart = response.find("\r\n");
    std::string headersPart = (headersStart == std::string::npos) ? response : response.substr(headersStart);
    std::string headersLower = headersPart;
    std::transform(headersLower.begin(), headersLower.end(), headersLower.begin(),
        [](unsigned char c) { return std::tolower(c); });

    if (response.find("HTTP/1.1 101") == std::string::npos ||
        headersLower.find("upgrade: websocket") == std::string::npos ||
        headersLower.find("connection: upgrade") == std::string::npos) {
        std::cerr << "WebSocket handshake failed. Invalid server response headers:\n"
            << response.substr(0, 500) << "..." << std::endl;
        Disconnect();
        return false;
    }

    std::cout << "WebSocket handshake response validated." << std::endl;


    std::cout << "WebSocket connection established successfully." << std::endl;
    frameBuffer.clear();
    receiveBuffer.clear();
    return true;
}

void WebSocketClient::Send(const std::string& payload) {
    if (!IsConnected()) {
        std::cerr << "Cannot send, not connected." << std::endl;
        return;
    }
    SendFrame(payload);
}

std::string WebSocketClient::Receive() {
    if (!IsConnected()) {
        return "DISCONNECTED";
    }

    std::string decryptedData = ReceiveEncrypted();

    if (decryptedData == "DISCONNECTED") {
        return "DISCONNECTED";
    }
    if (decryptedData.empty() && receiveBuffer.empty()) {
        return "";
    }

    frameBuffer.insert(frameBuffer.end(), decryptedData.begin(), decryptedData.end());

    while (true) {
        if (frameBuffer.empty()) {
            return "";
        }

        std::vector<char> currentFrameData = frameBuffer;
        std::string message = ParseFrame(currentFrameData, currentFrameData.size());

        if (message == "DISCONNECTED") {
            frameBuffer.clear();
            return "DISCONNECTED";
        }
        else if (!message.empty()) {

            frameBuffer.clear();
            return message;
        }
        else {

            return "";
        }

    }
}

void WebSocketClient::Disconnect() {
    if (sock != INVALID_SOCKET) {
        if (hContext.dwLower != 0 || hContext.dwUpper != 0) {
            DWORD shutdownType = SCHANNEL_SHUTDOWN;
            SecBuffer shutdownBuffer[1] = { sizeof(DWORD), SECBUFFER_TOKEN, &shutdownType };
            SecBufferDesc shutdownDesc = { SECBUFFER_VERSION, 1, shutdownBuffer };
            ApplyControlToken(&hContext, &shutdownDesc);
        }
        shutdown(sock, SD_BOTH);
        closesocket(sock);
        sock = INVALID_SOCKET;
        std::cout << "Socket closed." << std::endl;
    }
    if (hContext.dwLower != 0 || hContext.dwUpper != 0) {
        DeleteSecurityContext(&hContext);
        hContext = { 0 };
        std::cout << "Schannel context deleted." << std::endl;
    }
    if (hCred.dwLower != 0 || hCred.dwUpper != 0) {
        FreeCredentialsHandle(&hCred);
        hCred = { 0 };
        std::cout << "Schannel credentials freed." << std::endl;
    }
    frameBuffer.clear();
    receiveBuffer.clear();
}

void WebSocketClient::SendPing() {
    if (!IsConnected()) {
        std::cerr << "Cannot send ping, not connected." << std::endl;
        return;
    }
    std::cout << "Sending WebSocket ping..." << std::endl;
    SendFrame("", 0x9);
}

void WebSocketClient::SendFrame(const std::string& payload, uint8_t opcode /*= 0x1*/) {
    if (!IsConnected()) {
        std::cerr << "Cannot send frame, not connected." << std::endl;
        return;
    }

    std::vector<unsigned char> frame;
    size_t len = payload.length();

    frame.push_back(0x80 | (opcode & 0x0F));

    if (len <= 125) {
        frame.push_back(0x80 | static_cast<unsigned char>(len));
    }
    else if (len <= 65535) {
        frame.push_back(0x80 | 126);
        frame.push_back(static_cast<unsigned char>((len >> 8) & 0xFF));
        frame.push_back(static_cast<unsigned char>(len & 0xFF));
    }
    else {
        frame.push_back(0x80 | 127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<unsigned char>((len >> (i * 8)) & 0xFF));
        }
    }

    unsigned char mask[4];
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (int i = 0; i < 4; ++i) {
        mask[i] = static_cast<unsigned char>(dis(gen));
        frame.push_back(mask[i]);
    }

    size_t headerSize = frame.size();
    frame.resize(headerSize + len);
    for (size_t i = 0; i < len; ++i) {
        frame[headerSize + i] = static_cast<unsigned char>(payload[i]) ^ mask[i % 4];
    }

    if (!SendEncrypted(reinterpret_cast<const char*>(frame.data()), (int)frame.size())) {
        std::cerr << "Failed to send WebSocket frame (opcode: " << (int)opcode << ")" << std::endl;
    }
}


std::string WebSocketClient::ParseFrame(const std::vector<char>& buffer, int received) {
    if (!IsConnected()) return "DISCONNECTED";

    if (received < 2) return "";

    const unsigned char* data = reinterpret_cast<const unsigned char*>(buffer.data());

    bool fin = (data[0] & 0x80) != 0;
    uint8_t rsv = (data[0] & 0x70) >> 4;
    uint8_t opcode = data[0] & 0x0F;
    bool masked = (data[1] & 0x80) != 0;
    uint64_t payload_len = data[1] & 0x7F;
    int offset = 2;

    if (rsv != 0) {
        std::cerr << "Error: Received frame with non-zero RSV bits (" << (int)rsv << ")" << std::endl;
        Disconnect(); return "DISCONNECTED";
    }

    if (payload_len == 126) {
        if (received < 4) return "";
        payload_len = (static_cast<uint64_t>(data[2]) << 8) | data[3];
        offset += 2;
    }
    else if (payload_len == 127) {
        if (received < 10) return "";
        payload_len = 0;
        for (int i = 0; i < 8; ++i) payload_len = (payload_len << 8) | data[offset + i];
        if (payload_len > (1ULL << 63)) {
            std::cerr << "Error: Received frame with excessive payload length." << std::endl;
            Disconnect(); return "DISCONNECTED";
        }
        offset += 8;
    }

    if (masked) {
        std::cerr << "Error: Received masked frame from server." << std::endl;
        Disconnect(); return "DISCONNECTED";
    }

    uint64_t required_size_64 = (uint64_t)offset + payload_len;
    int required_size = (int)required_size_64;
    if (required_size_64 > INT_MAX || required_size < 0) {
        std::cerr << "Error: Calculated required frame size exceeds INT_MAX or is negative." << std::endl;
        Disconnect(); return "DISCONNECTED";
    }
    if (received < required_size) {
        return "";
    }

    std::string payload(buffer.data() + offset, (size_t)payload_len);

    switch (opcode) {
    case 0x0:
        std::cerr << "Warning: Received continuation frame (fragmentation not fully supported)." << std::endl;

        return "";

    case 0x1:
        if (!fin) {
            std::cerr << "Warning: Fragmented text frame received (fragmentation not fully supported)." << std::endl;

            return "";
        }

        return payload;

    case 0x2:
        if (!fin) {
            std::cerr << "Warning: Fragmented binary frame received (fragmentation not fully supported)." << std::endl;

            return "";
        }
        std::cout << "Received binary frame: " << payload.length() << " bytes (handling as text)" << std::endl;
        return payload;

    case 0x8:
        if (payload_len == 1) {
            std::cerr << "Error: Received close frame with payload length 1." << std::endl;
            Disconnect(); return "DISCONNECTED";
        }
        if (payload_len >= 2) {
            uint16_t statusCode = (static_cast<uint16_t>(static_cast<unsigned char>(payload[0])) << 8) |
                static_cast<uint16_t>(static_cast<unsigned char>(payload[1]));

            std::string reason = payload.substr(2);
            std::cout << "Received Close frame - Status: " << statusCode << ", Reason: " << reason << std::endl;
        }
        else {
            std::cout << "Received Close frame (no status code)." << std::endl;
        }
        Disconnect();
        return "DISCONNECTED";

    case 0x9:
        if (!fin) { std::cerr << "Error: Received fragmented ping." << std::endl; Disconnect(); return "DISCONNECTED"; }
        if (payload_len > 125) { std::cerr << "Error: Received ping with payload > 125 bytes." << std::endl; Disconnect(); return "DISCONNECTED"; }
        std::cout << "Received Ping frame." << std::endl;
        SendFrame(payload, 0xA);
        return "";

    case 0xA:
        if (!fin) { std::cerr << "Error: Received fragmented pong." << std::endl; Disconnect(); return "DISCONNECTED"; }
        if (payload_len > 125) { std::cerr << "Error: Received pong with payload > 125 bytes." << std::endl; Disconnect(); return "DISCONNECTED"; }
        std::cout << "Received Pong frame." << std::endl;
        return "";

    default:
        std::cerr << "Error: Received frame with reserved/invalid opcode: " << (int)opcode << std::endl;
        Disconnect(); return "DISCONNECTED";
    }
}
