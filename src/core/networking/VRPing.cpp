#include "VRPing.h"
#include "core/utils/toString.h"

#include <boost/asio.hpp>
#include <iostream>


#include <boost/asio/connect.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/system/system_error.hpp>
#include <boost/asio/write.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

using boost::asio::deadline_timer;
using boost::asio::ip::tcp;
using boost::lambda::bind;
using boost::lambda::var;
using boost::lambda::_1;


class ping_client {
    private:
        boost::asio::io_service io_service_;
        tcp::socket socket_;
        deadline_timer deadline_;
        boost::asio::streambuf input_buffer_;

        void check_deadline() {
            if (deadline_.expires_at() <= deadline_timer::traits_type::now()) {
                  boost::system::error_code ignored_ec;
                  socket_.close(ignored_ec);
                  deadline_.expires_at(boost::posix_time::pos_infin);
            }

            deadline_.async_wait(bind(&ping_client::check_deadline, this)); // Put the actor back to sleep.
        }

    public:
        ping_client() : socket_(io_service_), deadline_(io_service_) {
            deadline_.expires_at(boost::posix_time::pos_infin);
            check_deadline();
        }

        bool connect(const std::string& host, const std::string& port, boost::posix_time::time_duration timeout) {
            try {
                tcp::resolver::query query(host, port);
                tcp::resolver::iterator iter = tcp::resolver(io_service_).resolve(query);
                deadline_.expires_from_now(timeout);

                boost::system::error_code ec = boost::asio::error::would_block;
                boost::asio::async_connect(socket_, iter, var(ec) = _1);
                do io_service_.run_one(); while (ec == boost::asio::error::would_block);

                return !(ec || !socket_.is_open());
            } catch(...) {
                return false;
            }
        }
};

using namespace std;

VRPing::VRPing() {;}

bool VRPing::start(string address, string port, int timeout) {
    ping_client c;
    return c.connect(address, port, boost::posix_time::seconds(timeout));
}

/**
uses system call to ping.. meh..
the nicer solution would be to use boost asio with icmp socket!
BUT! ..this requires root rights, thus cannot be used :(
**/

bool VRPing::start(std::string address, int timeout) {
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

