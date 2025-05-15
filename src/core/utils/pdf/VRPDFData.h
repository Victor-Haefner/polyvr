#ifndef VRPDFDATA_H_INCLUDED
#define VRPDFDATA_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include "core/utils/VRUtilsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRPDFData : public std::enable_shared_from_this<VRPDFData> {
	public:
        struct Chunk {
            size_t begin = 0;
            size_t end = 0;
            size_t size = 0;

            string unpacked;

            Chunk(size_t b, size_t e) : begin(b), end(e) { size = e-b; }
        };

        struct Stream : Chunk {
            string name;

            Stream(size_t b, size_t e, string n) : name(n), Chunk(b,e) {}

            static string decompressZlib(const string& compressedData);
            string decode(const vector<char>& buffer);
        };

        struct Object : Chunk {
            size_t index = 0;
            string name;
            string header;
            string type;
            map<string, string> params;
            vector<Stream> streams;

            Object(size_t b, size_t e) : Chunk(b,e) {}
        };

    public:
        vector<Object> objects;
        map<string, int> objByName;
        map<string, vector<int>> objByType;
        vector<char> buffer;

        template<typename T>
        string extractHeader(const T& buffer, size_t beg, size_t end);
        map<string, string> extractParameters(const string header);

	public:
		VRPDFData();
		~VRPDFData();

		static VRPDFDataPtr create();
		VRPDFDataPtr ptr();

		void read(string path);

		vector<size_t> findOccurrences(const vector<char>& buffer, const string& keyword, size_t start = 0, size_t end = 0);
        vector<Object> extractObjects(const vector<char>& buffer);
        void extractObjectMetadata(const vector<char>& buffer, vector<Object>& objects);
        void extractStreams(const vector<char>& buffer, vector<Object>& objects);
        void extractPackedObjects(const vector<char>& buffer);
};

OSG_END_NAMESPACE;

#endif //VRPDFDATA_H_INCLUDED
