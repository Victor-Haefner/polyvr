#include "VRRestClient.h"
#include "VRRestResponse.h"
#include "core/utils/toString.h"
#include "core/utils/system/VRSystem.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRScene.h"

#include <iostream>
#include <future>
#include <boost/thread/recursive_mutex.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#include <curl/curl.h>
#endif

typedef boost::recursive_mutex::scoped_lock PLock;
static boost::recursive_mutex VRRestClientMtx;

using namespace OSG;

struct VRRestClient::RestPromise {
    future<void> f;
    //RestPromise(future<void>& F) : f(F) {}

    bool ready() {
        return (f.wait_for(chrono::seconds(0)) == future_status::ready);
    }
};

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

void VRRestClient::getAsync(string uri, VRRestCbPtr cb, int timeoutSecs) {
    auto job = [&](string uri, VRRestCbPtr cb, int timeoutSecs) -> void { // executed in async thread
        auto res = get(uri, timeoutSecs);
        auto fkt = VRUpdateCb::create("getAsync-finish", bind(&VRRestClient::finishAsync, this, cb, res));
        auto s = VRScene::getCurrent();
        if (s) s->queueJob(fkt);
    };

    future<void> f = async(launch::async, job, uri, cb, timeoutSecs);
    PLock lock(VRRestClientMtx);
    auto p = shared_ptr<RestPromise>(new RestPromise() );
    p->f = move(f);
    promises.push_back( p );
}

void VRRestClient::finishAsync(VRRestCbPtr cb, VRRestResponsePtr res) { // executed in main thread
    (*cb)(res);

    PLock lock(VRRestClientMtx);
    auto i = promises.begin();
    while (i != promises.end()) {
        if ((*i)->ready()) promises.erase(i++);
        else i++;
    }
}

void VRRestClient::test() {
    string req = "http://reqbin.com/echo/get/json";
    cout << " Start REST GET request test.." << endl;
    auto res = get(req);
    cout << "   response: " << res->getStatus() << endl;
    cout << "   response: " << res->getData() << endl;

    cout << " Start REST ASYNC GET request test.." << endl;
    auto cb = [&](VRRestResponsePtr res) {
        cout << "  async response at " << getTime() << endl;
        cout << "   async response: " << res->getStatus() << endl;
        cout << "   async response: " << res->getData() << endl;
    };
    cout << "  async request at " << getTime() << endl;
    getAsync(req, VRRestCb::create("asyncGet", cb));
    cout << "  async request done at " << getTime() << endl;
}
