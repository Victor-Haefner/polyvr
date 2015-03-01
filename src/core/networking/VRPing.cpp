#include "VRPing.h"
#include <boost/asio.hpp>
#include <iostream>

using namespace std;

VRPing::VRPing() {;}

bool VRPing::start(string IP, string port, int timeout) {
    boost::asio::io_service io;
    boost::asio::io_service::work work(io);
    boost::asio::io_service::strand strand(io);
    boost::asio::ip::tcp::socket sock(io);
    boost::asio::ip::tcp::resolver resolver(io);
    boost::asio::ip::tcp::resolver::query query(IP, port);

    bool res = true;
	try { sock.connect( *resolver.resolve( query ) ); }
	catch( std::exception& ex ) {
		std::cout << " ping exception: " << ex.what() << std::endl;
		res = false;
	}

	boost::system::error_code ec;
	sock.shutdown( boost::asio::ip::tcp::socket::shutdown_both, ec );
	sock.close( ec );

	io.stop();
	return res;
}
