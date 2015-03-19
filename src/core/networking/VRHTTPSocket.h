#ifndef VRHTTPSOCKET_H_INCLUDED
#define VRHTTPSOCKET_H_INCLUDED

#include <algorithm>
#include <curl/curl.h>
#include <microhttpd.h>
#include <jsoncpp/json/json.h>
//#include "../signals/VRCallbackManager.h"

OSG_BEGIN_NAMESPACE
using namespace std;

//MICROHTTPD server-------------------------------------------------------------
typedef VRFunction<map<string, string>*> VRFunc_serv;

void server_answer_job(VRFunc_serv* fkt, map<string, string>* params, int i) { (*fkt)(params); }

struct server_answer_data {
    map<string, VRFunc_serv* >* cb_map;
    map<string, string>* params;
    string* page;
};

int server_parseURI(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
    map<string, string>* uri_map = (map<string, string>*)cls;
    (*uri_map)[string(key)] = string(value);
    printf ("GET %s: %s\n", key, value);
    return MHD_YES;
}

int server_parseFORM(void *cls, enum MHD_ValueKind kind, const char *key, const char *filename, const char *content_type, const char *transfer_encoding, const char *data, uint64_t off, size_t size) {
    map<string, string>* uri_map = (map<string, string>*)cls;
    (*uri_map)[string(key)] = string(data);
    printf ("POST %s: %s\n", key, data);
    return MHD_YES;
}

int server_answer_to_connection (void* param, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **opt) {
    server_answer_data* sad = (server_answer_data*) param;
    string method_s(method);//GET, POST, ...
    string section(url+1);
    const char* page = sad->page->c_str();
    int size = sad->page->size();

    if (sad->cb_map->count(section) != 0) {
        if (method_s == "GET")
            MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, server_parseURI, sad->params);//Parse URI parameter
        if (method_s == "POST") {
            struct MHD_PostProcessor* pp = (MHD_PostProcessor*)(*opt);
            if (pp == NULL) {
                sad->params->clear();
                pp = MHD_create_post_processor(connection, 1024, server_parseFORM, sad->params);
                *opt = pp;
                return MHD_YES;
            }
            if (*upload_data_size) {
                MHD_post_process(pp, upload_data, *upload_data_size);
                *upload_data_size = 0;
                return MHD_YES;
            } else {
                MHD_destroy_post_processor(pp);
            }
        }

        VRFunction<int>* _fkt = new VRFunction<int>("HTTP_answer_job", boost::bind(server_answer_job, (*(sad->cb_map))[section], sad->params, _1));
        VRSceneManager::get()->queueJob(_fkt);
        page = "Ok";
        size = 2;
    }

    //----------------------------------------------

    struct MHD_Response *response;
    response = MHD_create_response_from_data (size, (void*) page, MHD_NO, MHD_YES);
    int ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);
    return ret;
}


//CURL HTTP client--------------------------------------------------------------
size_t httpwritefkt( char *ptr, size_t size, size_t nmemb, void *userdata) {
    string* s = (string*)userdata;
    s->append(ptr, size*nmemb);
    return size*nmemb;
}

class VRHTTPSocket {
    private:

        //server----------------------------------------------------------------
        struct MHD_Daemon* server;
        string* indexPage;
        map<string, string>* desc_map;
        map<string, VRFunc_serv* >* Server_callbacks;
        map<string, VRFunc_serv* >::iterator server_itr;

        void initServer(int port = 8082) {
            Server_callbacks = new map<string, VRFunc_serv* >();
            desc_map = new map<string, string>();
            indexPage = new string();

            server_answer_data* sad = new server_answer_data();
            sad->cb_map = Server_callbacks;
            sad->page = indexPage;
            sad->params = new map<string, string>();

            cout << "\n\nStart HTTP Server" << flush;
            server = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, port, NULL, NULL, &server_answer_to_connection, sad, MHD_OPTION_END);
            cout << "\n ..done" << flush;
        }

        void updateIndexPage() {
            string page = "<html><body>PolyVR HTTP bindings<br><br><table>";

            for(server_itr = Server_callbacks->begin(); server_itr != Server_callbacks->end(); server_itr++) {
                page += "<br><tr><td><a href=\"/";
                page += server_itr->first;
                page += "\">";
                page += server_itr->first;
                page += "</a></td><td width=\"100\"></td><td>";
                page += desc_map->at(server_itr->first);
                page += "</td></tr>";
            }

            page += "</table></body></html>";
            *indexPage = page;
        }

        //client----------------------------------------------------------------
        CURL* curl;

        void checkResult(CURLcode res) {
            if(res != CURLE_OK)
                  fprintf(stderr, "\ncurl_easy_perform() failed: %s\n",
                          curl_easy_strerror(res));
        }

        void initClient() { ; }

    protected:
        VRHTTPSocket() {
            curl = 0;
            server = 0;
            initClient();
            initServer();
            updateIndexPage();
        }

    public:

        static VRHTTPSocket* get() {
            static VRHTTPSocket* singleton_opt = 0;
            if (singleton_opt == 0) singleton_opt = new VRHTTPSocket();
            return singleton_opt;
        }

        enum{GET, PUSH, POST, PUT};

        typedef VRFunction<map<string, string>*> callback;

        string connect(string server, int httpreq, string cmd, string message) {
            curl = curl_easy_init();

            switch(httpreq) {
                case GET:
                    replaceInString(message, ' ', "%20");
                    server += "/" + cmd + "?" + message;
                    break;
                case POST:
                    server += "/" + cmd;
                    curl_easy_setopt(curl, CURLOPT_POST, 1);
                    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, message.c_str());
                    break;
            }


            curl_easy_setopt(curl, CURLOPT_URL, server.c_str());//scheme://host:port/path
            //curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, server.c_str());//use this to parse the header lines!

            string* ss = new string();

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httpwritefkt);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA , ss);
            CURLcode tmp = curl_easy_perform(curl);
            checkResult(tmp);
            curl_easy_cleanup(curl);

            string s = *ss;
            delete ss;
            return s;
        }

        void addServerCallback(string url, VRFunc_serv* fkt, string description) {
            (*Server_callbacks)[url] = fkt;
            (*desc_map)[url] = description;
            updateIndexPage();
        }

        //---------utility functions---------------
        void replaceInString(string &s, char c, string ss) {
            int i = 0;
            while(true) {
                i = s.find(c);
                if (i == -1) break;
                s.erase(i, 1);
                s.insert(i, ss);
            }
        }

        template<typename T>
        vector<T> convertStringToVector(string s, char d, bool verbose = false) {
            replaceInString(s, d, " ");
            stringstream ss(s);
            T tmp;
            vector<T> vec;
            while (ss >> tmp) vec.push_back(tmp);

            if (verbose) for (uint i=0;i<vec.size();i++) cout << "\nElement: " << vec[i];

            return vec;
        }

        template<typename T>
        string convertVectorToString(vector<T>* vec, char d, bool verbose = false) {
            stringstream ss;
            for (uint i=0;i<vec->size();i++) {
                ss << vec->at(i);
                if (i < vec->size() -1) ss << d;
            }

            string s = ss.str();
            if (verbose) cout << "\nString: " << s;

            return s;
        }

        Json::Value parseStringToJson(string s, string name) {
            Json::Value root;   // will contains the root value after parsing.
            Json::Reader reader;
            bool parsingSuccessful = reader.parse(s, root, false);
            if ( !parsingSuccessful ) {
                // report to the user the failure && their locations in the document.
                std::cout  << "Failed to parse configuration\n"
                           << reader.getFormattedErrorMessages();
            }

            Json::Value res = root[name];
            return res;
        }
};

OSG_END_NAMESPACE

#endif // VRHTTPSOCKET_H_INCLUDED
