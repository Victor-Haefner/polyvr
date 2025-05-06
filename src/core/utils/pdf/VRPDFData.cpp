#include "VRPDFData.h"

#include <map>
#include <zlib.h>
#include <fstream>

using namespace OSG;

VRPDFData::VRPDFData() {}
VRPDFData::~VRPDFData() {}

VRPDFDataPtr VRPDFData::create() { return VRPDFDataPtr( new VRPDFData() ); }
VRPDFDataPtr VRPDFData::ptr() { return static_pointer_cast<VRPDFData>(shared_from_this()); }

void VRPDFData::read(string path) {
    ifstream file(path, std::ios::binary);
    buffer = std::vector<char>((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    objects = extractObjects(buffer);
    extractObjectNames(buffer, objects);
    extractStreams(buffer, objects);
}

std::vector<size_t> VRPDFData::findOccurrences(const std::vector<char>& buffer, const std::string& keyword, size_t start, size_t end) {
    //cout << "  findOccurrences " << keyword << " from " << start << " to " << end << ", N " << buffer.size() << endl;
    std::vector<size_t> positions;
    if (end == 0) end = buffer.size();
    string data(buffer.begin()+start, buffer.begin()+end);

    size_t pos = 0;
    while ((pos = data.find(keyword, pos)) != std::string::npos) {
        positions.push_back(pos + start);
        pos += keyword.length(); // Move past the last found position
    }
    //cout << "   found " << positions.size() << endl;
    return positions;
}

vector<VRPDFData::Object> VRPDFData::extractObjects(const vector<char>& buffer) {
    vector<VRPDFData::Object> objects;
    auto A = findOccurrences(buffer, " obj\r");
    auto B = findOccurrences(buffer, "\rendobj\r");

    map<size_t, string> pairs;
    for (auto a : A) pairs[a] = "obj";
    for (auto b : B) pairs[b] = "end";

    size_t start;
    bool inObj = false;
    for (auto x : pairs) {
        size_t p = x.first;
        if (x.second == "obj") {
            start = x.first;
            inObj = true;
        }

        if (x.second == "end") {
            objects.push_back( VRPDFData::Object(start, x.first, "object") );
            inObj = false;
        }
    }

    return objects;
}

void VRPDFData::extractObjectNames(const std::vector<char>& buffer, vector<VRPDFData::Object>& objects) {
    for (auto& obj : objects) {
        size_t N = 0;
        size_t a = 0;
        for (size_t i = obj.begin; i<obj.end; i++) {

            if (buffer[i-1] == '<' && buffer[i] == '<') { // "<<"
                if (N == 0) a = i-1;
                N += 1;
            }

            if (buffer[i-1] == '>' && buffer[i] == '>') { // ">>"
                N -= 1;

                if (N == 0) {
                    obj.name = string(buffer.begin()+a, buffer.begin()+i+1);
                    break;
                }
            }
        }
    }
}

void VRPDFData::extractStreams(const std::vector<char>& buffer, vector<VRPDFData::Object>& objects) {
    for (auto& obj : objects) {
        auto A = findOccurrences(buffer, "stream\r\n", obj.begin, obj.end);
        auto B = findOccurrences(buffer, "endstream", obj.begin, obj.end);

        map<size_t, string> pairs;
        for (auto a : A) pairs[a] = "obj";
        for (auto b : B) pairs[b] = "end";

        size_t start;
        bool inObj = false;
        for (auto x : pairs) {
            size_t p = x.first;
            if (x.second == "obj") {
                start = x.first + 6;
                while (buffer[start] == '\r' || buffer[start] == '\n') start++; // strip
                inObj = true;
            }

            if (x.second == "end") {
                size_t end = x.first-1;
                while (buffer[end-1] == '\r' || buffer[end-1] == '\n') end--; // strip
                obj.streams.push_back( VRPDFData::Stream(start, end, "stream") );
                inObj = false;
            }
        }
    }
}
