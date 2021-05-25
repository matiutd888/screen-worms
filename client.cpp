//
// Created by mateusz on 23.05.2021.
//

#include <ostream>
#include "Connection.h"

//Klient:
//
//./screen-worms-client game_server [-n player_name] [-p n] [-i gui_server] [-r n]
//game_server – adres (IPv4 lub IPv6) lub nazwa serwera gry
//-n player_name – nazwa gracza, zgodna z opisanymi niżej wymaganiami
//-p n – port serwera gry (domyślne 2021)
//-i gui_server – adres (IPv4 lub IPv6) lub nazwa serwera obsługującego interfejs użytkownika (domyślnie localhost)
//-r n – port serwera obsługującego interfejs użytkownika (domyślnie 20210)

class ClientData {
    std::string serverAddress;
    std::string playerName;
    uint16_t serverPortNum;
    std::string guiAddress;
    uint16_t guiPortNum;
public:
    ClientData(const std::string &serverAddress, const std::string &playerName, uint16_t serverPortNum,
               const std::string &guiAddress, uint16_t guiPortNum) : serverAddress(serverAddress),
                                                                     playerName(playerName),
                                                                     serverPortNum(serverPortNum),
                                                                     guiAddress(guiAddress), guiPortNum(guiPortNum) {}


    friend std::ostream &operator<<(std::ostream &os, const ClientData &data) {
        os << "serverAddress: " << data.serverAddress << " playerName: " << data.playerName << " serverPortNum: "
           << data.serverPortNum << " guiAddress: " << data.guiAddress << " guiPortNum: " << data.guiPortNum;
        return os;
    }

    [[nodiscard]] const std::string &getServerAddress() const {
        return serverAddress;
    }

    [[nodiscard]] const std::string &getPlayerName() const {
        return playerName;
    }

    [[nodiscard]] uint16_t getServerPortNum() const {
        return serverPortNum;
    }

    [[nodiscard]] const std::string &getGuiAddress() const {
        return guiAddress;
    }

    [[nodiscard]] uint16_t getGuiPortNum() const {
        return guiPortNum;
    }
};

class ClientDataBuilder {
    std::string serverAddress;
    std::string playerName;
    uint16_t serverPortNum;
    std::string guiAddress;
    uint16_t guiPortNum;
    uint32_t next_expected_event_no;
public:
    ClientDataBuilder() {
        serverAddress = "";
        playerName = "";
        serverPortNum = Utils::DEFAULT_SERVER_PORT_NUM;
        guiAddress = Utils::DEFAULT_GUI_SERVER_NAME;
        guiPortNum = Utils::DEFAULT_GUI_PORT_NUM;
    }

    void parse(int argc, char **argv) {
        char c;
        if (argc < 2)
            syserr("Client usage: game_server [-n player_name] [-p n] [-i gui_server] [-r n]");
        serverAddress = argv[1];
        while ((c = getopt(argc - 1, argv + 1, Utils::optstring)) != -1) {
            size_t convertedSize = 0;
            std::string argStr = optarg;
            bool haveFoundNumber = false;
            switch (c) {
                case 'n':
                    playerName = argStr;
                    break;
                case 'p':
                    serverPortNum = std::stoi(argStr, &convertedSize);
                    haveFoundNumber = true;
                    break;
                case 'i':
                    guiAddress = argStr;
                    break;
                case 'r':
                    guiPortNum = std::stoi(argStr, &convertedSize);
                    haveFoundNumber = true;
                    break;
                default:;
            }
            if (haveFoundNumber) {
                if (convertedSize != argStr.size())
                    syserr("Argument parsing error!");
            }
        }
    }

    ClientData build() const {
        return ClientData(serverAddress, playerName, serverPortNum, guiAddress, guiPortNum);
    }
};

class ClientSide {
    ClientData data;
    UDPClientSocket sock;
    // TCPClientSocket guiSock;
    PollClient pollClient;

public:
//
//    ClientSide(const ClientData &clientData) : data(clientData),
//                                               sock(data.getServerPortNum(), data.getServerAddress().c_str()),
//                                               guiSock(data.getGuiPortNum(), data.getGuiAddress().c_str()),
//                                               pollClient(sock.getSocket(), guiSock.getSocket()) {
//
//    }

    ClientSide(const ClientData &clientData) : data(clientData),
                                               sock(data.getServerPortNum(), data.getServerAddress().c_str()),

                                               pollClient(sock.getSocket(), 0) {

    }

    void start() {
        while (1) {
            int pollCount = pollClient.doPoll();
            if (pollCount < 0) {
                std::cout << "Error in doPoll!" << std::endl;
                continue;
            }
            if (pollCount == 0) {
                std::cout << "No DoPol events!" << std::endl;
            }

            if (pollClient.hasPollinOccurred(PollClient::MESSAGE_SERVER)) {
                std::cout << "Message server!" << std::endl;
            }
            if (pollClient.hasPollinOccurred(PollClient::INTERVAL_SENDMESSAGE)) {
                std::cout << "Sending message to server!" << std::endl;
                ClientMessage clientMessage(1234, 1, 0, "Jacek", 5);
                std::cout << clientMessage << std::endl;
                WritePacket writePacket;
                clientMessage.encode(writePacket);
                sendto(sock.getSocket(), writePacket.getBufferConst(), writePacket.getOffset(), 0,
                       sock.getAddrInfo().ai_addr, sock.getAddrInfo().ai_addrlen);
            }
        }
    }
};

int main(int argc, char **argv) {
    ClientDataBuilder builder;
    builder.parse(argc, argv);
    ClientData clientData = builder.build();
    std::cout << clientData << std::endl;
    ClientSide clientSide(clientData);
    clientSide.start();
}