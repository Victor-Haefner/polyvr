#include "JsonServer.h"

#include <microhttpd.h>
#include <jsoncpp/json/json.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;
using namespace Json;


// Code based on largepost.c sample of the libmicrohttpd tutorials


const char *page_not_found = "<html><head><title>" JSON_PAGE_TITLE " - JSON interface</title></head><body><h2>This is the " JSON_PAGE_TITLE " JSON interface.</h2>The requested page could not be found on this server. Since you do not know what to look for, you are probably not right here and want to try somewhere else.</body></html>";
const char *page_bad_request = "<html><head><title>" JSON_PAGE_TITLE " - JSON interface</title></head><body><h2>This is the " JSON_PAGE_TITLE " JSON interface.</h2>The given JSON-Data is invalid and could not been parsed.</body></html>";


int JsonServer::answerConnection (void *cls, MHD_Connection *connection,
    const char *url, const char *method, const char*,
    const char *uploadData, size_t *uploadDataSize, void **conCls) {

    JsonServer *obj = (JsonServer*) cls;

    if (strcmp(method, "GET") == 0) {

        // Check if the right file has been requested
        if (strcmp(url, "/json_output") == 0) {
            // The value that will be returned
            Value value;
            // A JSON::Writer to transform the Value into a string
            Writer *w = obj->writer;

            // Retrieve the id of the viewarea, if any
            const char *val = MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "area");
            if (val != NULL) {
                int id = atoi(val);

                if (obj->listener != NULL) {
                    value = obj->listener->getViewareaData(id);
                } else {
                    value["error"] = "INTERNAL_ERROR";
                    value["error_message"] = "No handler is registered for this event.";
                }
            }

            // If requested, add the state of the simulator
            if (MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "state") != NULL && obj->listener != NULL) {
                // Add a state message to result
                IJsonServerListener::SIMULATORSTATE state = obj->listener->getSimulatorState();
                if (state == IJsonServerListener::RUNNING)
                    value["serverState"] = "running";
                else if (state == IJsonServerListener::PAUSED)
                    value["serverState"] = "paused";
                else
                    value["serverState"] = "unknown";
            }
            return sendPage(connection, MHD_HTTP_OK, w->write(value).c_str());
        }
    }

    // Check if the right file has been requested
    if (strcmp(method, "POST") == 0 && strcmp(url, "/json_input") == 0) {

        // If the string has not been created yet, create it
        if (*conCls == NULL) {
            string *str = new string;

            if (str == NULL)
                return MHD_NO;

            *conCls = (void*)str;

            return MHD_YES;
        }

        string *str = (string*)*conCls;
        if (*uploadDataSize != 0) {
            str->append(uploadData, *uploadDataSize);
            *uploadDataSize = 0;

            return MHD_YES;
        } else {
            Value value;
            if (!obj->reader->parse(*str, value)) {
                return sendPage(connection, MHD_HTTP_BAD_REQUEST, page_bad_request);
            }

            if (obj->listener != NULL) {
                value = obj->listener->handlePostRequest(value);
            } else {
                value.clear();
                value["error"] = "INTERNAL_ERROR";
                value["error_message"] = "No handler is registered for this event.";
            }

            Writer *w = obj->writer;
            return sendPage(connection, MHD_HTTP_OK, w->write(value).c_str());
        }
    }

    return sendPage(connection, MHD_HTTP_NOT_FOUND, page_not_found);
}

int JsonServer::sendPage (MHD_Connection *connection, int statusCode, const string& page) {

    int ret;
    MHD_Response *response;

    response = MHD_create_response_from_buffer (page.length(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
    if (!response)
        return MHD_NO;

    ret = MHD_queue_response (connection, statusCode, response);
    MHD_destroy_response (response);

    return ret;
}

void JsonServer::requestCompleted (void*, MHD_Connection*,
    void **conCls, MHD_RequestTerminationCode) {

    string *str = (string*)*conCls;

    if (str == NULL)
        return;

    delete str;
    *conCls = NULL;
}

JsonServer::JsonServer()
    : daemon(NULL), writer(new FastWriter()), reader(new Reader()), listener(NULL) {
}

JsonServer::JsonServer(const unsigned short port)
    : daemon(NULL), writer(new FastWriter()), reader(new Reader()), listener(NULL)  {
    start(port);
}

JsonServer::~JsonServer() {
    stop();
    if (reader != NULL)
        delete reader;
    if (writer != NULL)
        delete writer;
}

bool JsonServer::start(const unsigned short port) {
    if (daemon != NULL)
        stop();

    daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, port, NULL, NULL,
        &answerConnection, this,
        MHD_OPTION_NOTIFY_COMPLETED, requestCompleted, this,
        MHD_OPTION_END);
    return daemon != NULL;
}

bool JsonServer::isRunning() const {
    return daemon != NULL;
}

void JsonServer::stop() {
    if (daemon != NULL) {
        MHD_stop_daemon(daemon);
        daemon = NULL;
    }
}

void JsonServer::setListener(IJsonServerListener *listener) {
    this->listener = listener;
}
