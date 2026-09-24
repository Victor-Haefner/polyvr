#include "VRPing.h"
#include "core/utils/toString.h"
#ifndef _WIN32
#include "VRARP.h"
#endif

#include <iostream>
#include <cstdlib>
#include <iostream>
#include <string>

#include "asio.hpp"

using asio::ip::tcp;
using namespace std::placeholders;

class ping_client {
    private:
        asio::io_context io_service_;
        tcp::socket socket_;
        asio::steady_timer deadline_;
        asio::streambuf input_buffer_;

        void check_deadline() {
            if (deadline_.expiry() <= std::chrono::steady_clock::now()) {
                  asio::error_code ignored_ec;
                  socket_.close(ignored_ec);
                  deadline_.expires_at(std::chrono::steady_clock::time_point::max());
            }

            deadline_.async_wait(bind(&ping_client::check_deadline, this)); // Put the actor back to sleep.
        }

    public:
        ping_client() : socket_(io_service_), deadline_(io_service_) {
            deadline_.expires_at(std::chrono::steady_clock::time_point::max());
            check_deadline();
        }

        bool connect(const std::string& host, const std::string& port, std::chrono::steady_clock::duration timeout) {
            try {
                tcp::resolver resolver(io_service_);
                tcp::resolver::results_type endpoints = resolver.resolve(host, port);
                deadline_.expires_after(timeout);

                asio::error_code ec = asio::error::would_block;
                auto onErr = [&](const asio::error_code& e, tcp::endpoint) { ec = e; };
                asio::async_connect(socket_, endpoints, onErr);

                do io_service_.run_one(); while (ec == asio::error::would_block);

                return !(ec || !socket_.is_open());
            } catch(...) {
                return false;
            }
        }
};

using namespace std;

OSG::VRPing::VRPing() {;}
std::shared_ptr<OSG::VRPing> OSG::VRPing::create() { return std::shared_ptr<OSG::VRPing>( new OSG::VRPing() ); }

bool OSG::VRPing::startOnPort(string address, string port, int timeout) {
    ping_client c;
    return c.connect(address, port, std::chrono::seconds(timeout));
}

/**
uses system call to ping.. meh..
the nicer solution would be to use boost asio with icmp socket!
BUT! ..this requires root rights, thus cannot be used :(
**/

bool OSG::VRPing::start(std::string address, int timeout) {
    int max_attempts = 1;

#ifdef _WIN32
    std::string command = "ping -n " + toString(max_attempts) + " " + address + " 2>&1";
    int code = system(command.c_str());
    return (code == 0);
#else
    std::string command = "ping -c " + toString(max_attempts) + " " + address + " 2>&1";
    FILE* in;
    char buff[512];
    if (!(in = popen(command.c_str(), "r"))) return false;
    while (fgets(buff, sizeof(buff), in)!=NULL) ;
    int code =  pclose(in);
	return (code == 0);
#endif
}

// define popen, not just for here, but also for python API
#ifdef WASM
FILE *popen(const char *command, const char *type) { return 0; }
#endif

std::string OSG::VRPing::getMAC(std::string IP, std::string interface) {
#if !defined(_WIN32) && !defined(__APPLE__)
    return ::getMAC(IP, interface);
#else
    return "";
#endif
}
