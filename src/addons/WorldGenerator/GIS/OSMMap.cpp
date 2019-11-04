#include "OSMMap.h"
#include "core/utils/toString.h"
#include "core/math/boundingbox.h"
#include "core/utils/VRTimer.h"
#include <libxml++/libxml++.h>

// The XML parser headers
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/parsers/SAXParser.hpp>
#include <xercesc/util/XMLString.hpp>
#include <iostream>

#include <fstream>

/* FILE FORMAT INFOS:
    http://wiki.openstreetmap.org/wiki/Elements
    http://wiki.openstreetmap.org/wiki/Map_Features
*/

using namespace xercesc;
using namespace OSG;


OSG_BEGIN_NAMESPACE;
 class OSMSAXParser : public SAXParser {
    private:
        //OSMSAXHandler* handler;

    public:
        OSMSAXParser();
};

class OSMSAXHandler : public HandlerBase {
    private:
        long numerator = 0;
        int counter = 0;
        int currentDepth = -1; //Depth: 0 = OSM/XML, 1 = Node/Ways/Relations, 2 = Tags/Refs
        int currentType = -1; //error = -1, node = 0, way = 1, relation = 2
        int lastDepth = -1;
            //currentElementPtr;
        string newfilepath = "../data/osm/test2.osm";
        string lastParent;
        string buffer = "";
        vector<string> element;
        bool lvl1Open = false;
        bool lvl1Closer = false;
        bool lvl1Closed = true;
        map<string, string> whitelist;

        void writeLine(string line);
        void handleElement();

    public:
        OSMSAXHandler();
        void startDocument();
        void startElement(const XMLCh* const, AttributeList&);
        void endElement(const XMLCh* const);
        void endDocument();
        void fatalError(const SAXParseException&);
        long getNumerator();
        void setWhitelist(map<string, string> whl);
};
OSG_END_NAMESPACE;

OSMSAXParser::OSMSAXParser() {
    cout << "OSMSAXParser::OSMSAXParser" << endl;
}

OSMSAXHandler::OSMSAXHandler() {
}

void OSMSAXHandler::handleElement(){
    ///IMPLEMENT WHITELIST HERE
    for (auto each : element) {
        cout << each << endl;
    }
    element.clear();
    numerator++;
}

void OSMSAXHandler::writeLine(string line) {
    //if (numerator < 30) cout << line << endl;
    std::ofstream ofs;
    ofs.open (newfilepath, std::ofstream::out | std::ofstream::app);
    ofs << line << std::endl;
    element.push_back(line);
    ofs.close();
}

long OSMSAXHandler::getNumerator() { return numerator; }
void OSMSAXHandler::setWhitelist(map<string, string> whl) { whitelist = whl; }

void OSMSAXHandler::startDocument(){
    std::ofstream file { newfilepath.c_str() };
    cout << "OSMSAXHandler::startDocument" << endl;
    buffer = "<?xml version='1.0' encoding='UTF-8'?>";
    writeLine(buffer);
}

void OSMSAXHandler::endDocument(){
    buffer = "</osm>";
    writeLine(buffer);
    cout << "OSMSAXHandler::endDocument" << endl;
}

void OSMSAXHandler::startElement(const XMLCh* const name, AttributeList& attributes) {
    currentDepth++;
    counter = 0;
    char* message = XMLString::transcode(name);
    std::string msgAsString(message);
    int attributeLength = attributes.getLength();

    auto readAttributes = [&](){
        XMLSize_t index = 0;
        string res = " ";
        for (int i = 0; i < attributeLength; i++) {
            auto test0 = XMLString::transcode(attributes.getName(index));
            std::string test0AsString(test0);
            res+=test0AsString+"=";
            if (test0AsString == "id") lvl1Open = true;
            auto test1 = XMLString::transcode(attributes.getValue(index));
            std::string test1AsString(test1);
            res+="'"+test1AsString+"'";
            if ( i < attributeLength - 1) res+=" ";
            index++;
            counter++;
        }
        return res;
    };

    string atts = readAttributes();
    if ( currentDepth > 0 && currentDepth > lastDepth ) {
        buffer += ">";
        writeLine(buffer);
    } else {
        if ( currentDepth == 1 && currentDepth == lastDepth && !lvl1Closer ) {
            buffer += " />";
            writeLine(buffer);
            handleElement()
        }
    }

    if ( currentDepth == 0 ) {
        buffer = "<";
        buffer += msgAsString;
        buffer += readAttributes();
    }
    if ( currentDepth == 1 ) {
        //if ( attributeLength > 0 ) lvl1NeedsClose = true;
        //
        if ( msgAsString == "node" ) {
            currentType == 0;
            //cout << "n"<<attributeLength;
        } else
        if ( msgAsString == "way" ) {
            currentType == 1;
            //cout << "w"<<attributeLength;
        } else
        if ( msgAsString == "relation" ) {
            currentType == 2;
            //cout << "r"<<attributeLength;
        } else {
            currentType == -1;
            cout << "OSMSAXHandler::startElement unnknown element found on level 1: " << msgAsString << endl;
        }
        if ( counter>0 ) {
            buffer = "  <";
            buffer += msgAsString;
            lastParent = msgAsString;
            buffer+= atts;
        }
    } else
    if ( currentDepth == 2 ) {
        buffer = "    <";
        buffer += msgAsString;
        //cout << " dep2:" <<currentType;
        if ( currentType == 0 ) {
            //cout << "n"<<attributeLength;
        }//cout << "node stuff found: " << msgAsString << endl; } else
        if ( currentType == 1 ) {
            //cout << "w"<<attributeLength;
        }//cout << "way stuff found: " << msgAsString << endl; } else
        if ( currentType == 2 ) {
            //cout << "r"<<attributeLength;
        }//cout << "relation stuff found: " << msgAsString << endl; } else {
            //cout << "OSMSAXHandler::startElement unknown information found: " << msgAsString <<" - "<< lastParent << endl;
        //}
        if ( currentType < 0 || currentType > 2 ) {
            //cout << "f"<<attributeLength;
        }
        buffer+= readAttributes();
    } else
    if ( currentDepth > 2 ) {
        cout << "OSMSAXHandler::startElement unknown found: " << msgAsString << endl;
        cout << "   at level: " << currentDepth << endl;
    }


    //auto test0 = XMLString::transcode(attributes.getValue(index));
    //cout << " " << test0;
    XMLString::release(&message);
}

void OSMSAXHandler::endElement(const XMLCh* const name) {
    char* message = XMLString::transcode(name);
    std::string msgAsString(message);

    if ( currentDepth == 0 ) {
        //cout << buffer << endl;
    }
    if ( currentDepth == 1 ) {
        if ( currentDepth < lastDepth  ) {
            buffer = "  </";
            buffer += msgAsString;
            buffer +=">";
            lvl1Closer = true;
            writeLine(buffer);
            handleElement();
        }
        if ( currentDepth == lastDepth ) {
            lvl1Closer = false;
        }
    } else
    if ( currentDepth == 2 ) {
        buffer += " />";
        writeLine(buffer);
    } else
    if ( currentDepth > 2 ) {
        cout << "OSMSAXHandler::startElement unknown found: " << msgAsString << endl;
        cout << "   at level: " << currentDepth << endl;
    }

    lastDepth = currentDepth;
    currentDepth--;
    XMLString::release(&message);
}

void OSMSAXHandler::fatalError(const SAXParseException& exception) {
    char* message = XMLString::transcode(exception.getMessage());
    cout << "Fatal Error: " << message
         << " at line: " << exception.getLineNumber()
         << endl;
    XMLString::release(&message);
}

OSMBase::OSMBase(string id) : id(id) {}
OSMNode::OSMNode(string id, double lat, double lon) : OSMBase(id), lat(lat), lon(lon) {}
OSMWay::OSMWay(string id) : OSMBase(id) {}
OSMRelation::OSMRelation(string id) : OSMBase(id) {}

OSMBase::OSMBase(xmlpp::Element* el) {
    id = el->get_attribute_value("id");
    for(xmlpp::Node* n : el->get_children()) { // read node tags
        if (auto e = dynamic_cast<xmlpp::Element*>(n)) {
            if (e->get_name() == "tag") {
                tags[e->get_attribute_value("k")] = e->get_attribute_value("v");
            }
        }
    }
}

string OSMBase::toString() {
    string res;
    for (auto t : tags) res += " " + t.first + ":" + t.second;
    return res;
}

string OSMNode::toString() {
    string res = OSMBase::toString();
    res += " N" + ::toString(lat) + " E" + ::toString(lon);
    return res;
}

string OSMWay::toString() {
    string res = OSMBase::toString() + " nodes:";
    for (auto n : nodes) res += " " + n;
    return res;
}

string OSMRelation::toString() {
    string res = OSMBase::toString();
    return res;
}

bool OSMBase::hasTag(const string& t) {
    return tags.count(t) > 0;
}

OSMNode::OSMNode(xmlpp::Element* el) : OSMBase(el) {
    toValue(el->get_attribute_value("lat"), lat);
    toValue(el->get_attribute_value("lon"), lon);
}

OSMWay::OSMWay(xmlpp::Element* el, map<string, bool>& invalidIDs) : OSMBase(el) {
    for(xmlpp::Node* n : el->get_children()) {
        if (auto e = dynamic_cast<xmlpp::Element*>(n)) {
            if (e->get_name() == "tag") continue;
            if (e->get_name() == "nd") {
                string nID = e->get_attribute_value("ref");
                if (invalidIDs.count(nID)) continue;
                nodes.push_back(nID);
                continue;
            }
            cout << " OSMWay::OSMWay, unhandled element: " << e->get_name() << endl;
        }
    }
}

OSMRelation::OSMRelation(xmlpp::Element* el, map<string, bool>& invalidIDs) : OSMBase(el) {
    for(xmlpp::Node* n : el->get_children()) {
        if (auto e = dynamic_cast<xmlpp::Element*>(n)) {
            if (e->get_name() == "tag") continue;
            if (e->get_name() == "member") {
                string type = e->get_attribute_value("type");
                string eID = e->get_attribute_value("ref");
                if (invalidIDs.count(eID)) continue;
                if (type == "way") ways.push_back(eID);
                if (type == "node") nodes.push_back(eID);
                continue;
            }
            cout << " OSMRelation::OSMRelation, unhandled element: " << e->get_name() << endl;
        }
    }
}

OSMMap::OSMMap(string filepath) {
    readFile(filepath);
}

OSMMap::OSMMap(string filepath, bool stream) {
    if (stream) readStreamFile(filepath);
    else readFile(filepath);
}

OSMMap::OSMMap(string filepath, bool stream, map<string, string> whitelist) {
    readStreamFile(filepath, whitelist);
}

void OSMMap::clear() {
    bounds->clear();
    ways.clear();
    nodes.clear();
}

bool OSMMap::isValid(xmlpp::Element* e) {
    if (e->get_attribute("action")) {
        if (e->get_attribute_value("action") == "delete") {
            invalidElements[e->get_attribute_value("id")] = true;
            return false;
        }
    }
    return true;
};

void OSMMap::readFile(string path) {
    filepath = path;
    bounds = Boundingbox::create();

    xmlpp::DomParser parser;
    try { parser.parse_file(filepath); }
    catch(const exception& ex) { cout << "OSMMap Error: " << ex.what() << endl; return; }

    auto data = parser.get_document()->get_root_node()->get_children();
    for (auto enode : data) {
        if (auto element = dynamic_cast<xmlpp::Element*>(enode)) {
            if (!isValid(element)) continue;
            if (element->get_name() == "node") { readNode(element); continue; }
            if (element->get_name() == "bounds") { readBounds(element); continue; }
            //cout << " OSMMap::readFile, unhandled element: " << element->get_name() << endl;
        }
    }
    for (auto enode : data) {
        if (auto element = dynamic_cast<xmlpp::Element*>(enode)) {
            if (!isValid(element)) continue;
            if (element->get_name() == "way") { readWay(element, invalidElements); continue; }
            //cout << " OSMMap::readFile, unhandled element: " << element->get_name() << endl;
        }
    }
    for (auto enode : data) {
        if (auto element = dynamic_cast<xmlpp::Element*>(enode)) {
            if (!isValid(element)) continue;
            if (element->get_name() == "relation") { readRelation(element, invalidElements); continue; }
            //cout << " OSMMap::readFile, unhandled element: " << element->get_name() << endl;
        }
    }

    for (auto way : ways) {
        for (auto nID : way.second->nodes) {
            auto n = getNode(nID);
            if (!n) { /*cout << " Error in OSMMap::readFile: no node with ID " << nID << endl;*/ continue; }
            way.second->polygon.addPoint(Vec2d(n->lon, n->lat));
            n->ways.push_back(way.second->id);
        }
    }

    cout << "OSMMap::readFile path " << path << endl;
    cout << "  loaded " << ways.size() << " ways, " << nodes.size() << " nodes and " << relations.size() << " relations" << endl;
}

int OSMMap::readStreamFile(string path) {
    filepath = path;
    try {
        XMLPlatformUtils::Initialize();
    }
    catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        cout << "Error during initialization! :\n"
             << message << "\n";
        XMLString::release(&message);
        return 1;
    }

    OSMSAXParser* parser = new OSMSAXParser();
    //parser->setDoValidation(true);
    parser->setDoNamespaces(true);    // optional

    //DocumentHandler* docHandler = new OSMSAXHandler();
    OSMSAXHandler* docHandler = new OSMSAXHandler();
    ErrorHandler* errHandler = (ErrorHandler*) docHandler;
    parser->setDocumentHandler(docHandler);
    parser->setErrorHandler(errHandler);


    try {
        VRTimer t; t.start();
        cout << "OSMMap::readStreamFile - " << filepath << endl;
        //auto t1 = t.start()/1000;
        //cout << "started at: " << t1 << endl;
        parser->parse(path.c_str());
        auto t2 = t.stop()/1000;
        cout << "OSMMap::readStreamFile - secs needed: " << t2 << ", with elements: " << docHandler->getNumerator() << endl;
    }
    catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        cout << "Exception message is: \n"
             << message << "\n";
        XMLString::release(&message);
        return -1;
    }
    catch (const SAXParseException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        cout << "Exception message is: \n"
             << message << "\n";
        XMLString::release(&message);
        return -1;
    }
    catch (...) {
        cout << "Unexpected Exception \n" ;
        return -1;
    }

    delete parser;
    delete docHandler;
    return 0;
}

int OSMMap::readStreamFile(string path, map<string, string> whitelist) {
    return 0;
}

template <class Key, class Value>
unsigned long mapSize(const map<Key,Value> &map){
    unsigned long size = 0;
    for(auto it = map.begin(); it != map.end(); ++it){
        size += it->first.capacity();
        size += sizeof(it->second);
    }
    return size;
}

template <class Value>
unsigned long vecSize(const std::vector<Value> &vec){
    unsigned long size = 0;
    for(auto it = vec.begin(); it != vec.end(); ++it){
        size += it->capacity();
    }
    return size;
}

double OSMMap::getMemoryConsumption() {
    double res = sizeof(*this);

    res += filepath.capacity();
    res += sizeof(*bounds);
    res += mapSize(ways);
    res += mapSize(nodes);
    res += mapSize(relations);
    res += mapSize(invalidElements);

    for (auto& w : ways) if (w.second) res += mapSize(w.second->tags) + vecSize(w.second->nodes);
    for (auto& n : nodes) if (n.second) res += mapSize(n.second->tags) + vecSize(n.second->ways);
    for (auto& r : relations) if (r.second) res += mapSize(r.second->tags) + vecSize(r.second->ways) + vecSize(r.second->nodes);

    return res/1048576.0;
}

OSMMapPtr OSMMap::loadMap(string filepath) { return OSMMapPtr( new OSMMap(filepath) ); }
OSMMapPtr OSMMap::parseMap(string filepath) { return OSMMapPtr( new OSMMap(filepath, true) ); }
OSMMapPtr OSMMap::shrinkMap(string filepath, map<string, string> whitelist) { return OSMMapPtr( new OSMMap(filepath, true, whitelist) ); }
map<string, OSMWayPtr> OSMMap::getWays() { return ways; }
map<string, OSMNodePtr> OSMMap::getNodes() { return nodes; }
map<string, OSMRelationPtr> OSMMap::getRelations() { return relations; }
OSMNodePtr OSMMap::getNode(string id) { return nodes[id]; }
OSMWayPtr OSMMap::getWay(string id) { return ways[id]; }
OSMRelationPtr OSMMap::getRelation(string id) { return relations[id]; }
void OSMMap::reload() { clear(); readFile(filepath); }

vector<OSMWayPtr> OSMMap::splitWay(OSMWayPtr way, int segN) {
    vector<OSMWayPtr> res;
    int segL = way->nodes.size()/segN;

    int k = 0;
    OSMWayPtr w = 0;
    for (int s=0; s<segN && k<way->nodes.size(); s++) {
        w = OSMWayPtr( new OSMWay(way->id + "_" + toString(s)) );
        w->tags = way->tags;
        ways[w->id] = w;
        for (int i=0; i<segL; i++) {
            if (i == 0 && k != 0) k--;
            w->nodes.push_back( way->nodes[k] );
            k++;
            if (k >= way->nodes.size()) break;
        }
        for (auto n : w->nodes) {
            int N = nodes[n]->ways.size();
            for (int i=0; i<N; i++) {
                if (nodes[n]->ways[i] == way->id) { nodes[n]->ways[i] = w->id; break; }
                else if (i == N-1) { nodes[n]->ways.push_back(w->id); break; }
            }
        }
        res.push_back(w);
    }

    ways.erase(way->id);
    for (; k<way->nodes.size(); k++) w->nodes.push_back( way->nodes[k] );
    return res;
}

void OSMMap::readNode(xmlpp::Element* element) {
    OSMNodePtr node = OSMNodePtr( new OSMNode(element) );
    nodes[node->id] = node;
}

void OSMMap::readWay(xmlpp::Element* element, map<string, bool>& invalidIDs) {
    OSMWayPtr way = OSMWayPtr( new OSMWay(element, invalidIDs) );
    ways[way->id] = way;
}

void OSMMap::readRelation(xmlpp::Element* element, map<string, bool>& invalidIDs) {
    OSMRelationPtr rel = OSMRelationPtr( new OSMRelation(element, invalidIDs) );
    relations[rel->id] = rel;
}

void OSMMap::readBounds(xmlpp::Element* element) {
    Vec3d min(toFloat( element->get_attribute_value("minlon") ), toFloat( element->get_attribute_value("minlat") ), 0 );
    Vec3d max(toFloat( element->get_attribute_value("maxlon") ), toFloat( element->get_attribute_value("maxlat") ), 0 );
    bounds->clear();
    bounds->update(min);
    bounds->update(max);
}
