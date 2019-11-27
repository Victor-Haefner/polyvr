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

//template<> string typeName(const OSMMap& t) { return "OSMMap"; }
template<> string typeName(const OSMMap& o) { return "OSMMap"; }
template<> string typeName(const OSMWay& o) { return "OSMWay"; }
template<> string typeName(const OSMNode& o) { return "OSMNode"; }
template<> string typeName(const OSMBase& o) { return "OSMBase"; }

OSG_BEGIN_NAMESPACE;
 class OSMSAXParser : public SAXParser {
    private:
        //OSMSAXHandlerCP* handler;

    public:
        OSMSAXParser();
};

class OSMSAXHandlerCP : public HandlerBase {
    private:
        long numerator;
        long numeratorWritten;
        long nodeCounter;
        long wayCounter;
        long relCounter;
        long nodeCounterRaw;
        long wayCounterRaw;
        long relCounterRaw;
        int counter;
        int currentDepth; //Depth: 0 = OSM/XML, 1 = Node/Ways/Relations, 2 = Tags/Refs
        int currentType; //error = -1, node = 0, way = 1, relation = 2
        int lastDepth;
        bool lvl1Open;
        bool lvl1Closer;
        bool lvl1Closed;
        string currentID ="";
            //currentElementPtr;
        string newfilepath = "";
        string buffer = "";
        vector<string> element;

        //map<string, string> whitelist;
        vector<pair<string, string>> whitelist;
        map<string, string> miscAtts;
        map<string, string> tagsInfo;
        vector<string> refsForWays;
        vector<string> nodesForRelations;
        vector<string> waysForRelations;
        map<string, bool> neededNodes;
        bool writeToFile = true;
        bool sndPass = false;

        void writeLine(string line);
        void writeLineToFile(string line);
        void writeElementToFile();
        void handleElement();

        void init();

    public:
        OSMSAXHandlerCP();
        void startDocument();
        void startElement(const XMLCh* const, AttributeList&);
        void endElement(const XMLCh* const);
        void endDocument();
        void fatalError(const SAXParseException&);

        long getNumerator();
        long getNumeratorWritten();
        long getNodeCounter();
        long getWayCounter();
        long getRelationCounter();
        long getNodeRawCounter();
        long getWayRawCounter();
        long getRelationRawCounter();
        void setWhitelist(vector<pair<string, string>> whl);
        void setWriteToFile(bool in);
        void setPass(bool in);
        void setNewPath(string in);
        map<string,bool> getNeededNodes();
};

class OSMSAXHandlerBM : public HandlerBase {
    private:
        long numerator;
        long numeratorWritten;
        long nodeCounter;
        long wayCounter;
        long relCounter;
        int counter;
        int currentDepth; //Depth: 0 = OSM/XML, 1 = Node/Ways/Relations, 2 = Tags/Refs
        int currentType; //error = -1, node = 0, way = 1, relation = 2
        int lastDepth;
        bool lvl1Open;
        bool lvl1Closer;
        bool lvl1Closed;
        string currentID ="";
            //currentElementPtr;
        vector<string> element;

        map<string, string> miscAtts;
        map<string, string> tagsInfo;
        vector<string> refsForWays;
        vector<string> nodesForRelations;
        vector<string> waysForRelations;

        map<string, OSMNodePtr> nodes;
        map<string, OSMWayPtr> ways;
        map<string, OSMRelationPtr> relations;

        void handleElement();

        void init();

    public:
        OSMSAXHandlerBM();
        void startDocument();
        void startElement(const XMLCh* const, AttributeList&);
        void endElement(const XMLCh* const);
        void endDocument();
        void fatalError(const SAXParseException&);

        map<string, OSMNodePtr> getNodes();
        map<string, OSMWayPtr> getWays();
        map<string, OSMRelationPtr> getRelations();

        long getNumerator();
        long getNodeCounter();
        long getWayCounter();
        long getRelationCounter();
};
OSG_END_NAMESPACE;

OSMSAXParser::OSMSAXParser() {
    //cout << "OSMSAXParser::OSMSAXParser" << endl;
}

OSMSAXHandlerCP::OSMSAXHandlerCP() {
}

void OSMSAXHandlerCP::init(){
    numerator = 0;
    numeratorWritten = 0;
    nodeCounter = 0;
    wayCounter = 0;
    relCounter = 0;
    nodeCounterRaw = 0;
    wayCounterRaw = 0;
    relCounterRaw = 0;
    counter = 0;
    currentDepth = -1; //Depth: 0 = OSM/XML, 1 = Node/Ways/Relations, 2 = Tags/Refs
    currentType = -1; //error = -1, node = 0, way = 1, relation = 2
    lastDepth = -1;

    lvl1Open = false;
    lvl1Closer = false;
    lvl1Closed = true;

    buffer = "";

    miscAtts.clear();
    tagsInfo.clear();
    refsForWays.clear();
    nodesForRelations.clear();
    waysForRelations.clear();

    element.clear();
}

void OSMSAXHandlerCP::handleElement(){
    ///IMPLEMENT WHITELIST/BLACKLIST HERE
    bool force = false; //whitelist

    //checks taglist for wanted tags
    auto checkTag = [&](string key, string val) {
        if ( force ) return true;
        if ( tagsInfo.count(key) ) {
            if ( tagsInfo[key] == val ) return true;
            if ( val.length()<1 ) return true;
        }
        return false;
    };

    auto checkWhiteList = [&](){
        if ( force ) return true;
        bool res = false;
        for (auto each : whitelist) {
            res = checkTag(each.first, each.second);
            if (res) return true;
        }
        return res;
    };

    //bounds
    if ( currentType == -2 ) {
        force = true;
    }

    //nodes
    if ( currentType == 0 ) {
        nodeCounterRaw++;
        if ( !sndPass ) {
            force = checkWhiteList();
            if ( force ) neededNodes[currentID] = true;
            force = true;
        } else {
            if (neededNodes.count(currentID)) force = true;
        }
        if ( force ) nodeCounter++;
    }

    //ways
    if ( currentType == 1 ) {
        wayCounterRaw++;
        if ( !sndPass ) {
            force = checkWhiteList();
            if ( force ) {
                for (auto each: refsForWays) {
                    neededNodes[each] = true;
                }
            }
        } else {
            force = true;
        }
        if ( force ) wayCounter++;
    }

    //relations
    if ( currentType == 2 ) {
        relCounterRaw++;
        if ( !sndPass ) {
            force = checkWhiteList();
            if ( force ) {
                for (auto each: nodesForRelations) {
                    neededNodes[each] = true;
                }
            }
        } else {
            force = true;
        }
        if ( force ) relCounter++;
    }

    if ( force ) writeElementToFile();
    miscAtts.clear();
    tagsInfo.clear();
    refsForWays.clear();
    nodesForRelations.clear();
    waysForRelations.clear();

    element.clear();
    numerator++;
}

void OSMSAXHandlerCP::writeLine(string line) {
    element.push_back(line);
}

void OSMSAXHandlerCP::writeLineToFile(string line) {
    if (writeToFile) {
        std::ofstream ofs;
        //XMLCh *xmlLine = XMLString::transcode();
        ofs.open (newfilepath, std::ofstream::out | std::ofstream::app);
        ofs << line << std::endl;
        //XMLString::release(&xmlLine);
        ofs.close();
    }
}

void OSMSAXHandlerCP::writeElementToFile() {
    for (auto each : element) {
        writeLineToFile(each);
    }
    numeratorWritten++;
}

long OSMSAXHandlerCP::getNumerator() { return numerator; }
long OSMSAXHandlerCP::getNumeratorWritten() { return numeratorWritten; }
long OSMSAXHandlerCP::getNodeCounter() { return nodeCounter; }
long OSMSAXHandlerCP::getNodeRawCounter() { return nodeCounterRaw; }
long OSMSAXHandlerCP::getWayCounter() { return wayCounter; }
long OSMSAXHandlerCP::getWayRawCounter() { return wayCounterRaw; }
long OSMSAXHandlerCP::getRelationCounter() { return relCounter; }
long OSMSAXHandlerCP::getRelationRawCounter() { return relCounterRaw; }
map<string,bool> OSMSAXHandlerCP::getNeededNodes() { return neededNodes; }
void OSMSAXHandlerCP::setWhitelist(vector<pair<string, string>> whl) { whitelist = whl; }
void OSMSAXHandlerCP::setWriteToFile(bool in) { writeToFile = in; }
void OSMSAXHandlerCP::setPass(bool in) { sndPass = in; }
void OSMSAXHandlerCP::setNewPath(string in) { newfilepath = in; }

void OSMSAXHandlerCP::startDocument(){
    init();
    //cout << "OSMSAXHandlerCP::startDocument" << endl;
    if ( writeToFile ) std::ofstream file { newfilepath.c_str() };
    writeLineToFile("<?xml version='1.0' encoding='UTF-8'?>");
}

void OSMSAXHandlerCP::endDocument(){
    writeLineToFile("</osm>");
    //cout << "OSMSAXHandlerCP::endDocument with " << numerator << " elements" << endl;
}

void OSMSAXHandlerCP::startElement(const XMLCh* const name, AttributeList& attributes) {
    currentDepth++;
    counter = 0;
    char* message = XMLString::transcode(name);
    std::string msgAsString(message);
    int attributeLength = attributes.getLength();

    auto convertString = [&](string str) {
        string res = "";
        for (size_t i = 0; i < str.length(); i++) {
            if ( str.at(i) == '"' ) res+= "&quot;"; else
            if ( str.at(i) == '\'' ) res+= "&apos;"; else
            if ( str.at(i) == '<' ) res+= "&lt;"; else
            if ( str.at(i) == '>' ) res+= "&gt;"; else
            if ( str.at(i) == '&' ) res+= "&amp;"; else
            res += str.at(i);
        }
        return res;
    };

    auto readAttributes = [&](){
        XMLSize_t index = 0;
        string res = " ";
        string tmpKey = ""; //Tag
        string tmpVal = ""; //Tag
        string tmpType = ""; //Relation
        string tmpRef = ""; //Relation
        string tmpRole = ""; //Relation
        for (int i = 0; i < attributeLength; i++) {
            auto key = XMLString::transcode(attributes.getName(index));
            std::string keyAsString(key);
            string keyXML = convertString(keyAsString);
            res+=keyXML+"=";
            auto val = XMLString::transcode(attributes.getValue(index));
            std::string valAsString(val);
            string valXML = convertString(valAsString);
            res+="'"+valXML+"'";

            if (keyAsString == "id") {
                currentID = valAsString;
                lvl1Open = true;
            }
            if ( currentDepth == 1 ) miscAtts[keyAsString] = valAsString;
            if ( msgAsString == "tag" ) {
                if ( keyAsString == "k" ) { tmpKey = valAsString; }
                if ( keyAsString == "v" ) { tmpVal = valAsString; }
            }
            if ( msgAsString == "member" ) {
                if ( keyAsString == "type" ) { tmpType = valAsString; }
                if ( keyAsString == "ref" ) { tmpRef = valAsString; }
                if ( keyAsString == "role" ) { tmpRole = valAsString; }
            }
            if ( msgAsString == "nd" ) { refsForWays.push_back(valAsString); }

            if ( i < attributeLength - 1) res+=" ";
            index++;
            counter++;
        }
        if ( msgAsString == "tag" ) tagsInfo[tmpKey] = tmpVal;
        if ( msgAsString == "member" ) {
            if (tmpType == "node") nodesForRelations.push_back(tmpRef);
        }
        return res;
    };

    string atts = readAttributes();
    if ( currentDepth > 0 && currentDepth > lastDepth && lastDepth > 0 ) {
        buffer += ">";
        writeLine(buffer);
    } else
    if ( currentDepth == 1 && currentDepth == lastDepth && !lvl1Closer ) {
        buffer += " />";
        writeLine(buffer);
        handleElement(); ///END OF ELEMENT
    } else
    if ( currentDepth == 1 && currentDepth > lastDepth && lastDepth == 0 ) {
        if ( currentType == -2 ) {
            buffer += " />";
            writeLine(buffer);
            handleElement(); ///END OF ELEMENT
        }
    }


    if ( currentDepth == 0 ) {
        if ( msgAsString == "osm" )currentType = -3; //OSM
        buffer = "<";
        buffer += msgAsString;
        buffer += readAttributes();
        buffer += ">";
        writeLineToFile(buffer);
        miscAtts.clear();
        tagsInfo.clear();
        element.clear();
    }
    if ( currentDepth == 1 ) {
        if ( counter>0 ) {
            buffer = "  <";
            buffer += msgAsString;
            buffer+= atts;
            if ( msgAsString == "bounds" ) currentType = -2; else //nodes
            if ( msgAsString == "node" ) currentType = 0; else //nodes
            if ( msgAsString == "way" ) currentType = 1; else //ways
            if ( msgAsString == "relation" ) currentType = 2; else //relations
            currentType = -1; //unknown
        }
    } else
    if ( currentDepth == 2 ) {
        buffer = "    <";
        buffer += msgAsString;
        buffer+= readAttributes();
    } else
    if ( currentDepth > 2 ) {
        //cout << "OSMSAXHandlerCP::startElement unknown found: " << msgAsString << endl;
        //cout << "   at level: " << currentDepth << endl;
    }
    lastDepth = currentDepth;

    XMLString::release(&message);
}

void OSMSAXHandlerCP::endElement(const XMLCh* const name) {
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
            handleElement(); ///END OF ELEMENT
        }
        if ( currentDepth == lastDepth ) {
            lvl1Closer = false;
        }
        if ( lastDepth == 0 && currentType == 1 ){
            lvl1Closer = false;
        }
    } else
    if ( currentDepth == 2 ) {
        buffer += " />";
        writeLine(buffer);
    } else
    if ( currentDepth > 2 ) {
        cout << "OSMSAXHandlerCP::startElement unknown found: " << msgAsString << endl;
        cout << "   at level: " << currentDepth << endl;
    }

    lastDepth = currentDepth;
    currentDepth--;
    XMLString::release(&message);
}

void OSMSAXHandlerCP::fatalError(const SAXParseException& exception) {
    char* message = XMLString::transcode(exception.getMessage());
    cout << "Fatal Error: " << message
         << " at line: " << exception.getLineNumber()
         << endl;
    XMLString::release(&message);
}


OSMSAXHandlerBM::OSMSAXHandlerBM() {
}

void OSMSAXHandlerBM::init(){
    numerator = 0;
    nodeCounter = 0;
    wayCounter = 0;
    relCounter = 0;
    counter = 0;
    currentDepth = -1; //Depth: 0 = OSM/XML, 1 = Node/Ways/Relations, 2 = Tags/Refs
    currentType = -1; //error = -1, node = 0, way = 1, relation = 2
    lastDepth = -1;

    lvl1Open = false;
    lvl1Closer = false;
    lvl1Closed = true;

    miscAtts.clear();
    tagsInfo.clear();
    refsForWays.clear();
    nodesForRelations.clear();
    waysForRelations.clear();

    element.clear();
}

void OSMSAXHandlerBM::handleElement(){
    //bounds
    if ( currentType == -2 ) {}

    //nodes
    if ( currentType == 0 ) {
        double lat = atof(miscAtts["lat"].c_str());
        double lon = atof(miscAtts["lon"].c_str());
        OSMNodePtr node = OSMNodePtr( new OSMNode(currentID, lat, lon) );
        node->tags = tagsInfo;
        nodes[node->id] = node;
        nodeCounter++;
    }

    //ways
    if ( currentType == 1 ) {
        OSMWayPtr way = OSMWayPtr( new OSMWay(currentID) );
        way->nodes = refsForWays;
        ways[way->id] = way;
        wayCounter++;
    }

    //relations
    if ( currentType == 2 ) {
        OSMRelationPtr rel = OSMRelationPtr( new OSMRelation(currentID) );
        rel->nodes = nodesForRelations;
        rel->ways = waysForRelations;
        relations[rel->id] = rel;
        relCounter++;
    }
    miscAtts.clear();
    tagsInfo.clear();
    refsForWays.clear();
    nodesForRelations.clear();
    waysForRelations.clear();

    element.clear();
    numerator++;
}

map<string, OSMNodePtr> OSMSAXHandlerBM::getNodes() { return nodes; }
map<string, OSMWayPtr> OSMSAXHandlerBM::getWays()  { return ways; }
map<string, OSMRelationPtr> OSMSAXHandlerBM::getRelations() { return relations; }

long OSMSAXHandlerBM::getNumerator() { return numerator; }
long OSMSAXHandlerBM::getNodeCounter() { return nodeCounter; }
long OSMSAXHandlerBM::getWayCounter() { return wayCounter; }
long OSMSAXHandlerBM::getRelationCounter() { return relCounter; }

void OSMSAXHandlerBM::startDocument(){
    init();
    //cout << "OSMSAXHandlerBM::startDocument" << endl;
}

void OSMSAXHandlerBM::endDocument(){
    //cout << "OSMSAXHandlerBM::endDocument with " << numerator << " elements" << endl;
}

void OSMSAXHandlerBM::startElement(const XMLCh* const name, AttributeList& attributes) {
    currentDepth++;
    counter = 0;
    char* message = XMLString::transcode(name);
    std::string msgAsString(message);
    int attributeLength = attributes.getLength();

    auto readAttributes = [&](){
        XMLSize_t index = 0;
        string tmpKey = ""; //Tag
        string tmpVal = ""; //Tag
        string tmpType = ""; //Relation
        string tmpRef = ""; //Relation
        string tmpRole = ""; //Relation
        for (int i = 0; i < attributeLength; i++) {
            auto key = XMLString::transcode(attributes.getName(index));
            std::string keyAsString(key);
            auto val = XMLString::transcode(attributes.getValue(index));
            std::string valAsString(val);

            if (keyAsString == "id") {
                currentID = valAsString;
                lvl1Open = true;
            }
            if ( currentDepth == 1 ) miscAtts[keyAsString] = valAsString;
            if ( msgAsString == "tag" ) {
                if ( keyAsString == "k" ) { tmpKey = valAsString; }
                if ( keyAsString == "v" ) { tmpVal = valAsString; }
            }
            if ( msgAsString == "member" ) {
                if ( keyAsString == "type" ) { tmpType = valAsString; }
                if ( keyAsString == "ref" ) { tmpRef = valAsString; }
                if ( keyAsString == "role" ) { tmpRole = valAsString; }
            }
            if ( msgAsString == "nd" ) { refsForWays.push_back(valAsString); }

            index++;
            counter++;
        }
        if ( msgAsString == "tag" ) tagsInfo[tmpKey] = tmpVal;
        if ( msgAsString == "member" ) {
            if (tmpType == "node") nodesForRelations.push_back(tmpRef);
            if (tmpType == "way") waysForRelations.push_back(tmpRef);
        }
        return;
    };

    readAttributes();
    if ( currentDepth > 0 && currentDepth > lastDepth && lastDepth > 0 ) {
    } else {
        if ( currentDepth == 1 && currentDepth == lastDepth && !lvl1Closer ) {
            handleElement(); ///END OF ELEMENT
        }
    }

    if ( currentDepth == 0 ) {
        currentType = -3; //OSM
    }
    if ( currentDepth == 1 ) {
        if ( counter>0 ) {
            if ( msgAsString == "bounds" ) currentType = -2; else //nodes
            if ( msgAsString == "node" ) currentType = 0; else //nodes
            if ( msgAsString == "way" ) currentType = 1; else //ways
            if ( msgAsString == "relation" ) currentType = 2; else //relations
            currentType = -1; //unknown
        }
    } else
    if ( currentDepth == 2 ) {
    } else
    if ( currentDepth > 2 ) {
        //cout << "OSMSAXHandlerBM::startElement unknown found: " << msgAsString << endl;
        //cout << "   at level: " << currentDepth << endl;
    }
    lastDepth = currentDepth;

    XMLString::release(&message);
}

void OSMSAXHandlerBM::endElement(const XMLCh* const name) {
    char* message = XMLString::transcode(name);
    std::string msgAsString(message);

    if ( currentDepth == 0 ) {
        //cout << buffer << endl;
    }
    if ( currentDepth == 1 ) {
        if ( currentDepth < lastDepth  ) {
            lvl1Closer = true;
            handleElement(); ///END OF ELEMENT
        }
        if ( currentDepth == lastDepth ) {
            lvl1Closer = false;
        }
    } else
    if ( currentDepth == 2 ) {
    } else
    if ( currentDepth > 2 ) {
        cout << "OSMSAXHandlerBM::startElement unknown found: " << msgAsString << endl;
        cout << "   at level: " << currentDepth << endl;
    }

    lastDepth = currentDepth;
    currentDepth--;
    XMLString::release(&message);
}

void OSMSAXHandlerBM::fatalError(const SAXParseException& exception) {
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
    string res = "tags:";
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

VRPolygon OSMWay::getPolygon() { return polygon; }
vector<string> OSMWay::getNodes() { return nodes; }

string OSMRelation::toString() {
    string res = OSMBase::toString();
    return res;
}

bool OSMBase::hasTag(const string& t) {
    return tags.count(t) > 0;
}

map<string, string> OSMBase::getTags() { return tags; }

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
    auto getFileSize = [&](string filename) {
        struct stat stat_buf;
        int rc = stat(filename.c_str(), &stat_buf);
        return rc == 0 ? stat_buf.st_size : -1;
    };
    if ( getFileSize(filepath) < 10000000 ) readFile(filepath); //10Mb
    else readFileStreaming(filepath);
}

OSMMap::OSMMap() {
}

OSMMapPtr OSMMap::create() {
    return OSMMapPtr( new OSMMap() );
}

OSMMapPtr OSMMap::create(string filepath) {
    return OSMMapPtr( new OSMMap(filepath) );
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
    VRTimer t; t.start();
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

    auto t2 = t.stop()/1000.0;
    cout << "OSMMap::readFile path " << path << endl;
    cout << "  loaded " << ways.size() << " ways, " << nodes.size() << " nodes and " << relations.size() << " relations" << endl;
    cout << "  secs needed: " << t2 << endl;
}

int OSMMap::readFileStreaming(string path) {
    filepath = path;
    VRTimer t; t.start();
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

    //DocumentHandler* docHandler = new OSMSAXHandlerCP();
    OSMSAXHandlerBM* docHandler = new OSMSAXHandlerBM();
    ErrorHandler* errHandler = (ErrorHandler*) docHandler;
    parser->setDocumentHandler(docHandler);
    parser->setErrorHandler(errHandler);

    try {
        cout << "OSMMap::readFileStreaming - " << filepath << endl;
        parser->parse(path.c_str());
        nodes = docHandler->getNodes();
        ways = docHandler->getWays();
        relations = docHandler->getRelations();
        cout << "OSMMap::readFileStreaming - elements read: " << docHandler->getNumerator() << endl;
        cout << "OSMMap::readFileStreaming - nodes: " << docHandler->getNodeCounter() << ", ways: " << docHandler->getWayCounter() <<  ", relations: " << docHandler->getRelationCounter() << endl;
    }
    catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        cout << "OSMMap::readFileStreaming Exception message is: \n"
             << message << "\n";
        XMLString::release(&message);
        return -1;
    }
    catch (const SAXParseException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        cout << "OSMMap::readFileStreaming Exception message is: \n"
             << message << "\n";
        XMLString::release(&message);
        return -1;
    }
    catch (...) {
        cout << "OSMMap::readFileStreaming Unexpected Exception \n" ;
        return -1;
    }

    delete parser;
    delete docHandler;
    auto t2 = t.stop()/1000.0;
    cout << "OSMMap::readFileStreaming - secs needed: " << t2 << endl;
    return 0;
}

int OSMMap::filterFileStreaming(string path, vector<pair<string, string>> whitelist) {
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

    //DocumentHandler* docHandler = new OSMSAXHandlerCP();
    OSMSAXHandlerCP* docHandler = new OSMSAXHandlerCP();
    ErrorHandler* errHandler = (ErrorHandler*) docHandler;
    parser->setDocumentHandler(docHandler);
    parser->setErrorHandler(errHandler);

    docHandler->setWhitelist(whitelist);

    auto genPath = [&](string in){
        string osm = "";
        string res = "";
        for (int i = 0; i < 4; i++){
            osm = filepath.at(filepath.length()-1-i) + osm;
        }

        if (osm != ".osm") {
            res+="newOSM.osm";
            return res;
        }
        for (int i = 0; i < filepath.length()-4; i++) {
            res += filepath.at(i);
        }
        res += in;
        res += ".osm";
        return res;
    };

    string tempPath = genPath("_temp");
    try {
        VRTimer t; t.start();
        cout << "OSMMap::copyFileStreaming 1st Pass - " << filepath << endl;
        docHandler->setNewPath(tempPath);
        parser->parse(path.c_str());
        auto t2 = t.stop()/1000;
        cout << "OSMMap::copyFileStreaming 1st Pass - secs needed: " << t2 << endl;
        cout << "OSMMap::copyFileStreaming 1st Pass - elements read: " << docHandler->getNumerator() << ", elements written: " << docHandler->getNumeratorWritten() << endl;
        cout << "OSMMap::copyFileStreaming 1st Pass - read: nodes: " << docHandler->getNodeRawCounter() << ", ways: " << docHandler->getWayRawCounter() <<  ", relations: " << docHandler->getRelationRawCounter() << endl;
        cout << "OSMMap::copyFileStreaming 1st Pass - writ: nodes: " << docHandler->getNodeCounter() << ", ways: " << docHandler->getWayCounter() <<  ", relations: " << docHandler->getRelationCounter() << endl;
    }
    catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        cout << "OSMMap::copyFileStreaming 1st Pass Exception message is: \n"
             << message << "\n";
        XMLString::release(&message);
        return -1;
    }
    catch (const SAXParseException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        cout << "OSMMap::copyFileStreaming 1st Pass Exception message is: \n"
             << message << "\n";
        XMLString::release(&message);
        return -1;
    }
    catch (...) {
        cout << "OSMMap::copyFileStreaming 1st Pass Unexpected Exception \n" ;
        return -1;
    }

    try {
        VRTimer t; t.start();
        docHandler->setPass(true);
        docHandler->setNewPath(genPath("_filtered"));
        cout << "OSMMap::copyFileStreaming 2nd Pass - " << filepath << endl;
        parser->parse(tempPath.c_str());
        auto t2 = t.stop()/1000;
        cout << "OSMMap::copyFileStreaming 2nd Pass - secs needed: " << t2 << endl;
        cout << "OSMMap::copyFileStreaming 2nd Pass - elements read: " << docHandler->getNumerator() << ", elements written: " << docHandler->getNumeratorWritten() << endl;
        cout << "OSMMap::copyFileStreaming 2nd Pass - read: nodes: " << docHandler->getNodeRawCounter() << ", ways: " << docHandler->getWayRawCounter() <<  ", relations: " << docHandler->getRelationRawCounter() << endl;
        cout << "OSMMap::copyFileStreaming 2nd Pass - writ: nodes: " << docHandler->getNodeCounter() << ", ways: " << docHandler->getWayCounter() <<  ", relations: " << docHandler->getRelationCounter() << endl;
    }
    catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        cout << "OSMMap::copyFileStreaming 2nd Pass Exception message is: \n"
             << message << "\n";
        XMLString::release(&message);
        return -1;
    }
    catch (const SAXParseException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        cout << "OSMMap::copyFileStreaming 2nd Pass Exception message is: \n"
             << message << "\n";
        XMLString::release(&message);
        return -1;
    }
    catch (...) {
        cout << "OSMMap::copyFileStreaming 2nd Pass Unexpected Exception \n" ;
        return -1;
    }

    delete parser;
    delete docHandler;
    return 0;
}

void OSMMap::filterFileStreaming(string path, vector<vector<string>> wl) {
    vector<pair<string, string>> whitelist;
    for (auto each : wl){
        whitelist.push_back(pair<string, string>(each[0],each[1]));
    }
    /*
    vector<pair<string, string>> whitelist;
    whitelist.push_back(pair<string, string>("admin_level","2"));
    whitelist.push_back(pair<string, string>("admin_level","4"));
    whitelist.push_back(pair<string, string>("admin_level","5"));
    whitelist.push_back(pair<string, string>("admin_level","6"));
    whitelist.push_back(pair<string, string>("highway","motorway"));
    whitelist.push_back(pair<string, string>("railway","rail"));
    whitelist.push_back(pair<string, string>("place","city"));*/
    filterFileStreaming(path, whitelist);
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
