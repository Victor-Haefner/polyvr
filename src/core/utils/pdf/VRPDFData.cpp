#include "VRPDFData.h"
#include "core/utils/toString.h"

#include <map>
#include <zlib.h>
#include <fstream>
#include <iostream>
#include <regex>

using namespace OSG;

VRPDFData::VRPDFData() {}
VRPDFData::~VRPDFData() {}

VRPDFDataPtr VRPDFData::create() { return VRPDFDataPtr( new VRPDFData() ); }
VRPDFDataPtr VRPDFData::ptr() { return static_pointer_cast<VRPDFData>(shared_from_this()); }

void VRPDFData::read(string path) {
    ifstream file(path, std::ios::binary);
    buffer = std::vector<char>((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    objects = extractObjects(buffer);
    extractObjectMetadata(buffer, objects);
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

pair<size_t,int> findNext(const std::vector<char>& buffer, vector<string> strings, size_t b, size_t e) {
    if (e == 0) e = buffer.size();

    auto isMatchAt = [&](const string& s, size_t i) {
        for (size_t j=0; j<s.size(); j++) {
            if (i+j >= e) return false;
            if (buffer[i+j] != s[j]) return false;
        }
        return true;
    };

    for (size_t i=b; i<e; i++) {
        for (size_t k=0; k<strings.size(); k++) {
            if (isMatchAt(strings[k], i)) return pair<size_t,int>(i,k);
        }
    }

    return pair<size_t,int>(0,-1);
}

vector<VRPDFData::Object> VRPDFData::extractObjects(const vector<char>& buffer) {
    //cout << "VRPDFData::extractObjects " << endl;

    vector<VRPDFData::Object> objects;
    auto A = findOccurrences(buffer, " obj\r");
    auto B = findOccurrences(buffer, "\rendobj\r");

    map<size_t, string> pairs;
    for (auto a : A) pairs[a] = "obj";
    for (auto b : B) pairs[b] = "end";

    //string indent = " ";
    size_t start;
    bool inObj = false;
    for (auto x : pairs) {
        size_t p = x.first;
        if (x.second == "obj") {
            start = x.first;
            inObj = true;
            //cout << indent << x.first << " - " << x.second << endl;
            //indent += " ";
        }

        if (x.second == "end") {
            objects.push_back( VRPDFData::Object(start, x.first) );
            inObj = false;
            //indent = indent.substr(0, indent.size()-1);
            //cout << indent << x.first << " - " << x.second << endl;
        }
    }

    return objects;
}

void VRPDFData::extractObjectMetadata(const std::vector<char>& buffer, vector<VRPDFData::Object>& objects) {
    for (auto& obj : objects) {

        // extract object ID
        for (size_t i=0; true; i++) {
            auto c = buffer[obj.begin - i];
            if (c == '\r' || c == 'n') {
                obj.name = string(buffer.begin()+obj.begin-i+1, buffer.begin()+obj.begin);
                break;
            }
            if (i > 10) break;
        }

        // extract header
        size_t a0 = 0;
        int level = 0;
        vector<string> brackets = {"<<", ">>"};
        for (size_t i = obj.begin; i< obj.end; i++) {
            auto f = findNext(buffer, brackets, i, obj.end);
            if (f.second == -1) continue;
            i = f.first;

            if (f.second == 0) { // found "<<"
                level += 1;
                if (a0 == 0) a0 = f.first + 2;
            } else if (f.second == 1) { // found ">>"
                level -= 1;
                if (level == 0) {
                    obj.header = string( buffer.begin() + a0, buffer.begin() + f.first );
                    break;
                }
            }
        }

        auto parts = splitString(obj.header, '/');
        for (size_t i=1; i<parts.size(); i++) {
            if (parts[i-1] == "Type") obj.type = parts[i];
        }


        //cout << " pdf obj header " << obj.header << endl;
        cout << " pdf object: " << obj.name << ", " << obj.type << endl;


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
