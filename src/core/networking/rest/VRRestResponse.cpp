#include "VRRestResponse.h"
#include "core/utils/toString.h"

using namespace OSG;

VRRestResponse::VRRestResponse() {}
VRRestResponse::~VRRestResponse() {}

VRRestResponsePtr VRRestResponse::create() { return VRRestResponsePtr( new VRRestResponse() ); }
VRRestResponsePtr VRRestResponse::ptr() { return static_pointer_cast<VRRestResponse>(shared_from_this()); }

void VRRestResponse::setStatus(int s) { status = s; }
void VRRestResponse::setHeaders(string s) { headers = s; }
void VRRestResponse::setData(string s) { data = s; }
void VRRestResponse::appendData(string s) { data += s; }

int VRRestResponse::getStatus() { return status; }
string VRRestResponse::getHeaders() { return headers; }
string VRRestResponse::getData() { return data; }

// alphanum
const char SAFE[256] = {
        /*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
        /* 0 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* 1 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* 2 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* 3 */ 1,1,1,1, 1,1,1,1, 1,1,0,0, 0,0,0,0,

        /* 4 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
        /* 5 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
        /* 6 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
        /* 7 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,

        /* 8 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* 9 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* A */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* B */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

        /* C */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* D */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* E */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* F */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};

string VRRestResponse::uriEncode(const string& s) {
   const char DEC2HEX[16 + 1] = "0123456789ABCDEF";
   const unsigned char * pSrc = (const unsigned char *)s.c_str();
   const int N = s.length();
   unsigned char* const pStart = new unsigned char[N * 3];
   unsigned char* pEnd = pStart;
   const unsigned char * const SRC_END = pSrc + N;

   for (; pSrc < SRC_END; ++pSrc) {
      if (SAFE[*pSrc]) *pEnd++ = *pSrc;
      else { // escape this char
         *pEnd++ = '%';
         *pEnd++ = DEC2HEX[*pSrc >> 4];
         *pEnd++ = DEC2HEX[*pSrc & 0x0F];
      }
   }

   string sResult((char *)pStart, (char *)pEnd);
   delete [] pStart;
   return sResult;
}


