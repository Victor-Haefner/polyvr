#ifndef VRPDFDATA_H_INCLUDED
#define VRPDFDATA_H_INCLUDED

#include <OpenSG/OSGConfig.h>
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
        };

        struct Object : Chunk {
            string name;
            string header;
            string type;
            vector<Stream> streams;

            Object(size_t b, size_t e) : Chunk(b,e) {}
        };

    public:
        vector<Object> objects;
        std::vector<char> buffer;

	public:
		VRPDFData();
		~VRPDFData();

		static VRPDFDataPtr create();
		VRPDFDataPtr ptr();

		void read(string path);

		std::vector<size_t> findOccurrences(const std::vector<char>& buffer, const std::string& keyword, size_t start = 0, size_t end = 0);
        vector<Object> extractObjects(const vector<char>& buffer);
        void extractObjectMetadata(const std::vector<char>& buffer, vector<Object>& objects);
        void extractStreams(const std::vector<char>& buffer, vector<Object>& objects);
};

OSG_END_NAMESPACE;

#endif //VRPDFDATA_H_INCLUDED
