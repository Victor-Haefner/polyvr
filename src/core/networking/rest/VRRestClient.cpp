#include "VRRestClient.h"
#include "VRRestResponse.h"
#include "core/utils/toString.h"
#include "core/utils/system/VRSystem.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRMutex.h"
#include "core/scene/VRScene.h"

#include <iostream>
#include <future>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#include <curl/curl.h>
#endif


using namespace OSG;

static VRMutex VRRestClientMtx;

struct VRRestClient::RestPromise {
    future<void> f;
    //RestPromise(future<void>& F) : f(F) {}

    bool ready() {
        return (f.wait_for(chrono::seconds(0)) == future_status::ready);
    }
};

VRRestClient::VRRestClient(string name) : VRNetworkClient(name) {}
VRRestClient::~VRRestClient() {
#ifndef __EMSCRIPTEN__
    if (curl) curl_easy_cleanup(curl);
#endif
}

VRRestClientPtr VRRestClient::create(string name) { return VRRestClientPtr( new VRRestClient(name) ); }
//VRRestClientPtr VRRestClient::ptr() { return dynamic_pointer_cast<VRRestClient>(shared_from_this()); }

size_t getRespData(char *ptr, size_t size, size_t nmemb, VRRestResponse* res) {
    res->appendData(string(ptr, size*nmemb));
    return size*nmemb;
}

size_t getRespHeaders(char* ptr, size_t size, size_t nitems, VRRestResponse* res) {
    size_t total_size = size * nitems;
    std::string header(ptr, total_size);
    res->appendHeader( stripString(header) );
    return total_size;
}


void setupHeaders(CURL* curl, vector<string>& headers) {
    struct curl_slist* hlist = NULL;
    for (auto h : headers) if (h != "") hlist = curl_slist_append(hlist, h.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hlist);
}

VRRestResponsePtr VRRestClient::get(string uri, int timeoutSecs, vector<string> headers) {
    auto res = VRRestResponse::create();
#ifdef __EMSCRIPTEN__
    char* data = (char*)EM_ASM_INT({
        var uri = Module.UTF8ToString($0);

        var uri2 = "proxy.php?uri="+encodeURIComponent(uri);
	console.log(uri2);
        var request = new XMLHttpRequest();
        request.open("GET", uri2, false); // false means synchronously
	request.overrideMimeType("text/plain; charset=x-user-defined");
        request.send();

        const byteCount = request.responseText.length;
        const responsePtr = Module._malloc(byteCount+16); // 16 bytes for the prepended buffer size
        var byteCountStr = ("000000000000000" + byteCount).slice(-16);
        Module.stringToUTF8(byteCountStr, responsePtr, 17);

	function putOnHeap(str, outIdx, maxBytesToWrite) {
		  var endIdx = outIdx + maxBytesToWrite;
		  for (var i = 0; i < str.length; ++i) {
			if (outIdx >= endIdx) break;
		  	HEAPU8[outIdx++] = str.charCodeAt(i);
		  }
	}

	putOnHeap(request.responseText, responsePtr+16, byteCount);
        return responsePtr;
    }, uri.c_str());
    size_t bufLength;
    string bufLengthStr = string(data,16);
    toValue(bufLengthStr, bufLength);
    res->setData( string(data+16, bufLength) );
    free(data);
    cout << "VRRestClient::get, got N bytes: " << bufLengthStr << ", -> " << bufLength << endl;
    if (bufLength < 20) cout << " VRRestClient::get, got: " << res->getData() << endl;
#else
    auto curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, res.get());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &getRespData);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, res.get());
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &getRespHeaders);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeoutSecs);
    curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
    setupHeaders(curl, headers);
    res->setHeaders({});

    CURLcode c = curl_easy_perform(curl);
    if (c != CURLE_OK) fprintf(stderr, "VRRestClient::get, curl_easy_perform() failed: %s, request was: %s\n", curl_easy_strerror(c), uri.c_str());
    curl_easy_cleanup(curl);
#endif

    res->setStatus(200);
    return res;
    //cout << " response: " << response->getStatus() << endl;
    //cout << " response: " << response->getData() << endl;
}

VRRestResponsePtr VRRestClient::post(string uri, const string& data, int timeoutSecs, vector<string> headers) {
    auto res = VRRestResponse::create();

#ifndef __EMSCRIPTEN__
    auto curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, res.get());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &getRespData);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, res.get());
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &getRespHeaders);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeoutSecs);
    curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
    setupHeaders(curl, headers);
    res->setHeaders({});

    CURLcode c = curl_easy_perform(curl);
    if (c != CURLE_OK) fprintf(stderr, "VRRestClient::post, curl_easy_perform() failed: %s, request was: %s\n", curl_easy_strerror(c), uri.c_str());
    curl_easy_cleanup(curl);
#endif

    res->setStatus(200);
    return res;
}

void VRRestClient::connectPort(string uri, int port, int timeoutSecs) {
    connect(uri+":"+toString(port), timeoutSecs);
}

void VRRestClient::connect(string uri, int timeoutSecs) { // TODO: used? deprecated?
#ifndef __EMSCRIPTEN__
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeoutSecs);
    curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
    //curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    //curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L); // set keep-alive idle time to 120 seconds
    //curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L); // interval time between keep-alive probes: 60 seconds
    //CURLcode c = curl_easy_perform(curl);
    //if (c != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s, request was: %s\n", curl_easy_strerror(c), uri.c_str());
    isConnected = true;
#endif
}

bool VRRestClient::connected() { return isConnected; }

void VRRestClient::getAsync(string uri, VRRestCbPtr cb, int timeoutSecs, vector<string> headers) { // TODO: implement correctly for wasm
#ifdef __EMSCRIPTEN__
    auto res = get(uri, timeoutSecs);
    auto fkt = VRUpdateCb::create("getAsync-finish", bind(&VRRestClient::finishAsync, this, cb, res));
    auto s = VRScene::getCurrent();
    if (s) s->queueJob(fkt);
#else
    auto job = [&](string uri, VRRestCbPtr cb, int timeoutSecs, vector<string> headers) -> void { // executed in async thread
        auto res = get(uri, timeoutSecs, headers);
        if (cb) {
            auto fkt = VRUpdateCb::create("getAsync-finish", bind(&VRRestClient::finishAsync, this, cb, res));
            auto s = VRScene::getCurrent();
            if (s) s->queueJob(fkt);
        }
    };

    future<void> f = async(launch::async, job, uri, cb, timeoutSecs, headers);
    VRLock lock(VRRestClientMtx);
    auto p = shared_ptr<RestPromise>(new RestPromise() );
    p->f = move(f);
    promises.push_back( p );
#endif
}

void VRRestClient::postAsync(string uri, VRRestCbPtr cb, const string& data, int timeoutSecs, vector<string> headers) { // TODO: implement correctly for wasm
#ifdef __EMSCRIPTEN__
    auto res = get(uri, timeoutSecs);
    auto fkt = VRUpdateCb::create("getAsync-finish", bind(&VRRestClient::finishAsync, this, cb, res));
    auto s = VRScene::getCurrent();
    if (s) s->queueJob(fkt);
#else
    auto job = [&](string uri, VRRestCbPtr cb, string data, int timeoutSecs, vector<string> headers) -> void { // executed in async thread
        auto res = post(uri, data, timeoutSecs, headers);
        if (cb) {
            auto fkt = VRUpdateCb::create("getAsync-finish", bind(&VRRestClient::finishAsync, this, cb, res));
            auto s = VRScene::getCurrent();
            if (s) s->queueJob(fkt);
        }
    };

    future<void> f = async(launch::async, job, uri, cb, data, timeoutSecs, headers);
    VRLock lock(VRRestClientMtx);
    auto p = shared_ptr<RestPromise>(new RestPromise() );
    p->f = move(f);
    promises.push_back( p );
#endif
}

void VRRestClient::finishAsync(VRRestCbPtr cb, VRRestResponsePtr res) { // executed in main thread
    (*cb)(res);

    VRLock lock(VRRestClientMtx);
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
