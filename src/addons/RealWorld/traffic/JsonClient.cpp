#include "JsonClient.h"
#include <string.h>


int curls_running = 0;

/**
 * A helper function to send the string contents over a stream
 * @param buffer Pointer to the buffer to write to.
 * @param size Size of elements in buffer. size*nitems is the maximal amount of bytes to write.
 * @param nitems Size of one element.
 * @param userp Data to a user defined structure to write.
 * @return The number of bytes written.
 */
size_t JsonClient::readStrCallback(char *buffer, size_t size, size_t nitems, void *userp)
{
    if (userp == NULL)
        return 0;

    stringBuf *buf = (stringBuf*)userp;

    int toWrite = min(size*nitems, buf->str.length() - buf->offset);

    if (toWrite < 0)
        return 0;


    strncpy(buffer, (buf->str.substr(buf->offset)).c_str(), toWrite);

    buf->offset += toWrite;

    return toWrite;
}

struct strholder {
strholder() {
    //cout << "~~~~~~~~~~~~~~ Constructing strholder @" << this << ".\n";
}
~strholder() {
    //cout << "~~~~~~~~~~~~~~ Destroying strholder @" << this << " with contents '" << str << "'.\n";
}
string str;

};

/**
 * A helper function to write the string contents taken from a stream.
 * @param buffer Pointer to the buffer to read from.
 * @param size Size of an element in buffer. size*nitems is the maximal amount of bytes to write.
 * @param nitems Number of elements.
 * @param userp Data to a user defined std::string to write to.
 * @return The number of bytes read, has to be nitems.
 */
size_t JsonClient::writeStrCallback(char *ptr, size_t size, size_t nitems, void *userp) {
    //string *str = (string*)userp;
    string *str = &((strholder*)userp)->str;

//cout << "String ptr: " << userp << "\n";
//cout << "String length: " << str->length() << "\n";
//cout << "String capacity: " << str->capacity() << "\n";
//cout << "Data size: " << size << "\n";
//cout << "Data nitems: " << nitems << "\n";
//cout << "String contents:\n===\n" << *str << "\n===\n";
//cout << "Data contents:\n===\n" << ptr << "\n===\n";
//cout << "pre-append\n";
//*/

    str->append(ptr, size*nitems);
//cout << "\npost-append\n";
    return size*nitems;
}

/**
 * Creates an object of this class.
 * The client will not be connected.
 */
JsonClient::JsonClient()
    : curl(0), curlMutex() {
}

/**
 * Creates an object of this class && sets name && port of the server.
 * @param server The name || address of the server. The portnumber can be appended as "address:port".
 */
JsonClient::JsonClient(const string& servername)
    : curl(0), curlMutex() {
    setServer(servername);
}

/**
 * Disconnects the client && frees all ressources.
 */
JsonClient::~JsonClient() {

    lock_guard<mutex> guardCurl(curlMutex);

    if (curl != NULL) {
        curl_easy_cleanup(curl);
        curl_global_cleanup();
    }
}

/**
 * Sets the name && address of the server to communicate with.
 * @param server The name || address of the server. The portnumber can be appended as "address:port".
 */
void JsonClient::setServer(const string& server) {

    lock_guard<mutex> guardCurl(curlMutex);

    if (curl == NULL) {
        CURLcode res;

        res = curl_global_init(CURL_GLOBAL_DEFAULT);

        if(res != CURLE_OK) {
            return;
        }

        // Start curl
        curl = curl_easy_init();

// Get verbose debug output for now
//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    }

    char *buf = new char[server.length() + 24];

    snprintf(buf, server.length() + 24, "http://%s/", server.c_str());

    this->address.assign(buf);

    delete [] buf;
}

/**
 * Sends data to an client.
 * @param data A JSON object to send to the server.
 * @return A JSON object with the returned value. Errors are reported within the \c error && \c error_message fields.
 */
Value JsonClient::sendData(const Value& data) {

    lock_guard<mutex> guardCurl(curlMutex);

    // Prepare the data to send
    stringBuf buf;
    buf.offset = 0;
    // Styled writer has bigger but prettier output. Useful for debugging.
    FastWriter writer;
    //StyledWriter writer;
    buf.str = writer.write(data);
//printf("TRAFFIC: Sending to server: %s\n", buf.str.c_str());
    // Prepare the return value
    Value value;

    // First set the URL that is about to receive our POST
    curl_easy_setopt(curl, CURLOPT_URL, (this->address + "json_input").c_str());

    // Now specify we want to POST data
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    // we want to use our own read function
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, readStrCallback);

    // pointer to pass to our read function
    curl_easy_setopt(curl, CURLOPT_READDATA, &buf);

    // Set the expected POST size
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, buf.str.length());

    //string recvHeader;
    //string recvBody;

    strholder recvHeader;
    strholder recvBody;

    // Send all received data to this function
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStrCallback);

    // We want the headers be written to string
    curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &recvHeader);

    // We want the body be written to this string
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &recvBody);

    // Perform the request && check for errors
//cerr << "start curl_easy_perform in line " << __LINE__ << ", now " << curls_running << " are running.\n"; curls_running++;
    if(curl_easy_perform(curl) != CURLE_OK) {
//cerr << "end curl_easy_perform in line " << __LINE__ << ", now " << curls_running << " are running.\n"; curls_running--;
        value["error"] = "COULD_NOT_SEND";
        value["error_message"] = "Could not send the data to the server.";
        return value;
    }
//cerr << "end curl_easy_perform in line " << __LINE__  << ", now " << curls_running << " are running.\n"; curls_running--;

    // Check headers if successful, we want:
    // HTTP/1.0 200 Ok
/*
OUT because lazy
   if (recvHeader.find("HTTP/") == string::npos) {
        // Fail.
        value["error"] = "INVALID_SERVER";
        value["error_message"] = "The server did not respond with a valid HTTP-response.";
        return value;
    }
    if (recvHeader.find(" 200 ") != string::npos) {
        // Everything is fine.
    } else if (recvHeader.find(" 404 ") != string::npos) {
        value["error"] = "FILE_NOT_FOUND";
        value["error_message"] = "Could not find the correct file on the server. Maybe it is no traffic simulation server.";
        return value;
    } else if (recvHeader.find(" 400 ") != string::npos) {
        value["error"] = "BAD_REQUEST";
        value["error_message"] = "The server could not understand your JSON request.";
        return value;
    } else {
        value["error"] = "UNKNOWN_ERROR";
        value["error_message"] = "In unknown error occurred on the server && the response could not be understood.";
        return value;
    }
*/
    /*string code = recvHeader.substr(9, 3);
    if (code.compare("200") == 0) {
        // Everything is fine.
    } else if (code.compare("404") == 0) {
        value["error"] = "FILE_NOT_FOUND";
        value["error_message"] = "Could not find the correct file on the server. Maybe it is no traffic simulation server.";
        return value;
    } else if (code.compare("400") == 0) {
        value["error"] = "BAD_REQUEST";
        value["error_message"] = "The server could not understand your JSON request.";
        return value;
    } else {
        value["error"] = "UNKNOWN_ERROR";
        value["error_message"] = "In unknown error occurred on the server && it could not handle your request.";
        return value;
    }*/

    Reader reader;
    //if (!reader.parse(recvBody, value)) {
    if (!reader.parse(recvBody.str, value)) {
        value["error"] = "INVALID_RESULT";
        value["error_message"] = "Could not parse returned result.";
    }
    return value;
}

/**
 * Retrieves the state of the simulator.
 * @return The state of the simulator.
 */
const JsonClient::SIMULATORSTATE JsonClient::getSimulatorState() {

    lock_guard<mutex> guardCurl(curlMutex);

    // Set the URL
    curl_easy_setopt(curl, CURLOPT_URL, (this->address + "json_output?state=getIt").c_str());


    string recvHeader;
    string recvBody;

    // Send a GET request
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    // Write no body
    curl_easy_setopt(curl, CURLOPT_READDATA, NULL);

    // Send all received data to this function
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStrCallback);

    // We want the headers be written to string
    curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &recvHeader);

    // We want the body be written to this string
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &recvBody);

    // Perform the request && check for errors
//cerr << "start curl_easy_perform in line " << __LINE__ << ", now " << curls_running << " are running.\n";  curls_running++;
    if(curl_easy_perform(curl) != CURLE_OK) {
//cerr << "end curl_easy_perform in line " << __LINE__ << ", now " << curls_running << " are running.\n";  curls_running--;
        return UNKNOWN;
    }
//cerr << "end curl_easy_perform in line " << __LINE__ << ", now " << curls_running << " are running.\n";  curls_running--;

    // Check headers if successful, we want:
    // HTTP/1.0 200 Ok
    string code = recvHeader.substr(9, 3);
    if (code.compare("200") != 0) {
        return UNKNOWN;
    }

    Reader reader;
    Value value;
    if (reader.parse(recvBody, value)) {
        if (value["serverState"].compare("running") == 0)
            return RUNNING;
        else if (value["serverState"].compare("paused") == 0)
            return PAUSED;
    }
    return UNKNOWN;
}

/**
 * Retrieves the contents of a viewarea.
 * @param id The id of the viewarea to retrieve informations about.
 * @return A JSON object with the returned value. Errors are reported within the \c error && \c error_message fields.
 */
Value JsonClient::retrieveViewareaData(const unsigned int& id) {

    lock_guard<mutex> guardCurl(curlMutex);

    Value value;
    char buffer[16];
    sprintf(buffer, "%u", id);

    // Set the URL
    curl_easy_setopt(curl, CURLOPT_URL, (this->address + "json_output?area=" + buffer).c_str());

    string recvHeader;
    string recvBody;

    // Send a GET request
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    // Write no body
    curl_easy_setopt(curl, CURLOPT_READDATA, NULL);

    // Send all received data to this function
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStrCallback);

    // We want the headers be written to string
    curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &recvHeader);

    // We want the body be written to this string
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &recvBody);

    // Perform the request && check for errors
//cerr << "start curl_easy_perform in line " << __LINE__ << ", now " << curls_running << " are running.\n";  curls_running++;
    if(curl_easy_perform(curl) != CURLE_OK) {
//cerr << "end curl_easy_perform in line " << __LINE__ << ", now " << curls_running << " are running.\n";  curls_running--;
        value["error"] = "COULD_NOT_SEND";
        value["error_message"] = "Could not send the data to the server.";
        return value;
    }
//cerr << "end curl_easy_perform in line " << __LINE__ << ", now " << curls_running << " are running.\n";  curls_running--;

    // Check headers if successful, we want:
    // HTTP/1.0 200 Ok
    string code = recvHeader.substr(9, 3);
    if (code.compare("200") == 0) {
        // Everything is fine.
    } else if (code.compare("404") == 0) {
        value["error"] = "FILE_NOT_FOUND";
        value["error_message"] = "Could not find the correct file on the server. Maybe it is no traffic simulation server.";
        return value;
    } else if (code.compare("400") == 0) {
        value["error"] = "BAD_REQUEST";
        value["error_message"] = "The server could not understand your JSON request.";
        return value;
    } else {
        value["error"] = "UNKNOWN_ERROR";
        value["error_message"] = "In unknown error occurred on the server && it could not handle your request.";
        return value;
    }

    Reader reader;
    if (!reader.parse(recvBody, value)) {
        value["error"] = "INVALID_RESULT";
        value["error_message"] = "Could not parse returned result.";
    }
    return value;
}
