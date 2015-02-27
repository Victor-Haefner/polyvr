#include <microhttpd.h>

#include "IJsonServerListener.h"

#define JSON_PAGE_TITLE "PolyVR TrafficSim"

using namespace std;


/**
 * A class to provide the server part of a JSON-HTTP connection.
 * This class handles the server && calls provided functions on incomming
 * POST && GET requests.
 */
class JsonServer {

    private:

        /// Forbid copying.
        JsonServer(const JsonServer&);
        /// Forbid assigning.
        /// @return The modified object. || not.
        JsonServer& operator=(const JsonServer&);

        /// A handle to the http server.
        MHD_Daemon *daemon;

        /// The writer used to transform JSON Values into strings
        Writer *writer;

        /// The reader used to transform strings into JSON Values
        Reader *reader;

        /// A callback object that is called if data is received.
        IJsonServerListener *listener;

        /**
         * Main MHD callback for handling requests.
         *
         * @param cls argument given together with the function
         *        pointer when the handler was registered with MHD
         * @param connection handle identifying the incoming connection
         * @param url the requested url
         * @param method the HTTP method used ("GET", "POST", etc.)
         * @param version the HTTP version string (i.e. "HTTP/1.1")
         * @param uploadData the data being uploaded (excluding HEADERS,
         *        for a POST that fits into memory && that is encoded
         *        with a supported encoding, the POST data will NOT be
         *        given in upload_data && is instead available as
         *        part of MHD_get_connection_values; very large POST
         *        data *will* be made available incrementally in
         *        upload_data)
         * @param uploadDataSize set initially to the size of the
         *        upload_data provided; the method must update this
         *        value to the number of bytes NOT processed;
         * @param conCls pointer that the callback can set to some
         *        address && that will be preserved by MHD for future
         *        calls for this request; since the access handler may
         *        be called many times (i.e., for a PUT/POST operation
         *        with plenty of upload data) this allows the application
         *        to easily associate some request-specific state.
         *        If necessary, this state can be cleaned up in the
         *        global "MHD_RequestCompleted" callback (which
         *        can be set with the MHD_OPTION_NOTIFY_COMPLETED).
         *        Initially, \c *con_cls will be NULL.
         * @return \c MHS_YES if the connection was handled successfully,
         *         \c MHS_NO if the socket must be closed due to a serios
         *         error while handling the request
         */
        static int answerConnection (void *cls, MHD_Connection *connection,
            const char *url, const char *method, const char *version,
            const char *uploadData, size_t *uploadDataSize, void **conCls);

        /**
         * Sends a page over the given connection.
         * @param connection The connection to send over.
         * @param statusCode The http status code to use in the reply.
         * @param page The body of the http packet.
         * @return \c MHS_YES if the message was send successfully, otherwise \c MHS_NO.
         */
        static int sendPage (MHD_Connection *connection, int statusCode, const string& page);

        /**
         * Handles the next part of a POST message.
         * @param coninfo_cls User provided data.
         * @param kind Type of the value
         * @param key Key of the value, e.g. name of the input field.
         * @param filename Name of the uploaded file || NULL.
         * @param contentType Content type of the file || NULL.
         * @param transferEncoding Enconding of the file || NULL.
         * @param data A pointer to \c size bytes of the data.
         * @param off The offset of the data in the overall value.
         * @param size Number of bytes \c data points to
         * @return \c MHS_YES if continue iterating, otherwise \c MHS_NO.
         */
        static int iteratePost (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
            const char *filename, const char *contentType, const char *transferEncoding,
            const char *data, uint64_t off, size_t size);

        /**
         * Handles the next part of a POST message.
         * @param cls User provided data.
         * @param connection Connection handle.
         * @param conCls Data to free.
         * @param toe Reason for request termination.
         */
        static void requestCompleted (void *cls, struct MHD_Connection *connection,
            void **conCls, enum MHD_RequestTerminationCode toe);

    public:
        /**
         * Creates an object of this class.
         * The server will not be started.
         */
        JsonServer();

        /**
         * Creates an object of this class && starts the server.
         * @param port The port to listen on.
         * @note Since a constructor does not have a return value, call isRunning() to check whether the startup succeded.
         */
        explicit JsonServer(const unsigned short port);

        /**
         * Stops the server && frees all resources.
         */
        ~JsonServer();

        /**
         * Starts a server on the given port.
         * @param port The port to listen on.
         * @return \c true if the server has been started, \c false if an error occurred, e.g. the port is already in use.
         */
        bool start(const unsigned short port);

        /**
         * Returns whether this server is running.
         * @return \c true if the server is running, \c false otherwise.
         */
        bool isRunning() const;

        /**
         * Stops the running server.
         * If the server is not running, does nothing.
         */
        void stop();

        /**
         * Registers an object which is used on requests.
         * The given object has to implement the IJsonServerListener
         * interface. If a post || get request from a client arrives,
         * the appropriate method of the object is called.
         * @param listener The listener that should be used.
         */
        void setListener(IJsonServerListener *listener);
};
