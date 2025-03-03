#include "VRPDF.h"
#include "core/utils/toString.h"
#include "core/math/pose.h"
#include "core/math/partitioning/boundingbox.h"
#include "core/math/Layer2D.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"

#ifdef _WIN32
#include <cairo/cairo-pdf.h>
#else
#include <cairo-pdf.h>
#endif

#include <zlib.h>

using namespace OSG;

VRPDF::VRPDF(string path) {
    //cairo_surface_t* surface = cairo_pdf_surface_create(path.c_str(), W*res, H*res);
    surface = cairo_pdf_surface_create(path.c_str(), 0, 0);
    cr = cairo_create(surface);
}

VRPDF::~VRPDF() {
    if (surface) cairo_surface_destroy(surface);
    if (cr) cairo_destroy(cr);
}

VRPDFPtr VRPDF::create(string path) { return VRPDFPtr( new VRPDF(path) ); }

void VRPDF::drawLine(Pnt2d p1, Pnt2d p2, Color3f c1, Color3f c2) {
    cairo_set_source_rgb(cr, c1[0], c1[1], c1[2]);
    cairo_set_line_width(cr, 0.5);
    cairo_move_to(cr, p1[0]*res, p1[1]*res);
    cairo_set_source_rgb(cr, c2[0], c2[1], c2[2]);
    cairo_line_to(cr, p2[0]*res, p2[1]*res);
    cairo_stroke(cr);
}

void VRPDF::drawText() {
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 40.0);
    cairo_move_to(cr, 10.0, 50.0);
    cairo_show_text(cr, "Disziplin ist Macht.");
}

void VRPDF::write(string path) {
    cairo_show_page(cr);
}

void VRPDF::project(VRObjectPtr obj, PosePtr plane) {
    BoundingboxPtr bb = obj->getBoundingbox();
    W = abs( bb->size().dot(plane->x()) );
    H = abs( bb->size().dot(plane->up()) );
    Ox = min( bb->min().dot(plane->x()),  bb->max().dot(plane->x())  );
    Oy = min( bb->min().dot(plane->up()), bb->max().dot(plane->up()) );

    cout << "VRPDF::project, " << Vec4d(W,H,Ox,Oy) << endl;
    Vec2d O = Vec2d(Ox,Oy);

    cairo_pdf_surface_set_size(surface, W*res, H*res);
    Layer2D projection;
    projection.project(obj, plane);
    for (auto l : projection.getLines()) drawLine(l.p1-O, l.p2-O, l.c1, l.c2);
}

void VRPDF::slice(VRObjectPtr obj, PosePtr plane) {
    // TODO
}

namespace PDF {
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
        vector<Stream> streams;

        Object(size_t b, size_t e, string n) : name(n), Chunk(b,e) {}
    };
}

std::vector<size_t> findOccurrences(const std::vector<char>& buffer, const std::string& keyword, size_t start = 0, size_t end = 0) {
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

vector<PDF::Object> extractObjects(const vector<char>& buffer) {
    vector<PDF::Object> objects;
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
            objects.push_back( PDF::Object(start, x.first, "object") );
            inObj = false;
        }
    }

    return objects;
}

void extractObjectNames(const std::vector<char>& buffer, vector<PDF::Object>& objects) {
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

void extractStreams(const std::vector<char>& buffer, vector<PDF::Object>& objects) {
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
                obj.streams.push_back( PDF::Stream(start, end, "stream") );
                inObj = false;
            }
        }
    }
}

vector<PDF::Object> filter3DObjects(vector<PDF::Object>& objects) {
    vector<PDF::Object> res;

    for (auto& obj : objects) {
        if ( contains(obj.name, "PRC") ) res.push_back(obj);
    }

    return res;
}



std::string decompressZlib(const std::string& compressedData) {
    uLong decompressedSize = compressedData.size() * 4; // A rough guess
    std::vector<char> decompressedBuffer(decompressedSize);
    int result = uncompress(reinterpret_cast<Bytef*>(decompressedBuffer.data()), &decompressedSize,
                            reinterpret_cast<const Bytef*>(compressedData.data()), compressedData.size());
    if (result != Z_OK) { std::cerr << "Decompression failed with error: " << result << std::endl; return ""; }
    return std::string(decompressedBuffer.begin(), decompressedBuffer.begin() + decompressedSize);
}

void unpack3DObject(const std::vector<char>& buffer, PDF::Object& object) {
    if (object.streams.size() == 0) return;
    PDF::Stream& stream = object.streams[0];
    string data(buffer.begin()+stream.begin, buffer.begin()+stream.end);
    data = decompressZlib(data);
    if (!startsWith(data, "PRC")) return;
    stream.unpacked = data;
}

void printObjects(vector<PDF::Object>& objects) {
    for (PDF::Object& object : objects) {
        cout << "Object " << object.name << endl;
        for (PDF::Stream& stream : object.streams) {
            cout << " Stream " << stream.name << ", size: " << stream.size << endl;
        }
    }
}

void printBin(const string& data, size_t p, size_t n) {
    size_t N = data.size();
    for (size_t i=0; i<n && p+i<N; i++) cout << (size_t)(unsigned char)data[p+i] << " ";
    cout << endl;
}

struct BitStreamParser {
    size_t beg;
    size_t end;
    unsigned char* data = 0;

    size_t currentBytePtr = 0;
    unsigned char currentByte = 0;

    unsigned char currentBitPtr = 0;
    bool currentBit = 0;

    BitStreamParser(size_t b, size_t e, unsigned char* d) : beg(b), end(e), data(d) {
        currentBytePtr = b;
        currentByte = data[currentBytePtr];
        currentBitPtr = 0;
        currentBit = (currentByte >> currentBitPtr) & 1;
        //cout << " init bit stream, first bit: " << currentBit << ", first byte " << (int)currentByte << endl;
    }

    bool empty() {
        return bool(currentBytePtr >= end);
    }

    unsigned char read(int N = 1) {
        unsigned char res = 0;

        for (int i=0; i<N; i++) {
            currentBitPtr++;
            if (currentBitPtr >= 8) {
                currentBitPtr = 0;
                currentBytePtr++;
                if ( empty() ) currentByte = 0;
                else currentByte = data[currentBytePtr];
                //cout << "      new byte " << (int)currentByte << endl;
            }

            currentBit = (currentByte >> currentBitPtr) & 1;
            if (currentBit) res |= (1 << i);
            //cout << " " << currentBytePtr << "." << (int)currentBitPtr << " " << currentBit << endl;
        }

        return res;
    }
};

namespace PRC {
    string currentName;

    // uncompressed unsinged int
    struct UUInt {
        unsigned int i;
        void parse(const string& data, size_t& p) { memcpy(&i, &data[p], 4); p += 4; }
    };

    // compressed types
    struct Bool {
        bool b;

        void parse(BitStreamParser& bs) {
            b = bs.read(1);
        }
    };

    struct Char {
        char c;

        void parse(BitStreamParser& bs) {
            c = (char)bs.read(8);
        }
    };

    struct Int {
        int i;

        void parse(BitStreamParser& bs) { // TODO
            if ( bs.empty() ) { cout << "Warning in Int::parse, no more data! " << endl; return; }
            i = 0;
            int shift = 0;

            if (!bs.currentBit) {
                bs.read();
            } else {
                cout << "Warning in Int::parse, not implemented! " << endl;
                /*while(bs.currentBit) {
                    bs.read();
                    unsigned int byte = bs.read(8);
                    i |= (byte << shift);   // Store in the correct position
                    shift += 8;
                }*/
            }
        }
    };

    struct UInt {
        unsigned int i;

        void parse(BitStreamParser& bs) {
            if ( bs.empty() ) { cout << "Warning in UInt::parse, no more data! " << endl; return; }
            i = 0;
            int shift = 0;

            if (!bs.currentBit) {
                bs.read();
            } else {
                while(bs.currentBit) {
                    bs.read();
                    unsigned int byte = bs.read(8);
                    i |= (byte << shift);   // Store in the correct position
                    shift += 8;
                }
            }
        }
    };

    struct Double {
        double d;

        void parse(BitStreamParser& bs) {
            if ( bs.empty() ) { cout << "Warning in Double::parse, no more data! " << endl; return; }
            d = 0;
            int shift = 0;

            if (!bs.currentBit) {
                bs.read();
            } else {
                cout << "Warning in Double::parse, not implemented! " << endl;
                /*while(bs.currentBit) {
                    bs.read();
                    unsigned int byte = bs.read(8);
                    i |= (byte << shift);   // Store in the correct position
                    shift += 8;
                }*/
            }
        }
    };

    struct String {
        Bool notNull;
        UInt size;
        string str;

        void parse(BitStreamParser& bs) {
            notNull.parse(bs);
            if (notNull.b) {
                size.parse(bs);
                str = string(bs.read(size.i), size.i);
                cout << " String::parse: " << str << endl;
            }
        }
    };
}

std::ostream& operator<<(std::ostream& os, const PRC::Bool& i) { os << i.b; return os; }
std::ostream& operator<<(std::ostream& os, const PRC::Int& i) { os << i.i; return os; }
std::ostream& operator<<(std::ostream& os, const PRC::UInt& i) { os << i.i; return os; }
std::ostream& operator<<(std::ostream& os, const PRC::UUInt& i) { os << i.i; return os; }
std::ostream& operator<<(std::ostream& os, const PRC::Double& d) { os << d.d; return os; }
std::ostream& operator<<(std::ostream& os, const PRC::String& s) { os << s.str; return os; }

namespace PRC {
    struct Name {
        Bool reuseCurrentName;
        String name;

        void parse(BitStreamParser& bs) {
            reuseCurrentName.parse(bs);

            if (!reuseCurrentName.b) {
                name.parse(bs);
                currentName = name.str;
            }
            else name.str = currentName;
        }
    };

    struct UniqueID {
        UUInt a,b,c,d;

        void parse(const string& data, size_t& p) {
            a.parse(data, p);
            b.parse(data, p);
            c.parse(data, p);
            d.parse(data, p);
        }
    };
}

std::ostream& operator<<(std::ostream& os, const PRC::Name& n) { os << n.name; return os; }
std::ostream& operator<<(std::ostream& os, const PRC::UniqueID& id) { os << id.a << "." << id.b << "." << id.c << "." << id.d; return os; }

namespace PRC {
    struct FileStructureDescription { // done
        UniqueID fileID;
        UUInt reserved;
        UUInt Nsections;
        vector<UUInt> sectionOffsets;

        void parse(const string& data, size_t& p) {
            fileID.parse(data, p);
            reserved.parse(data, p);
            Nsections.parse(data, p);
            for (int i=0; i<Nsections.i; i++) {
                UUInt o;
                o.parse(data, p);
                sectionOffsets.push_back(o);
            }
        }
    };

    struct UncompressedBlock { // done
        UUInt Nbytes;
        string block;

        void parse(const string& data, size_t& p) {
            Nbytes.parse(data, p);
            block = string(&data[p], Nbytes.i);
            p += Nbytes.i;
        }
    };

    struct UncompressedFile { // done
        UUInt Nblocks;
        vector<UncompressedBlock> blocks;

        void parse(const string& data, size_t& p) {
            Nblocks.parse(data, p);
            for (int i=0; i<Nblocks.i; i++) {
                UncompressedBlock d;
                d.parse(data, p);
                blocks.push_back(d);
            }
        }
    };

    struct FileHeader { // done
        char PRC[3];
        UUInt minVersion;
        UUInt authVersion;
        UniqueID fileID;
        UniqueID appID;
        UUInt Nstructures;
        vector<FileStructureDescription> structureDescriptions;
        UUInt modelFileDataStartOffset;
        UUInt modelFileDataEndOffset;
        UUInt Nfiles;
        vector<UncompressedFile> uncompressedFiles;

        void parse(const string& data, size_t& p) {
            memcpy(PRC, &data[p], 3); p += 3;
            minVersion.parse(data, p);
            authVersion.parse(data, p);
            fileID.parse(data, p);
            appID.parse(data, p);

            Nstructures.parse(data, p);
            for (int i=0; i<Nstructures.i; i++) {
                FileStructureDescription d;
                d.parse(data, p);
                structureDescriptions.push_back(d);
            }

            modelFileDataStartOffset.parse(data, p);
            modelFileDataEndOffset.parse(data, p);

            Nfiles.parse(data, p);
            for (int i=0; i<Nfiles.i; i++) {
                UncompressedFile d;
                d.parse(data, p);
                uncompressedFiles.push_back(d);
            }
        }
    };

    struct Entity_Schema_definition {
        UInt type;
        UInt Ntokens;
        vector<UInt> tokens;

        void parse(BitStreamParser& bs) {
            type.parse(bs);
            Ntokens.parse(bs);

            for (int i=0; i<Ntokens.i; i++) {
                UInt t;
                t.parse(bs);
                tokens.push_back(t);
            }
        }
    };

    struct AttributeEntry {
        Bool isInteger;
        UInt intTitle;
        String strTitle;

        void parse(BitStreamParser& bs) {
            isInteger.parse(bs);
            if (isInteger.b) intTitle.parse(bs);
            else strTitle.parse(bs);
        }
    };

    struct KeyValue {
        AttributeEntry title;
        UInt key;
        Int value1;
        Double value2;
        Int value3; // time_t
        String value4;

        void parse(BitStreamParser& bs) {
            title.parse(bs);
            key.parse(bs);
            if (key.i == 1) value1.parse(bs);
            else if (key.i == 2) value2.parse(bs);
            else if (key.i == 3) value3.parse(bs);
            else if (key.i == 4) value4.parse(bs);
            else {
                cout << "Warning! invalid key " << key.i << endl;
            }
        }
    };

    struct PRC_TYPE_MISC_Attribute {
        UInt attribute;
        AttributeEntry title;
        UInt Npairs;
        vector<KeyValue> pairs;

        void parse(BitStreamParser& bs) {
            attribute.parse(bs);
            title.parse(bs);
            Npairs.parse(bs);

            for (int i=0; i<Npairs.i; i++) {
                KeyValue k;
                k.parse(bs);
                pairs.push_back(k);
            }
        }
    };

    struct AttributeData {
        UInt Nattributes;
        vector<PRC_TYPE_MISC_Attribute> attributes;

        void parse(BitStreamParser& bs) {
            Nattributes.parse(bs);
            cout << " Nattributes " << Nattributes << endl;

            for (int i=0; i<Nattributes.i; i++) {
                PRC_TYPE_MISC_Attribute a;
                a.parse(bs);
                attributes.push_back(a);
                break; // ------------------------- TOTEST
            }
        }
    };

    struct ContentPRCBase {
        AttributeData attribute;
        Name name;
        UInt CADID1;
        UInt CADID2;
        UInt structureID;

        void parse(BitStreamParser& bs) {
            attribute.parse(bs);
            name.parse(bs);
            CADID1.parse(bs);
            CADID2.parse(bs);
            structureID.parse(bs);
            //cout << "ContentPRCBase attribute " << ID << endl;
        }
    };

    struct PRC_TYPE_TESS {
        ;
    };

    struct PRC_TYPE_ASM_FileStructureTessellation {
        UInt ID;
        ContentPRCBase info;
        UInt Ntessellations;
        vector<PRC_TYPE_TESS> tessellations;

        void parse(BitStreamParser& bs) {
            ID.parse(bs);
            info.parse(bs);
            Ntessellations.parse(bs);

            cout << " tessellation ID: " << ID << endl;
            cout << " N tessellation: " << Ntessellations << endl;
        }
    };

    struct TopologicalContext {
        UInt ID;
        ContentPRCBase info;
        Char behaviour;
        Double grandularity;
        Double tolerance;
        Bool hasMinFaceThickness;
        Double minFaceThickness;
        Bool hasScaleFactor;
        Double scale;
        UInt Nbodies;
        //vector<PRC_TYPE_TOPO_Body> bodies;

        void parse(BitStreamParser& bs) {
            ID.parse(bs);
            info.parse(bs);
            behaviour.parse(bs);
            grandularity.parse(bs);
            tolerance.parse(bs);
            hasMinFaceThickness.parse(bs);
            if (hasMinFaceThickness.b) minFaceThickness.parse(bs);
            hasScaleFactor.parse(bs);
            if (hasScaleFactor.b) scale.parse(bs);
            Nbodies.parse(bs);
        }
    };

    struct FileStructureExactGeometry {
        UInt Ncontexts;
        vector<TopologicalContext> contexts;

        void parse(BitStreamParser& bs) {
            Ncontexts.parse(bs);
            cout << " Ncontexts: " << Ncontexts << endl;
        }
    };

    struct PRC_TYPE_ASM_FileStructureGeometry {
        UInt ID;
        ContentPRCBase info;
        FileStructureExactGeometry geometry;
        //UserData data;

        void parse(BitStreamParser& bs) {
            ID.parse(bs);
            cout << " geometry ID: " << ID << endl;
            info.parse(bs);
            geometry.parse(bs);
            //data.parse(bs);
        }
    };

    struct FileStructureHeader { // done
        char PRC[3];
        UUInt minVersion;
        UUInt authVersion;
        UniqueID fileID;
        UniqueID appID;
        UUInt Nfiles;
        vector<UncompressedFile> files;

        void parse(const string& data, size_t& p) {
            cout << "FileStructureHeader parse, p: " << p << endl;
            memcpy(PRC, &data[p], 3); p += 3;
            minVersion.parse(data, p);
            authVersion.parse(data, p);
            fileID.parse(data, p);
            appID.parse(data, p);

            Nfiles.parse(data, p);
            for (int i=0; i<Nfiles.i; i++) {
                UncompressedFile d;
                d.parse(data, p);
                files.push_back(d);
            }
        }
    };

    struct FileStructureSchema {
        UInt Ntypes;
        vector<Entity_Schema_definition> definitions;

        void parse(BitStreamParser& bs) {
            Ntypes.parse(bs);
            cout << " Ntypes: " << Ntypes << endl;

            /*for (int i=0; i<Ntypes.i; i++) {
                Entity_Schema_definition d;
                d.parse(data, p);
                definitions.push_back(d);
            }*/
        }
    };

    struct FileStructure {
        FileStructureHeader header;
        //PRC_TYPE_ASM_FileStructureGlobals globals;
        //PRC_TYPE_ASM_FileStructureTree tree;
        PRC_TYPE_ASM_FileStructureTessellation tessellation;
        PRC_TYPE_ASM_FileStructureGeometry geometry;
        //PRC_TYPE_ASM_FileStructureExtraGeometry geometryInfo;
    };
}

#include <iostream>
#include <fstream>
#include <string>

void exportToFile(string data, string path) {
    std::ofstream out(path, std::ios::binary);
    out.write(data.c_str(), data.size());
    out.close();
}

void parsePRCStructure(string& data, PRC::FileStructureDescription& description) {
    if (description.Nsections.i != 6) return;
    if (description.sectionOffsets.size() != 7) return;

    PRC::FileStructure structure;

    size_t startHeader = description.sectionOffsets[0].i;
    size_t startGlobals = description.sectionOffsets[1].i;
    size_t startTree = description.sectionOffsets[2].i;
    size_t startTessellations = description.sectionOffsets[3].i;
    size_t startGeometries = description.sectionOffsets[4].i;
    size_t startExtraGeometries = description.sectionOffsets[5].i;
    size_t startNextHeader = description.sectionOffsets[6].i;

    size_t sizeHeader = startGlobals - startHeader;
    size_t sizeGlobals = startTree - startGlobals;
    size_t sizeTree = startTessellations - startTree;
    size_t sizeTessellations = startGeometries - startTessellations;
    size_t sizeGeometries = startExtraGeometries - startGeometries;
    size_t sizeExtraGeometries = startNextHeader - startExtraGeometries;

    cout << "Header start: " << startHeader << ", size: " << sizeHeader << endl;
    cout << "Globals start: " << startGlobals << ", size: " << sizeGlobals << endl;
    cout << "Tree start: " << startTree << ", size: " << sizeTree << endl;
    cout << "Tessellations start: " << startTessellations << ", size: " << sizeTessellations << endl;
    cout << "Geometries start: " << startGeometries << ", size: " << sizeGeometries << endl;
    cout << "ExtraGeometries start: " << startExtraGeometries << ", size: " << sizeExtraGeometries << endl;

    size_t current = startHeader;
    auto& h = structure.header;
    h.parse(data, current);

    cout << "  structure head " << string(h.PRC, 3) << endl;
    cout << "  structure minVersion: " << h.minVersion << endl;
    cout << "  structure authVersion: " << h.authVersion << endl;
    cout << "  structure fileID: " << h.fileID << endl;
    cout << "  structure appID: " << h.appID << endl;
    cout << "  structure Nfiles: " << h.Nfiles << endl;


    auto extractSection = [&](size_t& a, size_t N) -> string {
        cout << "try to uncompress section, from " << a << " -> " << a+N << " ( " << N << " )";
        string udata(data.begin()+a, data.begin()+a+N);
        udata = decompressZlib(udata);
        cout << " uncompressed data size: " << udata.size() << endl;
        a += N;
        return udata;
    };

    string dataGlobals = extractSection(current, sizeGlobals);
    string dataTree = extractSection(current, sizeTree);
    string dataTessellations = extractSection(current, sizeTessellations);
    string dataGeometries = extractSection(current, sizeGeometries);
    string dataExtraGeometries = extractSection(current, sizeExtraGeometries);

    BitStreamParser bst(0, dataTessellations.size(), (unsigned char*)&dataTessellations[0]);
    BitStreamParser bsg(0, dataGeometries.size(), (unsigned char*)&dataGeometries[0]);

    structure.tessellation.parse(bst);
    //structure.geometry.parse(bsg);

    //exportToFile(dataGlobals, "globals.bin");
    //exportToFile(dataTree, "tree.bin");
    //exportToFile(dataTessellations, "tessellations.bin");
    //exportToFile(dataGeometries, "geometries.bin");
    //exportToFile(dataExtraGeometries, "extrageometries.bin");

    //return;

    /*for (auto d : {dataGlobals, dataTree, dataTessellations, dataGeometries, dataExtraGeometries}) {
        BitStreamParser bs(0, d.size(), (unsigned char*)&d[0]);

        cout << "k " << d.size() << endl;
        for (int j=0; j<32; j++) {
            unsigned int k = bs.readUInt32();
            cout << " " << j << ": " << k << endl;
        }
    }*/



    //printBin(udata2, 0, udata2.size());

    //size_t scurrent = 0;
    //s.parse(udata2, scurrent);

    size_t tcurrent = 0;
    //t.parse(dataTessellations, tcurrent);

    //size_t gcurrent = 0;
    //g.parse(udata5, gcurrent);



    //cout << "  attribute " << t.info.attribute << endl;
    //cout << "  tesselation name " << t.info.name << endl;
    //cout << "  tesselation, N tessellations: " << t.Ntessellations << endl;
}

void processPRC(PDF::Object& object) {
    if (object.streams.size() == 0) return;
    PDF::Stream& stream = object.streams[0];
    if (stream.unpacked == "") return;

    string& data = stream.unpacked;

    size_t current = 0;
    PRC::FileHeader header;
    header.parse(data, current);

    cout << endl << " --== header start ==--" << endl;
    cout << string(header.PRC, 3) << endl;
    cout << " min version " << header.minVersion << endl;
    cout << " auth version " << header.authVersion << endl;
    cout << " internal ID: " << header.fileID << endl;
    cout << " app ID: " << header.appID << endl;
    cout << " N structures: " << header.Nstructures << endl;

    for (int i=0; i<header.Nstructures.i; i++) {
        PRC::FileStructureDescription& description = header.structureDescriptions[i];
        if (i > 0) {
            PRC::FileStructureDescription& prevDescription = header.structureDescriptions[i-1];
            prevDescription.sectionOffsets.push_back( description.sectionOffsets[0] ); // append start of next structure
        }
        cout << "  file " << i << ", ID: " << description.fileID << ", N sections " << description.Nsections << endl;
    }

    cout << " modelFileData start offset: " << header.modelFileDataStartOffset << endl;
    cout << " modelFileData end offset: " << header.modelFileDataEndOffset << endl;
    cout << " N files " << header.Nfiles << endl;
    cout << " --== header end ==--" << endl << endl;

    for (PRC::FileStructureDescription& description : header.structureDescriptions) {
        parsePRCStructure(stream.unpacked, description);
        break;
    }
}

VRTransformPtr VRPDF::extract3DModels(string path) {
    ifstream file(path, std::ios::binary);
    vector<char> buffer((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

    vector<PDF::Object> objects = extractObjects(buffer);
    extractObjectNames(buffer, objects);
    extractStreams(buffer, objects);
    vector<PDF::Object> geometries = filter3DObjects(objects);
    for (auto& obj : geometries) unpack3DObject(buffer, obj);
    for (auto& obj : geometries) processPRC(obj);

    // TODO: convert PRC to geometry

    //printObjects(objects);
    //printObjects(geometries);

    return 0;
}





