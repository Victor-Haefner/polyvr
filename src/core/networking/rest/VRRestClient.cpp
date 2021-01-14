#include "VRRestClient.h"
#include "VRRestResponse.h"
#include "core/utils/toString.h"

#include <curl/curl.h>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

using namespace OSG;

template<> string typeName(const VRRestClient& p) { return "RestClient"; }

VRRestClient::VRRestClient() {}
VRRestClient::~VRRestClient() {}

VRRestClientPtr VRRestClient::create() { return VRRestClientPtr( new VRRestClient() ); }
VRRestClientPtr VRRestClient::ptr() { return static_pointer_cast<VRRestClient>(shared_from_this()); }

size_t getOut(char *ptr, size_t size, size_t nmemb, VRRestResponse* res) {
    res->appendData(string(ptr, size*nmemb));
    return size*nmemb;
}

VRRestResponsePtr VRRestClient::get(string uri, int timeoutSecs) {
    auto res = VRRestResponse::create();

#ifdef __EMSCRIPTEN__
    EM_ASM({
        var request = new XMLHttpRequest();
        request.open("GET", $0, false); // false means synchronously
        request.send();
        if (request.status === 200) {
            console.log(request.responseText);
        }
    }, uri);
#else
    auto curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, res.get());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &getOut);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeoutSecs);
    curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
    CURLcode c = curl_easy_perform(curl);
    if (c != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(c));
    curl_easy_cleanup(curl);
#endif

    res->setStatus("ok");
    return res;
    //cout << " response: " << response->getStatus() << endl;
    //cout << " response: " << response->getData() << endl;
}

void VRRestClient::test() {
    string req = "http://reqbin.com/echo/get/json";
    cout << " Start REST GET request test.." << endl;
    auto res = get(req);
    cout << " response: " << res->getStatus() << endl;
    cout << " response: " << res->getData() << endl;
}

