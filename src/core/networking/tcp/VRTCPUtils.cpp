#include "VRTCPUtils.h"
#include "core/networking/rest/VRRestClient.h"
#include "core/networking/rest/VRRestResponse.h"
#include "core/utils/toString.h"

#include <boost/asio.hpp>
#include <iostream>

using namespace OSG;
using boost::asio::ip::udp;
using boost::asio::ip::tcp;

string VRTCPUtils::getLocalIP() {
    string lIP;
    try {
        boost::asio::io_service netService;
        udp::resolver resolver(netService);
        udp::resolver::query query(udp::v4(), "google.com", "");
        udp::resolver::iterator endpoints = resolver.resolve(query);
        udp::endpoint ep = *endpoints;
        udp::socket socket(netService);
        socket.connect(ep);
        boost::asio::ip::address addr = socket.local_endpoint().address();
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
    return uri;
}

string VRTCPUtils::getHostIP(string host) {
    host = getHostName(host);

    string IP;
    try {
        boost::asio::io_service netService;
        tcp::resolver resolver(netService);
        tcp::resolver::query query(host, "80");
        tcp::resolver::iterator endpoints = resolver.resolve(query);
        tcp::endpoint ep = *endpoints;
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
