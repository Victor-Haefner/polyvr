#include "VRRestClient.h"
#include "VRRestResponse.h"
#include "core/utils/toString.h"

#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#include <curl/curl.h>
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
// TODO: the response can contain binary data, the data pointer is thus not null terminated.. the size has to be prepended to the result!
#ifdef __EMSCRIPTEN__
    char* data = (char*)EM_ASM_INT({
        var uri = Module.UTF8ToString($0);

        var uri2 = "proxy.php?uri="+encodeURIComponent(uri);
        var request = new XMLHttpRequest();
        request.open("GET", uri2, false); // false means synchronously
        request.send();

        const byteCount = (Module.lengthBytesUTF8(request.responseText) + 1); // +1 for 0 char?
        const responsePtr = Module._malloc(byteCount+16); // 16 bytes for the prepended buffer size
        var byteCountStr = ("000000000000000" + byteCount).slice(-16);
        Module.stringToUTF8(byteCountStr, responsePtr, 17);
        Module.stringToUTF8(request.responseText, responsePtr+16, byteCount);

        return responsePtr;
    }, uri.c_str());
    size_t bufLength;
    string bufLengthStr = string(data,16);
    toValue(bufLengthStr, bufLength);
    res->setData( string(data+16, bufLength) );
    free(data);
    cout << "VRRestClient::get, got N bytes: " << bufLengthStr << ", -> " << bufLength << endl;
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

