#include "VRPDFData.h"
#include "core/utils/toString.h"

#include <map>
#include <zlib.h>
#include <fstream>
#include <iostream>
#include <regex>
#include <stack>

using namespace OSG;


string VRPDFData::Stream::decompressZlib(const string& compressedData) {
    uLong decompressedSize = compressedData.size() * 4; // A rough guess
    vector<char> decompressedBuffer(decompressedSize);
    int result = uncompress(reinterpret_cast<Bytef*>(decompressedBuffer.data()), &decompressedSize,
                            reinterpret_cast<const Bytef*>(compressedData.data()), compressedData.size());
    if (result != Z_OK) { cerr << "Decompression failed with error: " << result << endl; return ""; }
    return string(decompressedBuffer.begin(), decompressedBuffer.begin() + decompressedSize);
}

string VRPDFData::Stream::decode(const std::vector<char>& buffer) {
    string data(buffer.begin()+begin, buffer.begin()+end);
    return decompressZlib(data);
}


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
    extractPackedObjects(buffer);
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

template<typename T>
pair<size_t,int> findNext(const T& buffer, vector<string> strings, size_t b, size_t e) {
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
            objects[objects.size()-1].index = objects.size()-1;
            inObj = false;
            //indent = indent.substr(0, indent.size()-1);
            //cout << indent << x.first << " - " << x.second << endl;
        }
    }

    return objects;
}

template<typename T>
string VRPDFData::extractHeader(const T& buffer, size_t beg, size_t end) {
    size_t a0 = 0;
    int level = 0;
    vector<string> brackets = {"<<", ">>"};

    for (size_t i = beg; i<end; i++) {
        auto f = findNext(buffer, brackets, i, end);
        if (f.second == -1) continue;
        i = f.first;

        if (f.second == 0) { // found "<<"
            level += 1;
            if (a0 == 0) a0 = f.first + 2;
            //cout << " found << at " << a0 << ", level: " << level << endl;
        } else if (f.second == 1) { // found ">>"
            level -= 1;
            //cout << " found >> at " << f.first << ", level: " << level << endl;
            if (level == 0) return string( buffer.begin() + a0, buffer.begin() + f.first );
        }
    }

    return "";
}

void VRPDFData::extractObjectMetadata(const vector<char>& buffer, vector<VRPDFData::Object>& objects) {
    for (auto& obj : objects) {

        // extract object ID
        for (size_t i=0; true; i++) {
            auto c = buffer[obj.begin - i];
            if (c == '\r' || c == '\n') {
                auto beg = buffer.begin()+obj.begin;
                obj.name = splitString( string(beg-i+1, beg), ' ' )[0];
                objByName[obj.name] = obj.index;
                break;
            }
            if (i > 30) break;
        }

        obj.header = extractHeader(buffer, obj.begin, obj.end);

        auto parts = splitString(obj.header, '/');
        for (size_t i=1; i<parts.size(); i++) {
            if (parts[i-1] == "Type") {
                obj.type = parts[i];
                objByType[obj.type].push_back( obj.index );
            }
        }

        cout << " pdf object: " << obj.name << ", " << obj.type << " - " << obj.header << endl;
    }
}

void VRPDFData::extractStreams(const vector<char>& buffer, vector<VRPDFData::Object>& objects) {
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


#define pdfPtrFwd( X ) \
struct X; \
typedef std::shared_ptr<X> X ## Ptr; \
typedef std::weak_ptr<X> X ## WeakPtr; \

pdfPtrFwd(PDFPair);
pdfPtrFwd(PDFValue);
pdfPtrFwd(PDFString);
pdfPtrFwd(PDFArray);
pdfPtrFwd(PDFDict);

struct PDFValue {
    int type = -1;
    virtual string toString() = 0;
    virtual string structAsString(string indent) = 0;
};

struct PDFPair {
    PDFPairWeakPtr parent;
    string key;
    PDFValuePtr val;
    string toString() { return key + " : " + string(val?val->toString():string()); }
    string structAsString(string indent) { return "\n"+indent+"pair (" + key + "):" + string(val?val->structAsString(indent+" "):string()); }
    static PDFPairPtr create() { return PDFPairPtr(new PDFPair); };
};

struct PDFString : PDFValue {
    string str;
    PDFString() { type = 1; }
    string toString() { return str; }
    string structAsString(string indent) { return "\n"+indent+"string (" + str + ")"; }
    static PDFStringPtr create() { return PDFStringPtr(new PDFString); };
};

struct PDFArray : PDFValue {
    vector<PDFPairPtr> params;
    string toString() { string s = " ["; for(auto& p : params) s += p->toString()+", "; s += "] "; return s; }
    PDFArray() { type = 2; }
    static PDFArrayPtr create() { return PDFArrayPtr(new PDFArray); };

    string structAsString(string indent) {
        string s = "\n"+indent+"array [";
        for(auto& p : params) if (p) s += p->structAsString(indent+" ");
        s += "\n"+indent+"]";
        return s;
    }
};

struct PDFDict : PDFValue {
    map<string, PDFPairPtr> params;
    string toString() { string s = " {"; for(auto& p : params) s += p.second->toString()+"; "; s += "} "; return s; }
    PDFDict() { type = 3; }
    static PDFDictPtr create() { return PDFDictPtr(new PDFDict); };

    string structAsString(string indent) {
        string s = "\n"+indent+"dict {";
        for(auto& p : params) if (p.second) s += p.second->structAsString(indent+" ");
        s += "\n"+indent+"}";
        return s;
    }
};

vector<PDFPairPtr> stackToVector(stack<PDFPairPtr> s) {
    size_t N = s.size();
    vector<PDFPairPtr> v(N);
    for (int i=0; i<N; i++) {
        v[N-i-1] = s.top();
        s.pop();
    }
    return v;
}

vector<PDFValuePtr> parseHeader(const string header) {
    vector<PDFValuePtr> roots;
    PDFValuePtr currentRoot;
    stack<PDFPairPtr> context;
    bool nextIsName = false;
    bool firstElement = false;

    auto endPair = [&]() {
        if (context.size() == 0) return;
        cout << " endPair" << endl;

        auto current = context.top();

        if (auto parent = current->parent.lock()) {
            if (auto a = dynamic_pointer_cast<PDFArray>(parent->val)) {
                cout << "  append '" << current->key << "' to array " << parent->key << endl;
                a->params.push_back( current );
            }

            else if (auto d = dynamic_pointer_cast<PDFDict> (parent->val)) {
                cout << "  append '" << current->key << "' to dict " << parent->key << endl;
                d->params[current->key] = current;
            }

            else cout << "Warning! parent is neither array nor dictionary!" << endl;
        } else if (currentRoot) {
            if (auto a = dynamic_pointer_cast<PDFArray>(currentRoot)) {
                cout << "  append '" << current->key << "' to array root" << endl;
                a->params.push_back( current );
            }

            else if (auto d = dynamic_pointer_cast<PDFDict> (currentRoot)) {
                cout << "  append '" << current->key << "' to dict root" << endl;
                d->params[current->key] = current;
            }

            else cout << "Warning! root is neither array nor dictionary!" << endl;
        }

        context.pop();
    };

    auto endArray = [&]() {
        endPair();
        cout << "endArray" << endl;
    };

    auto endDict  = [&]() {
        endPair();
        cout << "endDict" << endl;
    };



    auto newValue = [&](PDFValuePtr p) {
        if (context.size() > 0) {
            auto current = context.top();
            if (current) {
                cout << " set val of pair " << current->key << endl;
                current->val = p;
                return;
            }
        }

        roots.push_back(p);
    };

    auto startArray = [&]() {
        cout << "startArray" << endl;
        auto a = PDFArray::create();
        newValue(a);
        firstElement = true;
    };

    auto startDict = [&]() {
        cout << "startDict" << endl;
        auto d = PDFDict::create();
        newValue(d);
        firstElement = true;
    };

    auto startPair = [&]() {
        if (!firstElement) endPair();
        firstElement = false;

        cout << "startPair" << endl;
        PDFPairPtr parent;
        if (context.size() > 0) parent = context.top();

        auto p = PDFPair::create();
        if (parent) p->parent = parent;
        context.push(p);
        nextIsName = true;
    };


    auto processCharacter = [&](char newC) {
        auto currentPair = context.top();
        if (nextIsName) currentPair->key += newC;
        else {
            if (!currentPair->val) currentPair->val = PDFString::create();
            if (auto v = dynamic_pointer_cast<PDFString>(currentPair->val)) {
                v->str += newC;
            }
        }
    };


    size_t pos = 0;
    char lastC = 0;

    cout << " process header: " << header << endl;

    while (pos < header.size()) {
        char newC = header[pos];
        //cout << "  " << pos << ", '" << newC << "' - " << context.size() << endl;
        //cout << "  " << pos << ", '" << newC << "/" << lastC << "' - " << context.size() << " -: ";
        //for (auto s : stackToVector(context)) cout << " " << s->type;

        if (newC == '<')      { if (lastC == '<') { startDict(); nextIsName = false; newC = 0; } } // avoid <<< issue
        else if (newC == '>') { if (lastC == '>') { endDict();   nextIsName = false; newC = 0; } } // avoid >>> issue
        else if (newC == '[') { nextIsName = false; startArray(); }
        else if (newC == ']') { nextIsName = false; endArray(); }
        else if (newC == '/' && !nextIsName) startPair();
        else if (newC == ' ' &&  nextIsName) nextIsName = false;
        else if (newC != 0) processCharacter(newC);
        lastC = newC;
        pos++;
    }

    cout << endl << "N roots: " << roots.size() << endl;
    for (auto root : roots) {
        cout << root->toString() << endl;
        cout << root->structAsString("") << endl;
    }
    return roots;
}



void VRPDFData::extractPackedObjects(const vector<char>& buffer) {
    if (!objByType.count("ObjStm")) return;

    for (auto& objI : objByType["ObjStm"]) {
        auto& obj = objects[objI];
        cout << "ObjStm header: " << obj.header << endl;
        for (auto& s : obj.streams) {
            string data = s.decode(buffer);
            string objIDsStr = splitString(data, "<<")[0];
            vector<string> objIDs = splitString( objIDsStr, ' ' );
            string objHeaders = subString(data, objIDsStr.size(), data.size()-objIDsStr.size());
            cout << objHeaders << endl;
            //auto roots = parseHeader(objHeaders);
            //auto roots = parseHeader("<</ColorSpace<</Cs6 166 0 R>>/ExtGState<</GS1 167 0 R>>/Font<</TT1 170 0 R/TT2 173 0 R/TT4 175 0 R/TT5 178 0 R>>/ProcSet[/PDF/Text/ImageC]/XObject<</Im1 162 0 R/Im2 163 0 R>>>>[/ICCBased 154 0 R]<</OP false/OPM 1/SA false/SM 0.02/Type/ExtGState/op false>>");
            //auto roots = parseHeader("<</ColorSpace<</Cs14 133 0 R/Cs6 166 0 R>>/ExtGState<</GS1 167 0 R>>/Font<</TT1 170 0 R/TT2 173 0 R/TT4 175 0 R/TT5 178 0 R>>/ProcSet[/PDF/Text/ImageC/ImageI]/XObject<</Im43 73 0 R/Im44 74 0 R/Im45 75 0 R/Im46 76 0 R/Im47 77 0 R>>>>");
            //auto roots = parseHeader("<</ColorSpace<</Cs14 133 0 R/Cs6 166 0 R>>/ExtGState<</GS1 167 0 R>>>>");
            //auto roots = parseHeader("<</A<</B 7/C 8>>/D<</E 3/F [/G 5/H 6]>>>>");
            auto roots = parseHeader("<</A 7/B 8>>");

            return;


            cout << " ObjStm data: " << data << endl;


            for (int i=1; i<objIDs.size(); i+=2) {
                string oID = objIDs[i-1];
                if (oID == "165" || oID == "166") {
                    int offset = toInt( objIDs[i] );
                    string objData = extractHeader(objHeaders, offset, objHeaders.size());
                    cout << "  Object: " << oID << ", data: " << objData << endl;
                }
            }
        }
    }
}

