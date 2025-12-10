#include "VRTCPUtils.h"
#include "core/networking/rest/VRRestClient.h"
#include "core/networking/rest/VRRestResponse.h"
#include "core/utils/toString.h"

#include "asio.hpp"
#include <iostream>

using namespace OSG;
using asio::ip::udp;
using asio::ip::tcp;

string VRTCPUtils::getLocalIP() {
    string lIP;
    try {
        asio::io_context netService;
        udp::resolver resolver(netService);
        udp::resolver::results_type endpoints = resolver.resolve(udp::v4(), "google.com", "");
        udp::endpoint ep = *endpoints.begin();
        udp::socket socket(netService);
        socket.connect(ep);
        asio::ip::address addr = socket.local_endpoint().address();
        lIP = addr.to_string();
    } catch (std::exception& e){
        std::cout << "VRTCPUtils::getLocalIP exception: " << e.what() << std::endl;
    }
    return lIP;
}

string VRTCPUtils::getHostName(string uri) {
    if (contains(uri, "//")) {
        uri = splitString(uri, "//")[1];
    }
    if (contains(uri, "/")) {
        uri = splitString(uri, "/")[0];
    }
    if (contains(uri, ":")) {
        uri = splitString(uri, ":")[0];
    }
    return uri;
}

string VRTCPUtils::getHostIP(string host) {
    host = getHostName(host);

    string IP;
    try {
        asio::io_context netService;
        tcp::resolver resolver(netService);
        tcp::resolver::results_type endpoints = resolver.resolve(host, "80");
        tcp::endpoint ep = *endpoints.begin();
        IP = ep.address().to_string();
    } catch (std::exception& e){
        std::cout << "VRTCPUtils::getHostIP from " << host << ", exception: " << e.what() << std::endl;
    }
    return IP;
}

string VRTCPUtils::getPublicIP(bool cached) {
    static string cachedIP = "";
    if (cached && cachedIP != "") return cachedIP;
    auto cli = VRRestClient::create();
    VRRestResponsePtr res = cli->get("http://api.ipify.org/", 20);
    cachedIP = res->getData();
    return cachedIP;

    /*int sock = socket(AF_INET, SOCK_DGRAM, 0);
    assert(sock != -1);

    const char* kGoogleDnsIp = "8.8.8.8";
    uint16_t kDnsPort = 53;
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
    serv.sin_port = htons(kDnsPort);

    connect(sock, (const sockaddr*) &serv, sizeof(serv));

    sockaddr_in name;
    socklen_t namelen = sizeof(name);
    getsockname(sock, (sockaddr*) &name, &namelen);

    char addressBuffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &name.sin_addr, addressBuffer, INET_ADDRSTRLEN);
#ifndef _WINDOWS
	::close(sock);
#else
    closesocket(sock);
#endif

    return string(addressBuffer);*/
}
