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


void printBits(const string& s, size_t a, size_t n) {
    for (int i=0; i<n; i++) {
        unsigned char c = s[a+i];
        for (int j=0; j<8; j++) {
            int b = ((c >> (7-j)) & 1);
            cout << b;
        }
        cout << " ";
    }
    cout << endl;
}

void printBits(unsigned int c) {
    for (int j=0; j<32; j++) {
        int b = ((c >> (31-j)) & 1);
        cout << b;
        if (j%8 == 7) cout << " ";
    }
}

void printBits(double c) {
    union DI {
        double d;
        uint64_t i;
    };

    DI di;
    di.d = c;

    for (int j=0; j<64; j++) {
        int b = ((di.i >> (63-j)) & 1);
        cout << b;
        if (j == 0 || j == 11 || j == 31) cout << " ";
    }
}

void printBits(unsigned char c) {
    for (int j=0; j<8; j++) {
        int b = ((c >> (7-j)) & 1);
        cout << b;
    }
}

void printBits(string s) {
    for (auto c : s) {
        printBits((unsigned char)c);
        cout << " ";
    }
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
        reset();
    }

    void reset() {
        set(beg, 0);
    }

    void set(size_t byte, size_t bit) {
        currentBytePtr = byte;
        currentByte = data[currentBytePtr];
        currentBitPtr = bit;
        currentBit = (currentByte >> (7-currentBitPtr)) & 1;
        //cout << " cu " << currentBytePtr << "." << (int)currentBitPtr << " " << currentBit << endl;
    }

    bool empty() {
        return bool(currentBytePtr >= end);
    }

    void printNextBits(size_t N) {
        size_t Bi = currentBytePtr;
        size_t bi = currentBitPtr;
        unsigned char B = currentByte;
        bool b = currentBit;

        for (int i=0; i<N; i++) {
            cout << b;

            bi++;
            if (bi >= 8) {
                bi = 0;
                Bi++;
                if ( empty() ) B = 0;
                else B = data[Bi];
                //cout << "      new byte " << (int)B << endl;
            }

            b = (B >> (7-bi)) & 1;
        }
        cout << endl;
    }

    unsigned char read(int N = 1) {
        unsigned char res = 0;

        for (int i=0; i<N; i++) {
            if (currentBit) res |= (1 << (7-i));

            currentBitPtr++;
            if (currentBitPtr >= 8) {
                currentBitPtr = 0;
                currentBytePtr++;
                if ( empty() ) currentByte = 0;
                else currentByte = data[currentBytePtr];
                //cout << "      new byte " << (int)currentByte << endl;
            }

            currentBit = (currentByte >> (7-currentBitPtr)) & 1;
            //cout << " cu " << currentBytePtr << "." << (int)currentBitPtr << " " << currentBit << endl;
        }

        //cout << " -= read =- "; printBits(res); cout << endl;
        return res;
    }

    uint32_t read4(int N = 1) {
        uint32_t res = 0;

        for (int i=0; i<N; i++) {
            if (currentBit) res |= (1 << (31-i));

            currentBitPtr++;
            if (currentBitPtr >= 8) {
                currentBitPtr = 0;
                currentBytePtr++;
                if ( empty() ) currentByte = 0;
                else currentByte = data[currentBytePtr];
                //cout << "      new byte " << (int)currentByte << endl;
            }

            currentBit = (currentByte >> (7-currentBitPtr)) & 1;
            //cout << " cu " << currentBytePtr << "." << (int)currentBitPtr << " " << currentBit << endl;
        }

        return res;
    }
};



#include "RPCUtils.h"

sCodageOfFrequentDoubleOrExponent* searchFrequentDouble( double value ) {
	sCodageOfFrequentDoubleOrExponent cofdoe;
	cofdoe.u2uod.Value = value;

	// bin search
	sCodageOfFrequentDoubleOrExponent* pcofdoe = (sCodageOfFrequentDoubleOrExponent *) bsearch(&cofdoe,acofdoe,sizeof(acofdoe)/sizeof(pcofdoe[0]),sizeof(pcofdoe[0]),stCOFDOECompare);

	// move to first matching exponent
	while ( pcofdoe>acofdoe && EXPONENT(pcofdoe->u2uod.Value) == EXPONENT((pcofdoe-1)->u2uod.Value) )
        pcofdoe--;

    // check if value matches
	while (pcofdoe->Type == VT_double) {
		if (fabs(value) == pcofdoe->u2uod.Value) break;
		pcofdoe++;
	}

    return pcofdoe;
}



// TODO: optimize!
bool compareBits(const sCodageOfFrequentDoubleOrExponent& e, const unsigned int& bits, bool verb) {
    if (verb)  { printBits(bits); cout << endl; }
    if (verb)  { printBits(e.Bits); cout << endl; }
    for (int i=0; i<e.NumberOfBits; i++) {
        int j = (e.NumberOfBits - 1 - i);
        int b1 = ((bits >> (31 - i)) & 1);
        int b2 = ((e.Bits >> j) & 1);
        if (verb) cout << " comp " << i << ", " << j << "  bb " << b1 << ", " << b2 << endl;
        if (b1 != b2) return false;
    }
    return true;
}

double readDouble(BitStreamParser& bs, bool& success) {
    bool verbose = false;
    success = true;

    ieee754_double value;
    memset(&value, 0, sizeof(value));
    if (verbose) cout << endl << "read double " << endl;

    /*size_t byte1Ptr = bs.currentBytePtr;
    size_t bit1Ptr = bs.currentBitPtr;
    for (int i=0; i<16; i++) {
        bool b = bs.read(1);
        //cout << b;
    }
    //cout << endl;
    bs.set(byte1Ptr, bit1Ptr);*/

    // get entry and reset bs
    size_t bytePtr = bs.currentBytePtr;
    size_t bitPtr = bs.currentBitPtr;

    uint32_t bits = bs.read4(22);
    size_t i = 0;
    sCodageOfFrequentDoubleOrExponent entry;
    for (const auto& e : acofdoe) {
        if (compareBits(e, bits, 0)) { entry = e; break; }
        i++;
    }
    if (verbose) cout << "Found entry #" << i << ", N bits: " << entry.NumberOfBits << endl;

    if (verbose) cout << "  set back.. " << endl;
    bs.set(bytePtr, bitPtr);
    if (verbose) cout << "  read " << entry.NumberOfBits << " bits" << endl;
    bs.read4(entry.NumberOfBits);

    // check for zero
    if ( entry.Bits == 0x1 ) {
        if (verbose) cout << " -- value is zero" << endl;
        return 0;
	}

    // read the sign bit
    if (verbose) cout << "  read negative: " << endl;
    bool b = bs.read(1);
    value.ieee.negative = b;
    if (verbose) cout << value.ieee.negative << endl;

    // if value return it (needs to be after sign bit read)
    if (entry.Type == VT_double) return entry.u2uod.Value;

    unsigned int exp = entry.u2uod.ul[1];
    value.ieee.exponent = (exp >> 20) & 0x7FF;
    if (verbose) { cout << "  exponent? "; printBits(entry.u2uod.ul[1]); cout << endl; }

    // check for mantissa
    if (verbose) cout << "  read hasMantissa: " << endl;
    bool hasMantissa = bs.read(1);
    if (verbose) cout << hasMantissa << endl;
    if (!hasMantissa) return value.d;

    unsigned long long mantissa = 0;

    // read last 4 bits of mantissa
    unsigned char B1 = bs.read(4);
    if (verbose) { cout << " last4: "; printBits(B1); cout << endl; }
    for (int i = 0; i < 4; i++) {
        int b = ((B1 >> (7-i)) & 1);
        if (verbose) cout << "  bit: " << b << endl;
        value.ieee.mantissa0 |= ((unsigned int)b << (19 - i));
    }

    // read remaining bits using compression rules
    vector<uint8_t> mantissaBytes;
    while (mantissaBytes.size() < 6) {
        if (bs.read(1)) {
            // Read a full 8-bit byte
            if (verbose) cout << "  read next byte: " << endl;
            uint8_t byte = bs.read(8);
            if (verbose) { printBits(byte); cout << endl; }
            mantissaBytes.push_back(byte);
        } else {
            // Read a 3-bit index and copy an earlier byte
            if (verbose) cout << "  read byte index: " << endl;
            uint8_t B = bs.read(3);
            unsigned int index = ((B >> 5) & 0b111);
            if (verbose) { printBits(B); cout << " -> " << (int)index << endl; }

            if (index == 0) break;

            if (index == 6) { // special case, no bytes until end
                while (mantissaBytes.size() < 5) mantissaBytes.push_back(0);
                if (verbose) cout << "  read last byte: " << endl;
                uint8_t byte = bs.read(8);
                mantissaBytes.push_back(byte);
                break;
            }

            if (index > mantissaBytes.size() ) {
                cout << "Warning! readDouble failed, tried to access index " << index << " but only have " << mantissaBytes.size() << endl;
                //cout << " bits: " << ; // TODO
                success = false;
                break;
            }

            // 'index' is the distance from the current byte to the previous byte to be copied
            uint8_t byte = mantissaBytes[ mantissaBytes.size() - index ];
            mantissaBytes.push_back( byte );
        }
    }
    while (mantissaBytes.size() < 6) mantissaBytes.push_back(0);

    value.ieee.mantissa1 |= ( mantissaBytes[5] << 8*0 );
    value.ieee.mantissa1 |= ( mantissaBytes[4] << 8*1 );
    value.ieee.mantissa1 |= ( mantissaBytes[3] << 8*2 );
    value.ieee.mantissa1 |= ( mantissaBytes[2] << 8*3 );
    value.ieee.mantissa0 |= ( mantissaBytes[1] << 8*0 );
    value.ieee.mantissa0 |= ( mantissaBytes[0] << 8*1 );

    return value.d;
}


struct BitWriter {
    string buffer; // 9 bytes
    int currentByte = 0;
    int currentBit = 0;

    BitWriter(int s) : buffer(s,0) {}

    void add_bits(unsigned int v, int Nbits) {
        //cout << "add_bits " << v << ", N " << Nbits << " -> ";
        for (int i=0; i<Nbits; i++) {
            int b = ((v >> (Nbits - 1 - i)) & 1);
            //int b = ((v >> i) & 1);
            //cout << b;
            buffer[currentByte] |= (b << (7-currentBit));
            currentBit++;
            if (currentBit >= 8) {
                currentBit = 0;
                currentByte++;
            }
        }
        //cout << endl;
    };

    void printBits() {
        for (int B=0; B<=currentByte; B++) {
            unsigned char c = buffer[B];
            if (B < currentByte) ::printBits((unsigned char)c);
            else {
                for (int j=0; j<currentBit; j++) {
                    int b = ((c >> (7-j)) & 1);
                    cout << b;
                }
            }

            cout << " ";
        }
        cout << endl;
    }
};

string writeUnsignedInteger(unsigned uValue) {
    //cout << "writeUnsignedInteger " << uValue << endl;
    BitWriter bWriter(5); // 5 bytes
    for(;;) {
        if (uValue == 0) {
            //cout << " write 0 at " << bWriter.currentByte << "." << bWriter.currentBit << endl;
            bWriter.add_bits(0, 1);
            break;
        }

        //cout << " write 1 at " << bWriter.currentByte << "." << bWriter.currentBit << endl;
        bWriter.add_bits(1, 1);
        //cout << " write 8 bits at "; printBits((unsigned char)(uValue & 0xFF)); cout << endl;
        bWriter.add_bits(uValue & 0xFF, 8);
        uValue >>= 8;
    }
    //cout << " bWriter " << bWriter.currentByte << "." << bWriter.currentBit << endl;
    return bWriter.buffer;
}

BitWriter writeDouble( double value ) {
    BitWriter bWriter(9); // 9 bytes

    //cout << endl << "write double: " << value << endl;
    sCodageOfFrequentDoubleOrExponent* pcofdoe = searchFrequentDouble( value );

    // write bits from map
    //cout << " -- value map key of entry " << (int)(pcofdoe - acofdoe) << endl;
    //cout << "  exponent: "; printBits(pcofdoe->Bits); cout << endl;
    for(int i=1<<(pcofdoe->NumberOfBits-1);i>=1;i>>=1)
        bWriter.add_bits( (pcofdoe->Bits&i)!=0,1);

    // if value 0, return
	if ( !memcmp(&value,stadwZero,sizeof(value)) || !memcmp(&value,stadwNegativeZero,sizeof(value)) ) {
        //cout << " -- value is zero" << endl;
        return bWriter;
	}

	// write sign
	union ieee754_double* pid = (union ieee754_double *)&value;
    //cout << " -- write sign" << endl;
	bWriter.add_bits( pid->ieee.negative, 1);

	// if value in entry stop here
	if (pcofdoe->Type == VT_double) {
        //cout << " -- pcofdoe->Type is VT_double" << endl;
        return bWriter;
	}

	// empty mantissa
	if (pid->ieee.mantissa0==0 && pid->ieee.mantissa1==0) {
        //cout << " -- mantissa is zero" << endl;
        bWriter.add_bits( 0,1);
        return bWriter;
    }

    //cout << " -- add one" << endl;
	bWriter.add_bits( 1,1);

	// encode mantissa
    //cout << " -- add first 4 mantissa bits" << endl;
	PRCbyte* pb = ((PRCbyte *)&value)+6;
	bWriter.add_bits( (*pb)&0x0f,4  );
	NEXTBYTE(pb);
	PRCbyte* pbStart = pb;
	PRCbyte* pbEnd  = ((PRCbyte *)&value);
	PRCbyte* pbStop = ((PRCbyte *)&value);

	PRCbyte *pbResult, bSaveAtEnd;
	int fSaveAtEnd;
	if ((fSaveAtEnd=(*pbStop!=*BEFOREBYTE(pbStop)))!=0) bSaveAtEnd = *pbEnd;
	PREVIOUSBYTE(pbStop);
	while ( *pbStop == *BEFOREBYTE(pbStop) ) PREVIOUSBYTE(pbStop);

	for(;MOREBYTE(pb,pbStop);NEXTBYTE(pb)) {
        //cout << " -- add double byte" << endl;
		if(pb!=pbStart && (pbResult=SEARCHBYTE(BEFOREBYTE(pb),*pb,DIFFPOINTERS(pb,pbStart)))!=NULL) {
			bWriter.add_bits( 0,1  );
			bWriter.add_bits( DIFFPOINTERS(pb,pbResult),3  );
		} else {
			bWriter.add_bits( 1,1  );
			bWriter.add_bits( *pb,8  );
		}
	}

	if ( !MOREBYTE(BEFOREBYTE(pbEnd), pbStop) ) {
		if(fSaveAtEnd) {
			bWriter.add_bits( 0,1  );
			bWriter.add_bits( 6,3  );
			bWriter.add_bits( bSaveAtEnd,8  );
		} else {
			bWriter.add_bits( 0,1  );
			bWriter.add_bits( 0,3  );
		}
	} else {
		if((pbResult=SEARCHBYTE(BEFOREBYTE(pb),*pb,DIFFPOINTERS(pb,pbStart)))!=NULL) {
			bWriter.add_bits( 0,1  );
			bWriter.add_bits( DIFFPOINTERS(pb,pbResult),3  );
		} else {
			bWriter.add_bits(1,1  );
			bWriter.add_bits(*pb,8  );
		}
	}

	return bWriter;
}

BitWriter writeDouble2( double value ) {
    BitWriter bWriter(9); // 9 bytes

    auto writeBit = [&](uint8_t b) {
        bWriter.add_bits(b,1);
    };

    auto writeByte = [&](uint8_t b) {
        bWriter.add_bits(b,8);
    };

    auto writeBits = [&](uint32_t b, int N) {
        bWriter.add_bits(b,N);
    };

  union ieee754_double *pid=(union ieee754_double *)&value;
  int
        i,
        fSaveAtEnd;
        PRCbyte
        *pb,
        *pbStart,
        *pbStop,
        *pbEnd,
        *pbResult,
        bSaveAtEnd = 0;
  sCodageOfFrequentDoubleOrExponent
        cofdoe,
        *pcofdoe;

  cofdoe.u2uod.Value=value;
  pcofdoe = (sCodageOfFrequentDoubleOrExponent *)bsearch(
                           &cofdoe,
                           acofdoe,
                           sizeof(acofdoe)/sizeof(pcofdoe[0]),
                           sizeof(pcofdoe[0]),
                           stCOFDOECompare);

  while(pcofdoe>acofdoe && EXPONENT(pcofdoe->u2uod.Value)==EXPONENT((pcofdoe-1)->u2uod.Value))
    pcofdoe--;

  assert(pcofdoe);
  while(pcofdoe->Type==VT_double)
  {
    if(fabs(value)==pcofdoe->u2uod.Value)
      break;
    pcofdoe++;
  }

  for(i=1<<(pcofdoe->NumberOfBits-1);i>=1;i>>=1)
    writeBit((pcofdoe->Bits&i)!=0);

  if
  (
    !memcmp(&value,stadwZero,sizeof(value))
    ||      !memcmp(&value,stadwNegativeZero,sizeof(value))
  )
    return bWriter;

  writeBit(pid->ieee.negative);

  if(pcofdoe->Type==VT_double)
    return bWriter;

  if(pid->ieee.mantissa0==0 && pid->ieee.mantissa1==0)
  {
    writeBit(0);
    return bWriter;
  }

  writeBit(1);

#ifdef WORDS_BIGENDIAN
  pb=((PRCbyte *)&value)+1;
#else
  pb=((PRCbyte *)&value)+6;
#endif
  //add_bits((*pb)&0x0f,4 STAT_V STAT_DOUBLE);
  writeBits((*pb)&0x0F,4);

  NEXTBYTE(pb);
  pbStart=pb;
#ifdef WORDS_BIGENDIAN
  pbEnd=
  pbStop= ((PRCbyte *)(&value+1))-1;
#else
  pbEnd=
  pbStop= ((PRCbyte *)&value);
#endif

  if((fSaveAtEnd=(*pbStop!=*BEFOREBYTE(pbStop)))!=0)
    bSaveAtEnd=*pbEnd;
  PREVIOUSBYTE(pbStop);

  while(*pbStop==*BEFOREBYTE(pbStop))
    PREVIOUSBYTE(pbStop);

  for(;MOREBYTE(pb,pbStop);NEXTBYTE(pb))
  {
    if(pb!=pbStart && (pbResult=SEARCHBYTE(BEFOREBYTE(pb),*pb,DIFFPOINTERS(pb,pbStart)))!=NULL)
    {
      writeBit(0);
      writeBits(DIFFPOINTERS(pb,pbResult),3);
    }
    else
    {
      writeBit(1);
      writeByte(*pb);
    }
  }

  if(!MOREBYTE(BEFOREBYTE(pbEnd),pbStop))
  {
    if(fSaveAtEnd)
    {
      writeBit(0);
      writeBits(6,3);
      writeByte(bSaveAtEnd);
    }
    else
    {
      writeBit(0);
      writeBits(0,3);
    }
  }
  else
  {
    if((pbResult=SEARCHBYTE(BEFOREBYTE(pb),*pb,DIFFPOINTERS(pb,pbStart)))!=NULL)
    {
      writeBit(0);
      writeBits(DIFFPOINTERS(pb,pbResult),3);
    }
    else
    {
      writeBit(1);
      writeByte(*pb);
    }
  }

  return bWriter;
}

namespace PRC {
    string currentName;

    // uncompressed unsinged int
    struct UUInt {
        unsigned int i = 0;
        void parse(const string& data, size_t& p) { memcpy(&i, &data[p], 4); p += 4; }
    };

    // compressed types
    struct Bool {
        bool b = 0;

        void parse(BitStreamParser& bs) {
            b = bs.read(1);
        }
    };

    struct Char {
        char c = 0;

        void parse(BitStreamParser& bs) {
            c = (char)bs.read(8);
        }
    };

    struct Int {
        int i = 0;

        void parse(BitStreamParser& bs) { // TODO
            if ( bs.empty() ) { cout << "Warning in Int::parse, no more data! " << endl; return; }
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
                bs.read();
            }
        }
    };

    struct UInt {
        unsigned int i = 0;

        void peek(BitStreamParser& bs) {
            size_t byte = bs.currentBytePtr;
            size_t bit = bs.currentBitPtr;
            parse(bs);
            bs.set(byte, bit);
        }

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
                    //cout << "  UInt byte: "; printBits(byte); cout << endl;
                    i |= (byte << shift);   // Store in the correct position
                    shift += 8;
                }
                bs.read();
            }
        }
    };

    struct Double {
        double d = 0;

        bool parse(BitStreamParser& bs) {
            if ( bs.empty() ) { cout << "Warning in Double::parse, no more data! " << endl; return false; }
            bool success = true;
            d = readDouble(bs, success);
            return success;
        }
    };

    struct BITS {
        vector<bool> bits;

        void parse(BitStreamParser& bs, UInt N) {
            for (size_t i=0; i<N.i; i++) {
                bool b = bs.read(1);
                bits.push_back(b);
            }
        }
    };

    struct String {
        Bool notNull;
        UInt size;
        string str;

        void parse(BitStreamParser& bs) {
            //cout << "String::parse" << endl;
            notNull.parse(bs);
            if (notNull.b) {
                size.parse(bs);
                //cout << " size: " << size.i << endl;
                str = string(size.i, 0);
                for (int i=0; i<size.i; i++) str[i] = bs.read(8);
                //cout << " str: '" << str << "'" << endl;
            }
        }
    };

    struct TypeID : UInt {
        void parse(BitStreamParser& bs, unsigned int type) {
            UInt::parse(bs);
            if (i != type) cout << "Warning in TypeID, expected " << type << ", got: " << i << endl;
        }

        void parse(BitStreamParser& bs, vector<unsigned int> types) {
            UInt::parse(bs);
            bool typeOK = false;
            for (auto& t : types)
                if (t == i) { typeOK = true; break; }
            if (!typeOK) {
                cout << "Warning in TypeID, got wrong type: " << i << endl;
                cout << " expected one of: ";
                for (auto& t : types) cout << t << " ";
                cout << endl;
            }
        }
    };

    template<class T>
    struct ArrayOf {
        vector<T> v;

        template<typename... Args>
        void parse(BitStreamParser& bs, UInt N, Args&&... args) {
            for (size_t i=0; i<N.i; i++) {
                T t;
                t.parse(bs, std::forward<Args>(args)...);
                v.push_back(t);
            }
        }
    };
}

std::ostream& operator<<(std::ostream& os, const PRC::Bool& i) { os << i.b; return os; }
std::ostream& operator<<(std::ostream& os, const PRC::Int& i) { os << i.i; return os; }
std::ostream& operator<<(std::ostream& os, const PRC::Char& i) { os << i.c; return os; }
std::ostream& operator<<(std::ostream& os, const PRC::UInt& i) { os << i.i; return os; }
std::ostream& operator<<(std::ostream& os, const PRC::UUInt& i) { os << i.i; return os; }
std::ostream& operator<<(std::ostream& os, const PRC::Double& d) { os << d.d; return os; }
std::ostream& operator<<(std::ostream& os, const PRC::String& s) { os << s.str; return os; }

namespace PRC {
    struct Name {
        Bool reuseCurrentName;
        String name;

        void parse(BitStreamParser& bs) {
            //cout << "Name::parse" << endl;
            reuseCurrentName.parse(bs);
            //cout << " reuseCurrentName? " << reuseCurrentName.b << endl;

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
    // https://ftp.linux.cz/pub/tex/CTAN/graphics/asymptote/prc/include/prc/PRC.h

    enum PRC_TYPE {

        TYPE_SURF = 75,
        TYPE_SURF_Base = TYPE_SURF + 1,	// Abstract type for all geometric surfaces.
        TYPE_SURF_Blend01 = TYPE_SURF + 2,	// Blend surface.
        TYPE_SURF_Blend02 = TYPE_SURF + 3,	// Blend Surface.
        TYPE_SURF_Blend03 = TYPE_SURF + 4,	// Blend Surface.
        TYPE_SURF_NURBS = TYPE_SURF + 5,	// Non Uniform BSpline surface.
        TYPE_SURF_Cone = TYPE_SURF + 6,	// Cone.
        TYPE_SURF_Cylinder = TYPE_SURF + 7,	// Cylinder.
        TYPE_SURF_Cylindrical = TYPE_SURF + 8,	// Surface who is defined in cylindrical space.
        TYPE_SURF_Offset = TYPE_SURF + 9,	// Offset surface.
        TYPE_SURF_Pipe = TYPE_SURF + 10,	// Pipe.
        TYPE_SURF_Plane = TYPE_SURF + 11,	// Plane.
        TYPE_SURF_Ruled = TYPE_SURF + 12,	// Ruled surface.
        TYPE_SURF_Sphere = TYPE_SURF + 13,	// Sphere.
        TYPE_SURF_Revolution = TYPE_SURF + 14,	// Surface of revolution.
        TYPE_SURF_Extrusion = TYPE_SURF + 15,	// Surface of extrusion.
        TYPE_SURF_FromCurves = TYPE_SURF + 16,	// Surface from two curves.
        TYPE_SURF_Torus = TYPE_SURF + 17,	// Torus.
        TYPE_SURF_Transform = TYPE_SURF + 18,	// Transformed surface.
        TYPE_SURF_Blend04 = TYPE_SURF + 19,	// defined for future use.

        TYPE_TOPO = 140,
        TYPE_TOPO_Context = TYPE_TOPO + 1,
        TYPE_TOPO_Item = TYPE_TOPO + 2,
        TYPE_TOPO_MultipleVertex = TYPE_TOPO + 3,
        TYPE_TOPO_UniqueVertex = TYPE_TOPO + 4,
        TYPE_TOPO_WireEdge = TYPE_TOPO + 5,
        TYPE_TOPO_Edge = TYPE_TOPO + 6,
        TYPE_TOPO_CoEdge = TYPE_TOPO + 7,
        TYPE_TOPO_Loop = TYPE_TOPO + 8,
        TYPE_TOPO_Face = TYPE_TOPO + 9,
        TYPE_TOPO_Shell = TYPE_TOPO + 10,
        TYPE_TOPO_Connex = TYPE_TOPO + 11,
        TYPE_TOPO_Body = TYPE_TOPO + 12,
        TYPE_TOPO_SingelWireBody = TYPE_TOPO + 13,
        TYPE_TOPO_BrepData = TYPE_TOPO + 14,
        TYPE_TOPO_SingleWireBodyCompress = TYPE_TOPO + 15,
        TYPE_TOPO_BrepDataCompress = TYPE_TOPO + 16,
        TYPE_TOPO_WIreBody = TYPE_TOPO + 17,


        TYPE_TESS = 170,
        TYPE_TESS_Base = TYPE_TESS + 1,
        TYPE_TESS_3D = TYPE_TESS + 2,
        TYPE_TESS_3D_Compressed = TYPE_TESS + 3,
        TYPE_TESS_Face = TYPE_TESS + 4,
        TYPE_TESS_3D_Wire = TYPE_TESS + 5,
        TYPE_TESS_Markup = TYPE_TESS + 6,

        TYPE_MISC = 200,
        TYPE_MISC_Attribute = TYPE_MISC + 1,	// Entity attribute.
        TYPE_MISC_CartesianTransformation = TYPE_MISC + 2,	// Cartesian transformation.
        TYPE_MISC_EntityReference = TYPE_MISC + 3,	// Entity reference.
        TYPE_MISC_MarkupLinkedItem = TYPE_MISC + 4,	// Link between a markup and an entity.
        TYPE_MISC_ReferenceOnPRCBase = TYPE_MISC + 5,	// Reference pointing on a regular entity (not topological).
        TYPE_MISC_ReferenceOnTopology = TYPE_MISC + 6,	// Reference pointing on a topological entity.
        TYPE_MISC_GeneralTransformation = TYPE_MISC + 7,	// General transformation.

        TYPE_RI = 230,
        TYPE_RI_RepresentationItem = TYPE_RI + 1,	// Basic abstract type for representation items.
        TYPE_RI_BrepModel = TYPE_RI + 2,	// Basic type for surfaces and solids.
        TYPE_RI_Curve = TYPE_RI + 3,	// Basic type for curves.
        TYPE_RI_Direction = TYPE_RI + 4,	// Optional point + vector.
        TYPE_RI_Plane = TYPE_RI + 5,	// Construction plane, as opposed to planar surface.
        TYPE_RI_PointSet = TYPE_RI + 6,	// Set of points.
        TYPE_RI_PolyBrepModel = TYPE_RI + 7,	// Basic type to polyhedral surfaces and solids.
        TYPE_RI_PolyWire = TYPE_RI + 8,	// Polyedric wireframe entity.
        TYPE_RI_Set = TYPE_RI + 9,// Logical grouping of arbitrary number of representation items.
        TYPE_RI_CoordinateSystem = TYPE_RI + 10,	// Coordinate system.

        TYPE_ASM = 300,
        TYPE_ASM_ModelFile = TYPE_ASM+1,
        TYPE_ASM_FileStructure = TYPE_ASM+2,
        TYPE_ASM_FileStructureGlobals = TYPE_ASM+3,
        TYPE_ASM_FileStructureTree = TYPE_ASM+4,
        TYPE_ASM_FileStructureTessellation = TYPE_ASM+5,
        TYPE_ASM_FileStructureGeometry = TYPE_ASM+6,
        TYPE_ASM_FileStructureExtraGeometry = TYPE_ASM+7,
        TYPE_ASM_ProductOccurrence = TYPE_ASM+10,
        TYPE_ASM_PartDefinition = TYPE_ASM+11,
        TYPE_ASM_Filter = TYPE_ASM+20,


        TYPE_MKP = 500,
        TYPE_MKP_View = TYPE_MKP + 1,	// Grouping of markup by views.
        TYPE_MKP_Markup = TYPE_MKP + 2,	// Basic type for simple markups.
        TYPE_MKP_Leader = TYPE_MKP + 3,	// basic type for markup leader
        TYPE_MKP_AnnotationItem = TYPE_MKP + 4,	// Usage of a markup.
        TYPE_MKP_AnnotationSet = TYPE_MKP + 5,	// Group of annotations.
        TYPE_MKP_AnnotationReference = TYPE_MKP + 6,	// Logical grouping of annotations for reference.

        TYPE_GRAPH = 700,
        TYPE_GRAPH_Style = TYPE_GRAPH + 1,	// Display style.
        TYPE_GRAPH_Material = TYPE_GRAPH + 2,	// Display material properties.
        TYPE_GRAPH_Picture = TYPE_GRAPH + 3,	// Picture.
        TYPE_GRAPH_TextureApplication = TYPE_GRAPH + 11,	// Texture application.
        TYPE_GRAPH_TextureDefinition = TYPE_GRAPH + 12,	// Texture definition.
        TYPE_GRAPH_TextureTransformation = TYPE_GRAPH + 13,	// Texture transformation.
        TYPE_GRAPH_LinePattern = TYPE_GRAPH + 21,	// One dimensional display style.
        TYPE_GRAPH_FillPattern = TYPE_GRAPH + 22,	// Abstract class for two-dimensional display style.
        TYPE_GRAPH_DottingPattern = TYPE_GRAPH + 23,	// Two-dimensional filling with dots.
        TYPE_GRAPH_HatchingPattern = TYPE_GRAPH + 24,	// Two-dimensional filling with hatches.
        TYPE_GRAPH_SolidPattern = TYPE_GRAPH + 25,	// Two-dimensional filling with particular style (color, material, texture).
        TYPE_GRAPH_VPicturePattern = TYPE_GRAPH + 26,	// Two-dimensional filling with vectorised picture.
        TYPE_GRAPH_AmbientLight = TYPE_GRAPH + 31,	// Scene ambient illumination.
        TYPE_GRAPH_PointLight = TYPE_GRAPH + 32,	// Scene point illumination.
        TYPE_GRAPH_DirectionalLight = TYPE_GRAPH + 33,	// Scene directional illumination.
        TYPE_GRAPH_SpotLight = TYPE_GRAPH + 34,	// Scene spot illumination.
        TYPE_GRAPH_SceneDisplayParameters = TYPE_GRAPH + 41,	// Parameters for scene visualisation.
        TYPE_GRAPH_Camera = TYPE_GRAPH + 42,	//
    };

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
        UInt entityType;
        UInt Ntokens;
        vector<UInt> tokens;

        void parse(BitStreamParser& bs) {
            entityType.parse(bs);
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

    struct Attribute {
        TypeID typeID;
        AttributeEntry title;
        UInt Npairs;
        vector<KeyValue> pairs;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_MISC_Attribute);
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
        vector<Attribute> attributes;

        void parse(BitStreamParser& bs) {
            Nattributes.parse(bs);
            //cout << " Nattributes " << Nattributes << endl;

            for (int i=0; i<Nattributes.i; i++) {
                Attribute a;
                a.parse(bs);
                attributes.push_back(a);
            }
        }
    };

    struct ContentPRCBase {
        AttributeData attribute;
        Name name;
        UInt CADID1;
        UInt CADID2;
        UInt structureID;

        void parse(BitStreamParser& bs, bool referencable) {
            attribute.parse(bs);
            name.parse(bs);
            cout << "ContentPRCBase name: " << name << endl;
            if (referencable) {
                CADID1.parse(bs);
                CADID2.parse(bs);
                structureID.parse(bs);
                cout << " ContentPRCBase IDs: " << CADID1 << ", " << CADID2 << ", " << structureID << endl;
            }
        }
    };

    struct ContentBaseTessData {
        Bool isCalculated;
        UInt Ncoords;
        ArrayOf<Double> coordinates;

        void parse(BitStreamParser& bs) {
            isCalculated.parse(bs);
            Ncoords.parse(bs);

            //cout << "   tess isCalculated: " << isCalculated << endl;
            //cout << "   tess N coords: " << Ncoords << endl;

            coordinates.parse(bs, Ncoords);

            /*for (size_t i=0; i<Ncoords.i; i++) {
                size_t Bi = bs.currentBytePtr;
                size_t bi = bs.currentBitPtr;
                //bs.printNextBits(32);

                Double d;
                bool r = d.parse(bs);
                if (!r) {
                    cout << "  failed to read double " << i << ", at: " << Bi << "." << bi << endl;
                    bs.set(Bi,bi);
                    bs.printNextBits(32);
                    break;
                }
                coordinates.push_back(d);

                //cout <<  i << " double: " << d.d << ", at: " << Bi << "." << bi << endl;
                //printBits(d.d);
                //if (i > 35) break; // TOTEST
                //break;
            }*/
        }
    };

    struct Color {
        Char R,G,B,A;

        void parse(BitStreamParser& bs, bool rgba) {
            R.parse(bs);
            G.parse(bs);
            B.parse(bs);
            if (rgba) A.parse(bs);
        }
    };

    struct ColorDataRemainder {
        Bool sameAsPrevious;
        Color color;

        void parse(BitStreamParser& bs, bool rgba) {
            sameAsPrevious.parse(bs);
            if (!sameAsPrevious.b) color.parse(bs, rgba);
        }
    };

    struct ColorData {
        Color firstColor;
        ArrayOf<ColorDataRemainder> remainingColors;

        void parse(BitStreamParser& bs, UInt Nverts, bool rgba) {
            firstColor.parse(bs, rgba);
            UInt Nvert_1;
            Nvert_1.i = Nverts.i-1;
            remainingColors.parse(bs, Nvert_1, rgba);
        }
    };

    struct VertexColors {
        Bool isRGBA;
        Bool isSegmentColor;
        Bool bOptimized;
        ArrayOf<ColorData> colorData;

        void parse(BitStreamParser& bs, UInt N) {
            isRGBA.parse(bs);
            isSegmentColor.parse(bs);
            bOptimized.parse(bs);

            if (!bOptimized.b) {
                if (isSegmentColor.b) N.i /= 2;
                colorData.parse(bs, N, N, isRGBA.b);
            }
        }
    };

    struct TessFace {
        TypeID typeID;
        UInt NlineAttributes;
        ArrayOf<UInt> lineAttibutes;
        UInt startWireData;
        UInt sizesWireSize;
        ArrayOf<UInt> sizesWire;
        UInt usedEntitiesFlag;
        UInt startTriangulated;
        UInt NtriangulatedData;
        ArrayOf<UInt> triangulatedData;
        UInt NtexCoordIndices;
        Bool hasVertexColors;
        VertexColors vertexColors;
        UInt behavior;

        void parse(BitStreamParser& bs, UInt Ncoords) {
            typeID.parse(bs, TYPE_TESS_Face);

            NlineAttributes.parse(bs);
            lineAttibutes.parse(bs, NlineAttributes);
            startWireData.parse(bs);
            sizesWireSize.parse(bs);
            sizesWire.parse(bs, sizesWireSize);
            usedEntitiesFlag.parse(bs);
            startTriangulated.parse(bs);
            NtriangulatedData.parse(bs);
            triangulatedData.parse(bs, NtriangulatedData);
            NtexCoordIndices.parse(bs);
            hasVertexColors.parse(bs);
            if (hasVertexColors.b) vertexColors.parse(bs, NtriangulatedData);
            if (NlineAttributes.i > 0) behavior.parse(bs);

            /*cout << " parse PRC_TYPE_TESS_Face " << typeID << endl;
            cout << "  NlineAttributes: " << NlineAttributes << endl;
            cout << "  startWireData: " << startWireData << endl;
            cout << "  sizesWireSize: " << sizesWireSize << endl;
            cout << "  usedEntitiesFlag: " << usedEntitiesFlag << endl;
            cout << "  startTriangulated: " << startTriangulated << endl;
            cout << "  NtriangulatedData: " << NtriangulatedData << endl;
            cout << "  NtexCoordIndices: " << NtexCoordIndices << endl;
            cout << "  hasVertexColors: " << hasVertexColors << endl;
            cout << "  behavior: " << behavior << endl;

            for (UInt& i : triangulatedData.v) cout << "   triN: " << i << endl;*/
        }
    };

    struct Tess3D {
        TypeID typeID;
        ContentBaseTessData base;
        Bool hasFaces;
        Bool hasLoops;
        Bool mustCalcNormals;
        Char normalCalcFlag; // should be 0
        Double creaseAngle;
        UInt NnormalCoords;
        ArrayOf<Double> normalCoords;
        UInt NwireIndices;
        ArrayOf<UInt> wireIndices;
        UInt NtriangleIndices; // always a multiple of 3
        ArrayOf<UInt> triangleIndices;
        UInt NfaceTesselation;
        vector<TessFace> faceTesselationData;
        UInt NtexCoords;
        ArrayOf<Double> texCoords;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_TESS_3D);
            base.parse(bs);
            hasFaces.parse(bs);
            hasLoops.parse(bs);
            mustCalcNormals.parse(bs);
            if (mustCalcNormals.b) {
                normalCalcFlag.parse(bs); // should be 0
                creaseAngle.parse(bs);
            }

            //cout << "   hasFaces " << hasFaces << endl;
            //cout << "   hasLoops " << hasLoops << endl;
            //cout << "   mustCalcNormals " << mustCalcNormals << endl;
            //cout << "   normalCalcFlag " << (unsigned char)normalCalcFlag.c << endl;
            //cout << "   creaseAngle " << creaseAngle << endl;

            NnormalCoords.parse(bs);
            normalCoords.parse(bs, NnormalCoords);
            NwireIndices.parse(bs);
            wireIndices.parse(bs, NwireIndices);
            NtriangleIndices.parse(bs);  // always a multiple of 3
            triangleIndices.parse(bs, NtriangleIndices);
            NfaceTesselation.parse(bs);

            //cout << "   NnormalCoords " << NnormalCoords << endl;
            //cout << "   NwireIndices " << NwireIndices << endl;
            //cout << "   NtriangleIndices " << NtriangleIndices << endl;
            //cout << "   NfaceTesselation " << NfaceTesselation << endl;

            for (size_t i=0; i<NfaceTesselation.i; i++) {
                TessFace face;
                face.parse(bs, base.Ncoords);
                faceTesselationData.push_back(face);
                //if (i > 0) break;
                //break;
            }

            //NtexCoords.parse(bs);
            //texCoords.parse(bs, NtexCoords);
        }

        VRGeometryPtr asGeometry() {
            VRGeoData data;

            size_t Ncoords = base.Ncoords.i / 3;
            for (size_t i = 0; i<Ncoords; i++) {
                double x = base.coordinates.v[i*3 + 0].d;
                double y = base.coordinates.v[i*3 + 1].d;
                double z = base.coordinates.v[i*3 + 2].d;
                data.pushVert(Pnt3d(x,y,z));
            }

            size_t Nnorms = NnormalCoords.i / 3;
            for (size_t i = 0; i<Nnorms; i++) {
                double x = normalCoords.v[i*3 + 0].d;
                double y = normalCoords.v[i*3 + 1].d;
                double z = normalCoords.v[i*3 + 2].d;
                data.pushNorm(Vec3d(x,y,z));
            }

            int stride = 1;
            if (!mustCalcNormals.b) stride = 2;

            for (TessFace& face : faceTesselationData) {
                UInt offset = face.startTriangulated;
                UInt pTypes = face.usedEntitiesFlag;

                UInt Nfan, Nstrip, Ntri;
                if (pTypes.i & FACETESSDATA_Triangle) Ntri = face.triangulatedData.v[0];
                if (pTypes.i & FACETESSDATA_TriangleFan) Nfan = face.triangulatedData.v[1];
                if (pTypes.i & FACETESSDATA_TriangleStripe) Nstrip = face.triangulatedData.v[ Nfan.i + 2 ];
                // TODO: check for more types!

                //cout << " N triangles: " << Ntri.i << endl;
                //cout << " N fans: " << Nfan.i << endl;
                //cout << " N strips: " << Nstrip.i << endl;

                // PRC_FACETESSDATA_Triangle (normal,point,normal,point,normal,point).
                for (size_t i = 0; i<Ntri.i; i++) {
                    size_t kN = offset.i + i*3*stride + 0;
                    size_t kP = offset.i + i*3*stride + 1;

                    size_t i1 = triangleIndices.v[kP + 0*stride].i /3;
                    size_t i2 = triangleIndices.v[kP + 1*stride].i /3;
                    size_t i3 = triangleIndices.v[kP + 2*stride].i /3;
                    if (i1 >= data.size() || i2 >= data.size() || i3 >= data.size()) {
                        cout << "  AAAAA?? " << i1 << ", " << i2 << ", " << i3 << endl;
                        continue;
                    }

                    size_t n1 = triangleIndices.v[kN + 0*stride].i /3;
                    size_t n2 = triangleIndices.v[kN + 1*stride].i /3;
                    size_t n3 = triangleIndices.v[kN + 2*stride].i /3;

                    data.pushNormalIndex(n1);
                    data.pushNormalIndex(n2);
                    data.pushNormalIndex(n3);

                    data.pushTri(i1,i2,i3);
                    //cout << "  pushTriangle " << i1 << " " << i2 << " " << i3 << ", " << n1 << " " << n2 << " " << n3 << endl;
                    /*cout << " tri " << i << ": " << endl;
                    for (int j = 0; j<stride*3; j++) {
                         cout << "  " << triangleIndices.v[i*stride*3 + j] << endl;
                    }*/
                    //if (i > 12) break;
                }
            }

            return data.asGeometry("PRC_Part");
        }
    };

    struct FileStructureTessellation {
        TypeID typeID;
        ContentPRCBase base;
        UInt Ntessellations;
        vector<Tess3D> tessellations;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_ASM_FileStructureTessellation);

            base.parse(bs, false);
            Ntessellations.parse(bs);

            for (size_t i=0; i<Ntessellations.i; i++) {
                UInt objType; objType.peek(bs);
                //cout << " tessellation " << i << ", type: " << objType.i << endl;
                if (objType.i == TYPE_TESS_3D) {
                    Tess3D t;
                    t.parse(bs);
                    tessellations.push_back(t);
                }
            }
        }
    };

    struct TopologicalContext {
        TypeID typeID;
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
            typeID.parse(bs, TYPE_TOPO_Context);
            info.parse(bs, true);
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
            //cout << " Ncontexts: " << Ncontexts << endl;
        }
    };

    struct FileStructureGeometry {
        TypeID typeID;
        ContentPRCBase info;
        FileStructureExactGeometry geometry;
        //UserData data;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_ASM_FileStructureGeometry);
            //cout << " geometry ID: " << ID << endl;
            info.parse(bs, true);
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
            //cout << "FileStructureHeader parse, p: " << p << endl;
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
            //cout << " Ntypes: " << Ntypes << endl;

            /*for (int i=0; i<Ntypes.i; i++) {
                Entity_Schema_definition d;
                d.parse(data, p);
                definitions.push_back(d);
            }*/
        }
    };

    struct GraphicsContent {
        UInt layer;
        UInt lineStyle;
        Char behaviorField1;
        Char behaviorField2;

        void parse(BitStreamParser& bs) {
            layer.parse(bs);
            lineStyle.parse(bs);
            behaviorField1.parse(bs);
            behaviorField2.parse(bs);

            cout << "GraphicsContent " << layer << ", " << lineStyle << ", " << (int)behaviorField1.c << ", " << (int)behaviorField2.c << endl;
            ;
        }
    };

    struct PRCBaseWithGraphics {
        ContentPRCBase base;
        Bool sameAsCurrent;
        GraphicsContent graphicalData;

        void parse(BitStreamParser& bs, bool referencable) {
            base.parse(bs, referencable);
            sameAsCurrent.parse(bs);
            cout << "PRCBaseWithGraphics, sameAsCurrent " << sameAsCurrent << endl;
            if (!sameAsCurrent.b) graphicalData.parse(bs);
        }
    };

    struct Vector3d {
        Double x,y,z;
        bool is3D = true;

        void parse(BitStreamParser& bs, bool is3D) {
            this->is3D = is3D;
            cout << "parseX" << endl;
            x.parse(bs);
            cout << "parseY" << endl;
            y.parse(bs);
            if (is3D) {
                cout << "parseZ" << endl;
                z.parse(bs);
            }
        }
    };

    struct BoundingBox {
        Vector3d vMin, vMax;

        void parse(BitStreamParser& bs) {
            vMin.parse(bs, true);
            vMax.parse(bs, true);

            cout << "Boundingbox " << vMin.x << "/" << vMax.x << "   " << vMin.y << "/" << vMax.y << "   " << vMin.z << "/" << vMax.z << endl;
        }
    };

    struct RepresentationItemContent {
        PRCBaseWithGraphics base;
        UInt coordSystemID;
        UInt tessellationID;

        void parse(BitStreamParser& bs, bool referencable) {
            base.parse(bs, referencable);
            coordSystemID.parse(bs);
            tessellationID.parse(bs);
        }
    };



    struct RepresentationItem {
        TypeID typeID;
        RepresentationItemContent content;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, {TYPE_RI_RepresentationItem, TYPE_RI_BrepModel, TYPE_RI_Curve, TYPE_RI_Direction, TYPE_RI_Plane, TYPE_RI_PointSet, TYPE_RI_PolyBrepModel, TYPE_RI_PolyWire, TYPE_RI_Set, TYPE_RI_CoordinateSystem });
            content.parse(bs, true);

            cout << " ---- RepresentationItem: " << typeID.i << endl;
        }
    };

    struct CompressedUniqueID {
        UInt i1,i2,i3,i4;

        void parse(BitStreamParser& bs) {
            i1.parse(bs);
            i2.parse(bs);
            i3.parse(bs);
            i4.parse(bs);
        }
    };

    struct AdditionalTargetData {
        Bool inSameStructure;
        CompressedUniqueID fileStructureID;
        UInt topologicalContext;
        UInt body;
        UInt Nindices;
        ArrayOf<UInt> indices;

        void parse(BitStreamParser& bs) {
            inSameStructure.parse(bs);
            if (!inSameStructure.b) fileStructureID.parse(bs);
            topologicalContext.parse(bs);
            body.parse(bs);
            Nindices.parse(bs);
            indices.parse(bs, Nindices);
        }
    };

    struct ReferenceOnTopology {
        TypeID typeID;
        UInt entityType;
        Bool hasExactGeometry;
        AdditionalTargetData data;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_MISC_ReferenceOnTopology);
            entityType.parse(bs);
            hasExactGeometry.parse(bs);
            if (hasExactGeometry.b) data.parse(bs);
        }
    };

    struct ReferenceOnPRCBase {
        TypeID typeID;
        UInt entityType;
        Bool inSameFileStrucure;
        CompressedUniqueID fileStructureID;
        UInt ID;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_MISC_ReferenceOnPRCBase);
            entityType.parse(bs);
            inSameFileStrucure.parse(bs);
            if (!inSameFileStrucure.b) fileStructureID.parse(bs);
            ID.parse(bs);
        }
    };

    struct ReferenceData {
        ReferenceOnTopology refTopology;
        ReferenceOnPRCBase refBase;

        void parse(BitStreamParser& bs) { // TODO: options
            refTopology.parse(bs);
            refBase.parse(bs);
        }
    };

    struct ContentEntityReference {
        PRCBaseWithGraphics base;
        UInt coordSystem;
        Bool refExists;
        ReferenceData entityID;

        void parse(BitStreamParser& bs, bool referencabe) {
            base.parse(bs, referencabe);
            coordSystem.parse(bs);
            refExists.parse(bs);
            if (refExists.b) entityID.parse(bs);
        }
    };

    struct ContentExtendedEntityReference {
        TypeID typeID;
        ContentEntityReference refOccurence;
        ReferenceData refData;

        void parse(BitStreamParser& bs, bool referencabe) {
            typeID.parse(bs, TYPE_MISC_MarkupLinkedItem);
            refOccurence.parse(bs, referencabe);
            refData.parse(bs);
        }
    };

    struct UserDataSubSection {
        Bool sameUUID;
        CompressedUniqueID uniqueID;
        UInt Nbits;
        BITS bits;

        void parse(BitStreamParser& bs) {
            sameUUID.parse(bs);
            if (!sameUUID.b) uniqueID.parse(bs);
            Nbits.parse(bs);
            if (Nbits.i > 0) bits.parse(bs, Nbits);
        }
    };

    struct UserDataStream {
        UInt Nsubs;
        ArrayOf<UserDataSubSection> sections;

        void parse(BitStreamParser& bs) {
            Nsubs.parse(bs);
            sections.parse(bs, Nsubs);
        }
    };

    struct UserData {
        UInt Nbits;
        UserDataStream data;

        void parse(BitStreamParser& bs) {
            Nbits.parse(bs);
            if (Nbits.i > 0) data.parse(bs);
        }
    };

    struct MarkupLinkedItem {
        TypeID typeID;
        ContentExtendedEntityReference entityRef;
        Bool showMarkup;
        Bool deleteMarkup;
        Bool showLeader;
        Bool deleteLeader;
        UserData userData;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_MISC_MarkupLinkedItem);
            entityRef.parse(bs, false);
            showMarkup.parse(bs);
            deleteMarkup.parse(bs);
            showLeader.parse(bs);
            deleteLeader.parse(bs);
            userData.parse(bs);
        }
    };

    struct ReferenceUniqueIdentifier {
        TypeID typeID;
        UInt refType;
        Bool inSameFileStructure;
        CompressedUniqueID fileStructureID;
        UInt entityID;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_MISC_ReferenceOnPRCBase);
            refType.parse(bs);
            inSameFileStructure.parse(bs);
            if (!inSameFileStructure.b) fileStructureID.parse(bs);
            entityID.parse(bs);
        }
    };

    struct MarkupLeader {
        TypeID typeID;
        PRCBaseWithGraphics base;
        ArrayOf<ReferenceUniqueIdentifier> itemIDs;
        //ArrayOf<ReferenceUniqueIdentifier> leaderIDs;
        UInt tessellationID;
        UserData userData;

        void parse(BitStreamParser& bs, UInt N1) {
            typeID.parse(bs, TYPE_MKP_Leader);
            /*base.parse(bs, true);
            itemIDs.parse(bs, N1);
            //leaderIDs.parse(bs, N2); // ????
            tessellationID.parse(bs);
            userData.parse(bs);*/
        }
    };

    struct Markup {
        TypeID typeID;
        PRCBaseWithGraphics base;
        UInt type;
        UInt subType;
        UInt Nitems;
        ArrayOf<ReferenceUniqueIdentifier> items;
        UInt Nleaders;
        ArrayOf<ReferenceUniqueIdentifier> leaders;
        UInt tessellationID;
        UserData data;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_MKP_Markup);
            base.parse(bs, true);
            type.parse(bs);
            subType.parse(bs);
            Nitems.parse(bs);
            items.parse(bs, Nitems);
            Nleaders.parse(bs);
            leaders.parse(bs, Nleaders);
            tessellationID.parse(bs);
            data.parse(bs);
        }
    };

    struct AnnotationItem {
        TypeID typeID;
        PRCBaseWithGraphics base;
        ReferenceUniqueIdentifier refID;
        UserData data;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_MKP_AnnotationItem);
            base.parse(bs, true);
            refID.parse(bs);
            data.parse(bs);
        }
    };

    struct AnnotationSet {
        TypeID typeID;
        PRCBaseWithGraphics base;
        UInt Nannotations;
        ArrayOf<AnnotationItem> items; // TODO: should be any of AnnotationEntities
        UserData data;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_MKP_AnnotationSet);
            base.parse(bs, true);
            Nannotations.parse(bs);
            items.parse(bs, Nannotations);
            data.parse(bs);
        }
    };

    struct AnnotationReference {
        TypeID typeID;
        PRCBaseWithGraphics base;
        UInt Nitems;
        ArrayOf<ReferenceUniqueIdentifier> uniqueIdentifiers;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_MKP_AnnotationReference);
            base.parse(bs, true);
            Nitems.parse(bs);
            uniqueIdentifiers.parse(bs, Nitems);
        }
    };

    struct MarkupData {
        UInt Nitems;
        ArrayOf<MarkupLinkedItem> items;
        UInt Nleaders;
        ArrayOf<MarkupLeader> leaders;
        UInt Nmarkups;
        ArrayOf<Markup> markups;
        UInt NannotationEntities;
        ArrayOf<AnnotationItem> annotationEntities; // TODO: should be any of AnnotationEntities

        void parse(BitStreamParser& bs) {
            Nitems.parse(bs);
            cout << "MarkupData Nitems " << Nitems << endl;
            items.parse(bs, Nitems);
            Nleaders.parse(bs);
            cout << "MarkupData Nleaders " << Nleaders << endl;
            Nleaders.i = 1; // TOTEST
            leaders.parse(bs, Nleaders, Nitems);
            /*Nmarkups.parse(bs);
            markups.parse(bs, Nmarkups);
            NannotationEntities.parse(bs);
            annotationEntities.parse(bs, NannotationEntities);*/
        }
    };

    struct Transformation {
        Char behavior;

        bool isIdentity = true;
        bool hasTranslation = false;
        bool hasRotation = false;
        bool hasMirror = false;
        bool hasUniformScale = false;
        bool hasNonUniformScale = false;
        bool isNonOrthogonal = false;
        bool isHomogenous = false;

        Vector3d translation;
        Vector3d rotationX;
        Vector3d rotationY;
        Vector3d rotationZ;
        Double scale;
        Vector3d scale3;
        Vector3d homogenousXYZ;
        Double homogenousW;

        void parse(BitStreamParser& bs, bool is3D) {
            behavior.parse(bs);
            isIdentity = bool(behavior.c & PRC_TRANSFORMATION_Identity);
            hasTranslation = bool(behavior.c & PRC_TRANSFORMATION_Translate);
            hasRotation = bool(behavior.c & PRC_TRANSFORMATION_Rotate);
            hasMirror = bool(behavior.c & PRC_TRANSFORMATION_Mirror);
            hasUniformScale = bool(behavior.c & PRC_TRANSFORMATION_Scale);
            hasNonUniformScale = bool(behavior.c & PRC_TRANSFORMATION_NonUniformScale);
            isNonOrthogonal = bool(behavior.c & PRC_TRANSFORMATION_NonOrtho);
            isHomogenous = bool(behavior.c & PRC_TRANSFORMATION_Homogeneous);

            if (hasTranslation) translation.parse(bs, is3D);

            if (isNonOrthogonal) {
                rotationX.parse(bs, is3D);
                rotationY.parse(bs, is3D);
                if (is3D) rotationZ.parse(bs, true);
            } else if (hasRotation) {
                rotationX.parse(bs, is3D);
                rotationY.parse(bs, is3D);
            }

            if (hasNonUniformScale) scale3.parse(bs, is3D);
            else if (hasUniformScale) scale.parse(bs);

            if (isHomogenous) {
                homogenousXYZ.parse(bs, is3D);
                homogenousW.parse(bs);
            }
        }
    };

    struct Domain {
        Vector3d vMin, vMax;

        void parse(BitStreamParser& bs) {
            vMin.parse(bs, false);
            vMax.parse(bs, false);
        }
    };

    struct BaseGeometry {
        Bool hasBase;
        AttributeData attribute;
        Name name;
        UInt cadID;

        void parse(BitStreamParser& bs) {
            hasBase.parse(bs);
            if (hasBase.b) {
                attribute.parse(bs);
                name.parse(bs);
                cadID.parse(bs);
            }
        }
    };

    struct ContentSurface {
        BaseGeometry geometry;
        UInt extention;

        void parse(BitStreamParser& bs) {
            geometry.parse(bs);
            extention.parse(bs);
        }
    };

    struct SurfPlane {
        TypeID typeID;
        ContentSurface surface;
        Transformation transformation;
        Domain domain;
        Double coeffAU;
        Double coeffAV;
        Double coeffBU;
        Double coeffBV;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_SURF_Plane);
            surface.parse(bs);
            transformation.parse(bs, true);
            domain.parse(bs);
            coeffAU.parse(bs);
            coeffAV.parse(bs);
            coeffBU.parse(bs);
            coeffBV.parse(bs);
        }
    };

    struct LightObject {
        TypeID typeID;
        ContentPRCBase base;
        UInt ambientID;
        UInt diffuseID;
        UInt emissiveID;
        UInt specularID;

        Vector3d location;
        Vector3d direction;
        Vector3d attenuation;
        Double intensity;
        Double fallOffAngle;
        Double fallOffExponent;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, { TYPE_GRAPH_AmbientLight, TYPE_GRAPH_PointLight, TYPE_GRAPH_DirectionalLight, TYPE_GRAPH_SpotLight } );

            base.parse(bs, true);
            ambientID.parse(bs);
            diffuseID.parse(bs);
            emissiveID.parse(bs);
            specularID.parse(bs);

            if (typeID.i == TYPE_GRAPH_PointLight) {
                location.parse(bs, true);
                attenuation.parse(bs, true);
            }

            if (typeID.i == TYPE_GRAPH_DirectionalLight) {
                direction.parse(bs, true);
                intensity.parse(bs);
            }

            if (typeID.i == TYPE_GRAPH_SpotLight) {
                location.parse(bs, true);
                attenuation.parse(bs, true);
                direction.parse(bs, true);
                fallOffAngle.parse(bs);
                fallOffExponent.parse(bs);
            }
        }
    };

    struct Camera {
        TypeID typeID;
        ContentPRCBase base;
        Bool orthographic;
        Vector3d position;
        Vector3d lookAt;
        Vector3d upVector;
        Double fovX;
        Double fovY;
        Double aspectRation;
        Double near;
        Double far;
        Double zoom;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_GRAPH_Camera);
            base.parse(bs, true);
            orthographic.parse(bs);
            position.parse(bs, true);
            lookAt.parse(bs, true);
            upVector.parse(bs, true);
            fovX.parse(bs);
            fovY.parse(bs);
            aspectRation.parse(bs);
            near.parse(bs);
            far.parse(bs);
            zoom.parse(bs);
        }
    };

    struct SceneDisplayParameters {
        TypeID typeID;
        ContentPRCBase base;
        Bool isActive;
        UInt Nlights;
        ArrayOf<LightObject> lights;
        Bool hasCamera;
        Camera camera;
        Bool hasRotationCenter;
        Vector3d rotationCenter;
        UInt NclippingPlanes;
        ArrayOf<SurfPlane> clippingPlanes;
        UInt backgroundLineStyle;
        UInt defaultLineStyle;
        UInt Nstyles;
        ArrayOf<UInt> typeStyles;
        Bool absolutePositions;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_GRAPH_SceneDisplayParameters);
            base.parse(bs, true);
            isActive.parse(bs);
            Nlights.parse(bs);
            lights.parse(bs, Nlights);
            hasCamera.parse(bs);
            if (hasCamera.b) camera.parse(bs);
            hasRotationCenter.parse(bs);
            if (hasRotationCenter.b) rotationCenter.parse(bs, true);
            NclippingPlanes.parse(bs);
            clippingPlanes.parse(bs, NclippingPlanes);
            backgroundLineStyle.parse(bs);
            defaultLineStyle.parse(bs);
            Nstyles.parse(bs);
            Nstyles.i *= 2; // next array has UInt pairs
            typeStyles.parse(bs, Nstyles);
            absolutePositions.parse(bs);
        }
    };

    struct ContentLayerFilterItems {
        Bool isInclusive;
        UInt Nlayers;
        ArrayOf<UInt> layers;

        void parse(BitStreamParser& bs) {
            isInclusive.parse(bs);
            Nlayers.parse(bs);
            layers.parse(bs, Nlayers);
        }
    };

    struct EntityReference {
        TypeID typeID;
        ContentEntityReference entityRef;
        UserData data;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_MISC_EntityReference);
            entityRef.parse(bs, true);
            data.parse(bs);
        }
    };

    struct ContentEntityFilterItems {
        Bool isInclusive;
        UInt Nentities;
        ArrayOf<EntityReference> entities;

        void parse(BitStreamParser& bs) {
            isInclusive.parse(bs);
            Nentities.parse(bs);
            entities.parse(bs, Nentities);
        }
    };

    struct Filter {
        TypeID typeID;
        ContentPRCBase base;
        ContentLayerFilterItems layerFilter;
        ContentEntityFilterItems entityFilter;
        UserData userData;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_ASM_Filter);
            base.parse(bs, true);
            layerFilter.parse(bs);
            entityFilter.parse(bs);
            userData.parse(bs);
        }
    };

    struct View {
        TypeID typeID;
        PRCBaseWithGraphics base;
        UInt Nannotations;
        ArrayOf<ReferenceUniqueIdentifier> annotationIDs;
        SurfPlane annotationPlane;
        Bool hasParameters;
        SceneDisplayParameters displayParameters;
        Bool isAnnotationView;
        Bool isDefaultView;
        Bool isDirection;
        UInt Nitems;
        ArrayOf<ReferenceUniqueIdentifier> linkedItems;
        UInt Nfilters;
        ArrayOf<Filter> filters;
        UserData userData;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_MKP_View);
            base.parse(bs, true);
            Nannotations.parse(bs);
            annotationIDs.parse(bs, Nannotations);
            annotationPlane.parse(bs);
            hasParameters.parse(bs);
            displayParameters.parse(bs);
            isAnnotationView.parse(bs);
            isDefaultView.parse(bs);
            isDirection.parse(bs);
            Nitems.parse(bs);
            linkedItems.parse(bs, Nitems);
            Nfilters.parse(bs);
            filters.parse(bs, Nfilters);
            userData.parse(bs);
        }
    };

    struct PartDefinition {
        TypeID typeID;
        PRCBaseWithGraphics base;
        BoundingBox bbox;
        UInt Nitems;
        ArrayOf<RepresentationItem> items;
        MarkupData markup;
        UInt Nviews;
        ArrayOf<View> views;
        UserData userData;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_ASM_PartDefinition);
            cout << "PartDefinition - typeID: " << typeID.i << "/" << TYPE_ASM_PartDefinition << endl;

            base.parse(bs, true);
            bs.printNextBits(1024);
            bbox.parse(bs);
            Nitems.parse(bs);

            cout << "Nitems " << Nitems.i << endl;

            items.parse(bs, Nitems);
            markup.parse(bs);
            /*Nviews.parse(bs);
            views.parse(bs, Nviews);
            userData.parse(bs);*/

        }
    };

    struct SaveFileIdentifier {
        Bool inSameFilestructure;
        CompressedUniqueID fileStructureID;

        void parse(BitStreamParser& bs) {
            inSameFilestructure.parse(bs);
            if (!inSameFilestructure.b) fileStructureID.parse(bs);
        }
    };

    struct ReferencesOfProductOcurrence {
        UInt partID;
        UInt prototypeID;
        SaveFileIdentifier prototypeSave;
        UInt externalID;
        SaveFileIdentifier externalSave;
        UInt Nchildren;
        ArrayOf<UInt> childOccurences;

        void parse(BitStreamParser& bs) {
            partID.parse(bs);
            prototypeID.parse(bs);
            prototypeSave.parse(bs);
            externalID.parse(bs);
            externalSave.parse(bs);
            Nchildren.parse(bs);
            childOccurences.parse(bs, Nchildren);
        }
    };

    struct ProductInformation {
        Bool fromCADFile;
        Double Units;
        Char flags;
        Int loadStatus;

        void parse(BitStreamParser& bs) {
            fromCADFile.parse(bs);
            Units.parse(bs);
            flags.parse(bs);
            loadStatus.parse(bs);
        }
    };

    struct ProductOccurence {
        TypeID typeID;
        PRCBaseWithGraphics base;
        ReferencesOfProductOcurrence references;
        Char behavior;
        ProductInformation information;
        Bool hasTransformation;
        Transformation transformation;
        UInt Nreferences;
        ArrayOf<EntityReference> entityReferences;
        MarkupData markup;
        UInt Nviews;
        ArrayOf<View> views;
        Bool hasFilter;
        Filter filter;
        UInt Nfilters;
        ArrayOf<Filter> filters;
        UInt NdisplayParameters;
        ArrayOf<SceneDisplayParameters> displayParameters;
        UserData useData;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_ASM_ProductOccurrence);
            base.parse(bs, true);
            references.parse(bs);
            behavior.parse(bs);
            information.parse(bs);
            hasTransformation.parse(bs);
            if (hasTransformation.b) transformation.parse(bs, true);
            Nreferences.parse(bs);
            entityReferences.parse(bs, Nreferences);
            markup.parse(bs);
            Nviews.parse(bs);
            views.parse(bs, Nviews);
            hasFilter.parse(bs);
            if (hasFilter.b) filter.parse(bs);
            Nfilters.parse(bs);
            filters.parse(bs, Nfilters);
            NdisplayParameters.parse(bs);
            displayParameters.parse(bs, NdisplayParameters);
            useData.parse(bs);
        }
    };

    struct ASMFileStructure {
        TypeID typeID;
        ContentPRCBase base;
        UInt nextIndex;
        UInt productOccurenceID;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_ASM_FileStructure);
            base.parse(bs, false);
            nextIndex.parse(bs);
            productOccurenceID.parse(bs);
        }
    };

    struct FileStructureTree {
        TypeID typeID;
        ContentPRCBase base;
        UInt Nparts;
        ArrayOf<PartDefinition> partDefintions;
        UInt Noccurences;
        ArrayOf<ProductOccurence> productOccurences;
        ASMFileStructure fileStructureData;
        UserData useData;

        void parse(BitStreamParser& bs) {
            typeID.parse(bs, TYPE_ASM_FileStructureTree);
            base.parse(bs, false);
            Nparts.parse(bs);

            cout << "FileStructureTree - typeID: " << typeID.i << "/" << TYPE_ASM_FileStructureTree << endl;
            cout << "Nparts " << Nparts << endl;

            partDefintions.parse(bs, Nparts);
            /*Noccurences.parse(bs);
            productOccurences.parse(bs, Noccurences);
            fileStructureData.parse(bs);
            useData.parse(bs);*/

        }
    };

    struct FileStructure {
        FileStructureHeader header;
        //PRC_TYPE_ASM_FileStructureGlobals globals;
        FileStructureTree tree;
        FileStructureTessellation tessellation;
        FileStructureGeometry geometry;
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

VRTransformPtr parsePRCStructure(string& data, PRC::FileStructureDescription& description) {
    if (description.Nsections.i != 6) return 0;
    if (description.sectionOffsets.size() != 7) return 0;

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

    //cout << "Header start: " << startHeader << ", size: " << sizeHeader << endl;
    //cout << "Globals start: " << startGlobals << ", size: " << sizeGlobals << endl;
    //cout << "Tree start: " << startTree << ", size: " << sizeTree << endl;
    //cout << "Tessellations start: " << startTessellations << ", size: " << sizeTessellations << endl;
    //cout << "Geometries start: " << startGeometries << ", size: " << sizeGeometries << endl;
    //cout << "ExtraGeometries start: " << startExtraGeometries << ", size: " << sizeExtraGeometries << endl;

    size_t current = startHeader;
    auto& h = structure.header;
    h.parse(data, current);

    //cout << "  structure head " << string(h.PRC, 3) << endl;
    //cout << "  structure minVersion: " << h.minVersion << endl;
    //cout << "  structure authVersion: " << h.authVersion << endl;
    //cout << "  structure fileID: " << h.fileID << endl;
    //cout << "  structure appID: " << h.appID << endl;
    //cout << "  structure Nfiles: " << h.Nfiles << endl;


    auto extractSection = [&](size_t& a, size_t N) -> string {
        //cout << "try to uncompress section, from " << a << " -> " << a+N << " ( " << N << " )";
        string udata(data.begin()+a, data.begin()+a+N);
        udata = decompressZlib(udata);
        //cout << " uncompressed data size: " << udata.size() << endl;
        a += N;
        return udata;
    };

    string dataGlobals = extractSection(current, sizeGlobals);
    string dataTree = extractSection(current, sizeTree);
    string dataTessellations = extractSection(current, sizeTessellations);
    string dataGeometries = extractSection(current, sizeGeometries);
    string dataExtraGeometries = extractSection(current, sizeExtraGeometries);

    BitStreamParser bstr(0, dataTree.size(), (unsigned char*)&dataTree[0]);
    BitStreamParser bst(0, dataTessellations.size(), (unsigned char*)&dataTessellations[0]);
    BitStreamParser bsg(0, dataGeometries.size(), (unsigned char*)&dataGeometries[0]);

    //structure.tree.parse(bstr);
    structure.tessellation.parse(bst);
    //structure.geometry.parse(bsg);


    auto root = VRTransform::create("Object");

    for (PRC::Tess3D& t : structure.tessellation.tessellations) {
        root->addChild( t.asGeometry() );
    }

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

    return root;
}

VRTransformPtr processPRC(PDF::Object& object) {
    if (object.streams.size() == 0) return 0;
    PDF::Stream& stream = object.streams[0];
    if (stream.unpacked == "") return 0;


    auto root = VRTransform::create("PRC");

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

    size_t i=0;
    for (PRC::FileStructureDescription& description : header.structureDescriptions) {
        //if (i == 16) { // TOTEST
            cout << "Process Part " << i << endl;
            auto t = parsePRCStructure(stream.unpacked, description);
            root->addChild(t);
        //}
        i += 1;
    }

    return root;
}


bool testDouble(double d) {
    BitWriter s = writeDouble(d);
    BitWriter s2 = writeDouble2(d);
    s.printBits();
    s2.printBits();

    BitStreamParser bs(0, s.buffer.size(), (unsigned char*)&s.buffer[0]);
    bool success;
    double r = readDouble(bs, success);
    cout << endl << " test double: " << d << " -> " << r << endl;
    cout << " bits1 : "; printBits(d); cout << endl;
    cout << " bits2 : "; printBits(r); cout << endl;
    cout << endl << endl;
    return (abs(d - r) < 1e-3);
}

bool testUInt(unsigned int i) {
    string s = writeUnsignedInteger(i);
    BitStreamParser bs(0, s.size(), (unsigned char*)&s[0]);
    PRC::UInt I;
    I.parse(bs);
    cout << " test UInt: " << i << " -> " << I.i << endl;
    //::printBits(s);
    return (i == I.i);
}

VRTransformPtr VRPDF::extract3DModels(string path) {
    //string s = writeUnsignedInteger(232);
    //::printBits(s);

    //testDouble(-85); return 0;
    //testDouble(355); return 0;
    //testDouble(-62.5); return 0;
    //testDouble(-61.9527); return 0;
    //testDouble(134.248); return 0;
    //testDouble(3.5); return 0;
    //testUInt(305);

    //return 0;

    auto root = VRTransform::create("PDF");

    ifstream file(path, std::ios::binary);
    vector<char> buffer((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

    vector<PDF::Object> objects = extractObjects(buffer);
    extractObjectNames(buffer, objects);
    extractStreams(buffer, objects);
    vector<PDF::Object> geometries = filter3DObjects(objects);
    for (auto& obj : geometries) unpack3DObject(buffer, obj);
    for (auto& obj : geometries) {
        auto model = processPRC(obj);
        root->addChild(model);
    }

    // TODO: convert PRC to geometry

    //printObjects(objects);
    //printObjects(geometries);

    return root;
}





