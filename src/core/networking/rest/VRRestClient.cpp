#include "VRRestClient.h"
#include "VRRestResponse.h"

#include <curl/curl.h>
#include <iostream>

using namespace OSG;

VRRestClient::VRRestClient() {}
VRRestClient::~VRRestClient() {}

VRRestClientPtr VRRestClient::create() { return VRRestClientPtr( new VRRestClient() ); }
VRRestClientPtr VRRestClient::ptr() { return static_pointer_cast<VRRestClient>(shared_from_this()); }

size_t getOut(char *ptr, size_t size, size_t nmemb, VRRestResponse* res) {
    res->appendData(string(ptr, size*nmemb));
    return size * nmemb;
}

VRRestResponsePtr VRRestClient::get(string uri, int timeoutSecs) {
    auto res = VRRestResponse::create();
    auto curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, res.get());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &getOut);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeoutSecs);
    curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
    CURLcode c = curl_easy_perform(curl);
    if (c != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(c));
    curl_easy_cleanup(curl);
    res->setStatus("ok");
    return res;
}
