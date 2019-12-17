#include "xml.h"

#include <libxml/tree.h>


XMLElement::XMLElement(_xmlNode* node) : node(node) {}
XMLElement::~XMLElement() {}

XMLElementPtr XMLElement::create(_xmlNode* node) { return XMLElementPtr( new XMLElement(node) ); }

string XMLElement::getName() { return string((const char*)node->name); }
string XMLElement::getNameSpace() { return string((const char*)node->ns); }

_xmlNode* getNextNode(_xmlNode* cur) {
    while ( cur && xmlIsBlankNode(cur) ) cur = cur->next;
    return cur;
}

vector<XMLElementPtr> XMLElement::getChildren() {
    vector<XMLElementPtr> res;
    auto cnode = getNextNode( node->xmlChildrenNode );
    while (cnode) {
        res.push_back(XMLElement::create(cnode));
        cnode = getNextNode( cnode->next );
    }
    return res;
}


XML::XML() {}
XML::~XML() {}

XMLPtr XML::create() { return XMLPtr( new XML() ); }

void XML::read(string path) {
    xmlDocPtr doc = xmlParseFile(path.c_str());
    xmlNodePtr xmlRoot = xmlDocGetRootElement(doc);
    root = XMLElement::create(xmlRoot);

    xmlFreeDoc(doc);
}

void XML::write(string path) { // TODO
    ;
}

XMLElementPtr XML::getRoot() { return root; }
