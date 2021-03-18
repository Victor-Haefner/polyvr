#include "VRSpreadsheet.h"
#include "toString.h"

#include <iostream>
#include <OpenSG/OSGVector.h>

#include "xml.h"
#include "system/VRSystem.h"
#include "zipper/unzipper.h"

using namespace OSG;

VRSpreadsheet::VRSpreadsheet() {}
VRSpreadsheet::~VRSpreadsheet() {}

VRSpreadsheetPtr VRSpreadsheet::create() { return VRSpreadsheetPtr( new VRSpreadsheet() ); }
VRSpreadsheetPtr VRSpreadsheet::ptr() { return static_pointer_cast<VRSpreadsheet>(shared_from_this()); }

void VRSpreadsheet::read(string path) {
    cout << "VRSpreadsheet::read: " << path << endl;
    string ext = getFileExtension(path);

    auto extractZipEntry = [](Unzipper& z, string name) {
        vector<unsigned char> vec;
        z.extractEntryToMemory(name, vec);
        return string(vec.begin(), vec.end());
    };

    if (ext == ".xlsx") {
        Unzipper z(path);
        auto entries = z.entries();

        sheets.clear();

        string workbookData = extractZipEntry(z, "xl/workbook.xml");
        auto sheets = parseWorkbook(workbookData);

        string stringsData = extractZipEntry(z, "xl/sharedStrings.xml");
        auto strings = parseStrings(stringsData);

        for (auto s : sheets) {
            string path = "xl/worksheets/sheet" + toString(s.first) + ".xml";
            string data = extractZipEntry(z, path);
            addSheet(s.second, data, strings);
        }

        return;
    }

    cout << "VRSpreadsheet::read error: unknown extention " << ext << endl;
}

Vec2i VRSpreadsheet::convCoords(string d) {
    string c, r;
    for (char v : d) {
        if (v >= '0' && v <= '9') r += v;
        else c += v;
    }

    int R = toInt(r)-1; // because they start at 1

    vector<int> cv;
    for (char v : c) cv.push_back(int(v-'A'));

    int C = 0;
    int P = 1;
    for (int i = cv.size() - 1; i >= 0; i--) {
        C += P * cv[i];
        P *= 26;
    }

    return Vec2i(C, R);
}

vector<string> VRSpreadsheet::parseStrings(string& data) {
    XML xml;
    xml.parse(data);

    vector<string> res;
    for (auto si : xml.getRoot()->getChildren()) {
        string s = si->getChild(0)->getText();
        res.push_back(s);
    }

    return res;
}

map<string, string> VRSpreadsheet::parseWorkbook(string& data) {
    XML xml;
    xml.parse(data);

    map<string, string> res;

    auto sheets = xml.getRoot()->getChild("sheets");
    for (auto sheet : sheets->getChildren()) {
        string state = sheet->getAttribute("state");
        if (state == "hidden") continue;

        string name = sheet->getAttribute("name");
        string sheetId = sheet->getAttribute("sheetId");
        res[sheetId] = name;
    }

    return res;
}

void VRSpreadsheet::addSheet(string& name, string& data, vector<string>& strings) {
    auto& sheet = sheets[name];
    sheet.name = name;
    sheet.xml = XML::create();
    sheet.xml->parse(data);
    auto root = sheet.xml->getRoot();

    auto dims = root->getChild("dimension");
    auto dv = dims->getAttribute("ref");
    Vec2i C0 = convCoords( splitString(dv,':')[0] );
    Vec2i C1 = convCoords(splitString(dv,':')[1] );
    sheet.NCols = C1[0] - C0[0] + 1;
    sheet.NRows = C1[1] - C0[1] + 1;

    auto sdata = root->getChild("sheetData");
    for (auto row : sdata->getChildren()) {
        Row R;
        for (auto col : row->getChildren()) {
            Cell C;
            C.ID   = col->getAttribute("r");
            C.type = col->getAttribute("t");
            auto V = col->getChild("v");
            C.data = V ? V->getText() : "";
            if (C.type == "s") C.data = strings[toInt(C.data)];
            R.cells.push_back(C);
        }
        sheet.rows.push_back(R);
    }

    cout << " VRSpreadsheet::addSheet " << name << " " << Vec2i(sheet.rows[0].cells.size(), sheet.rows.size()) << " " << Vec2i(sheet.NCols, sheet.NRows) << endl;

    // TODO: use dimensions and cell access properly (maps instead of vectors)
    sheet.NCols = sheet.rows[0].cells.size();
    sheet.NRows = sheet.rows.size();


    /*for (auto r : sheet.rows) {
        for (auto c : r.cells) {
            cout << "\t " << c.data;
        }
        cout << endl;
    }*/
}

vector<string> VRSpreadsheet::getSheets() {
    vector<string> res;
    for (auto s : sheets) res.push_back(s.first);
    return res;
}

size_t VRSpreadsheet::getNColumns(string sheet) {
    //cout << "VRSpreadsheet::getNColumns of " << sheet << endl;
    //cout << sheets[sheet].xml->getRoot()->getChild("sheetData")->toString() << endl;
    return sheets[sheet].NCols;
}

size_t VRSpreadsheet::getNRows(string sheet) {
    return sheets[sheet].NRows;
}

string VRSpreadsheet::getCell(string sheet, size_t i, size_t j) {
    return sheets[sheet].rows[j].cells[i].data;
}

VRSpreadsheet::Sheet::Sheet() {}
VRSpreadsheet::Sheet::Sheet(string n) : name(n) {}
