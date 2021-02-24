#include "VRCrystal.h"
#include "core/utils/toString.h"
#include <string>
#include <fstream>
#include <streambuf>

using namespace OSG;

VRCrystal::VRCrystal(string name) : VRMolecule(name) {}
VRCrystal::~VRCrystal(){}

VRCrystalPtr VRCrystal::create(string name) { return VRCrystalPtr(new VRCrystal(name)); }

struct CIFdata {
    map<string, string> parameters;
    vector< map<string, vector<string> > > blocks; // stores each block as table, column wise
};

CIFdata parseCIFfile(string path) {

    ifstream f(path);
    string data((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());

    CIFdata model;

    // temp vars
    vector<string> blockHead;
    map<string, vector<string> > currentBlock;

    auto parseParameter = [&](const string& line, int offset) {
        string key, value;
        string& current = key;
        for (unsigned int i=offset; i<line.size(); i++) {
            auto c = line[i];
            if (c == '\'') continue;
            if (c == ' ' || c == '\t') {
                current = value;
                continue;
            }
            current += c;
        }
        model.parameters[key] = value;
    };

    auto parseBlockData = [&](const string& line) {
        int column = 0;
        bool quoted1 = false;
        bool quoted2 = false;
        string value;

        for (unsigned int i=0; i<line.size(); i++) {
            auto c = line[i];
            if (c == '\'') quoted1 = !quoted1;
            if (c == '"') quoted2 = !quoted2;
            if (!quoted1 && !quoted2 && value != "") {
                if (c == ' ' || c == '\t' || i+1 == line.size()) {
                    currentBlock[blockHead[column]].push_back(value);
                    value = "";
                    continue;
                }
            }
            value += c;
        }
    };

    bool loopingHead = false;
    bool loopingBlock = false;

    for ( auto line : splitString(data,'\n') ) {
        if (line[0] == '#') continue;
        if (line == "") {
            if (loopingBlock) {
                loopingBlock = false;
                model.blocks.push_back(currentBlock);
                blockHead.clear();
                currentBlock.clear();
            }
            continue;
        }

        if (loopingHead) {
            if (line[0] == '_') blockHead.push_back(subString(line,1,line.size()-1));
            else { loopingHead = false; loopingBlock = true; }
        }

        if (loopingBlock) parseBlockData(line);
        if (line[0] == '_') parseParameter(line, 1);
        if (subString(line, 0, 5) == "data_") { }
        if (subString(line, 0, 5) == "save_") { }
        if (subString(line, 0, 5) == "loop_") loopingHead = true;
    }

    return model;
}

void VRCrystal::loadCell(string path) { // CIF data parser!
    auto cellData = parseCIFfile(path);
    if (cellData.blocks.size() > 1) {
        auto& positionData = cellData.blocks[1];
        vector<Vec3d> positions;
        if (positionData.count("symmetry_equiv_pos_as_xyz")) {
            for (string data : positionData["symmetry_equiv_pos_as_xyz"]) {
                Vec3d pos;
                auto posData = splitString( subString(data, 1, data.size()-1), ',');
                for (int i=0; i<3; i++) {
                    ;
                    pos[0] = toFloat( posData[0] );
                }
                positions.push_back(pos);
            }
        }
    }
}

void VRCrystal::setSize(Vec3i s) {
    ;
    updateGeo();
}
