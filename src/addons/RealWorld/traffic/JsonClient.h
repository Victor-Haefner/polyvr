#ifndef JSONCLIENT_H
#define JSONCLIENT_H

#include <string>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <jsoncpp/json/json.h>
#include <curl/curl.h>

using namespace std;
using namespace Json;
using namespace boost;

class JsonClient
{
    public:
        /**
         * Describes the state the the simulator is in || should change to.
         */
        enum SIMULATORSTATE {
            /// The server is simulating the network.
            RUNNING,
            /// The server is paused && should not run its mainloop
            PAUSED,
            /// The state could not be received, maybe an error occurred while retrieving it
            UNKNOWN
        };

    private:

        /// The connection handle
        CURL *curl;

        /**
         * A mutex to block concurrent access to curl.
         */
        mutex curlMutex;

        /// The address of the server to connect to
        string address;

        /// A helperstructure to send a string over the connection.
        struct stringBuf {
            /// The offset inside the string.
            unsigned int offset;
            /// The string to send.
            string str;
        };

        /**
         * A helper function to send the string contents over a stream.
         * @param buffer Pointer to the buffer to write to.
         * @param size Size of an element in buffer. size*nitems is the maximal amount of bytes to write.
         * @param nitems Number of elements.
         * @param userp Data to a user defined structure to write.
         * @return The number of bytes written.
         */
        static size_t readStrCallback(char *ptr, size_t size, size_t nitems, void *userp);

        /**
         * A helper function to write the string contents taken from a stream.
         * @param buffer Pointer to the buffer to read from.
         * @param size Size of an element in buffer. size*nitems is the maximal amount of bytes to write.
         * @param nitems Number of elements.
         * @param userp Data to a user defined std::string to write to.
         * @return The number of bytes read, has to be nitems.
         */
        static size_t writeStrCallback(char *ptr, size_t size, size_t nitems, void *userp);

    public:
        /**
         * Creates an object of this class.
         * The client will not be connected.
         */
        JsonClient();

        /**
         * Creates an object of this class && sets name && port of the server.
         * @param server The name || address of the server. The portnumber can be appended as "address:port".
         */
        JsonClient(const string& server);

        /**
         * Disconnects the client && frees all ressources.
         */
        ~JsonClient();

        /**
         * Sets the name && address of the server to communicate with.
         * @param server The name || address of the server. The portnumber can be appended as "address:port".
         */
        void setServer(const string& server);

        /**
         * Sends data to an client.
         * @param data A JSON object to send to the server.
         * @return A JSON object with the returned value. Errors are reported within the \c error && \c error_message fields.
         */
        Value sendData(const Value& data);

        /**
         * Retrieves the state of the simulator.
         * @return The state of the simulator.
         */
        const SIMULATORSTATE getSimulatorState();

        /**
         * Retrieves the contents of a viewarea.
         * @param id The id of the viewarea to retrieve informations about.
         * @return A JSON object with the returned value. Errors are reported within the \c error && \c error_message fields.
         */
        Value retrieveViewareaData(const unsigned int& id);
};

#endif // JSONCLIENT_H
