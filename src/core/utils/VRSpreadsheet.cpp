#include "VRSpreadsheet.h"
#include "toString.h"

#include <iostream>
#include <OpenSG/OSGVector.h>

#ifndef WITHOUT_MDB
#include <mdbtools.h>
#endif

#include "xml.h"
#include "system/VRSystem.h"
#include "zipper/unzipper.h"

using namespace OSG;


Table::Table() {}
Table::~Table() {}

TablePtr Table::create() { return TablePtr( new Table() ); }
TablePtr Table::ptr() { return static_pointer_cast<Table>(shared_from_this()); }

vector<string> Table::getCol(string name) {
    if (!data.count(name)) return {};
    vector<string>& c = data[name];
    size_t N = c.size();
    for (size_t i = N; i<rowsN; i++) c.push_back("");
    return c;
}

vector<string> Table::getRow(size_t row) {
    if (row >= rowsN) return {};
    vector<string> r;
    for ( auto& n : colOrder ) {
        vector<string>& c = data[n];
        if (row < c.size()) r.push_back( c[row] );
        else r.push_back("");
    };
    return r;
}

vector< vector<string> > Table::getRows() {
    vector< vector<string> > rows;
    for (size_t i=0; i<rowsN; i++) rows.push_back( getRow(i) );
    return rows;
}

string Table::get(string name, size_t row) {
    if (!data.count(name)) return "";
    vector<string>& c = data[name];
    if (row < c.size()) return data[name][row];
    return "";
}

vector<string> Table::getColumns() {
    return colOrder;
}

size_t Table::getNCols() { return colOrder.size(); }
size_t Table::getNRows() { return rowsN; }


void Table::addCol(string name) {
    colOrder.push_back(name);
    data[name] = {};
}

void Table::add(string name, string val) {
    if (!data.count(name)) return;
    vector<string>& c = data[name];
    c.push_back(val);
    rowsN = max(rowsN, c.size());
}

void Table::set(string name, size_t row, string val) {
    if (!data.count(name)) return;
    data[name][row] = val;
}

size_t Table::getColSize(string name) {
    if (!data.count(name)) return 0;
    return data[name].size();
}


VRSpreadsheet::VRSpreadsheet() {}
VRSpreadsheet::~VRSpreadsheet() {}

VRSpreadsheetPtr VRSpreadsheet::create() { return VRSpreadsheetPtr( new VRSpreadsheet() ); }
VRSpreadsheetPtr VRSpreadsheet::ptr() { return static_pointer_cast<VRSpreadsheet>(shared_from_this()); }

TablePtr VRSpreadsheet::asTable(string sheet) {
    auto t = Table::create();
    if (!sheets.count(sheet)) { cout << "Unknown sheet \"" << sheet << "\"" << endl; return t; }

    Sheet& s = sheets[sheet];
    if (s.NRows == 0 || s.NCols == 0) return t;

    Row& cols = s.rows[0]; // expect column headers in first row

    map<string, int> colIDs;
    for (Cell& c : cols.cells) {
        t->addCol(c.data);
        colIDs[c.data] = c.col;
    }

    for (size_t i = 1; i<s.NRows; i++) {
        Row& row = s.rows[i];
        for (auto& ci : colIDs) {
            Cell& c = row.cells[ci.second];
            while (t->getColSize(ci.first)+1 < c.row) t->add(ci.first, ""); // fill column if necessary
            t->add(ci.first, c.data);
        }
    }

    return t;
}

void VRSpreadsheet::read(string path) {
    if (!exists(path)) {
        cout << "VRSpreadsheet::read failed, path not found: '" << path << "'" << endl;
        return;
    }

    //cout << "VRSpreadsheet::read: " << path << endl;
    string ext = getFileExtension(path);

    auto extractZipEntry = [](Unzipper& z, string name) {
        vector<unsigned char> vec;
        z.extractEntryToMemory(name, vec);
        return string(vec.begin(), vec.end());
    };

    if (ext == ".xlsx") {
        try {
            Unzipper z(path, ".zip");

            auto entries = z.entries();

            sheets.clear();

            string workbookData1 = extractZipEntry(z, "xl/workbook.xml");
            string workbookData2 = extractZipEntry(z, "xl/_rels/workbook.xml.rels");
            auto sheets = parseWorkbook(workbookData1, workbookData2);

            string stringsData = extractZipEntry(z, "xl/sharedStrings.xml");
            auto strings = parseStrings(stringsData);

            for (auto s : sheets) {
                string path = "xl/" + s.first;
                string data = extractZipEntry(z, path);
                addSheet(s.second, data, strings);
            }
        } catch(exception& e) { cout << "VRSpreadsheet::read unzipper error: " << e.what() << endl; return; }

        return;
    }

#ifndef WITHOUT_MDB
    if (ext == ".mdb" || ext == ".accdb" || ext == ".eap") { // MS Access DB
        auto mdb = mdb_open(path.c_str(), MDB_NOFLAGS);
        if (!mdb) { cout << "ERROR: failed to open access DB file: " << path << endl; return; }
        auto r = mdb_read_catalog (mdb, MDB_ANY);
        if (!r) { cout << "ERROR: failed to read access DB catalog: " << path << endl; return; }

        vector<string> tables;
        for (int i=0; i < mdb->num_catalog; i++) {
            MdbCatalogEntry* entry = (MdbCatalogEntry*)g_ptr_array_index (mdb->catalog, i);
            if (entry->object_type != MDB_TABLE) continue;
            if (mdb_is_system_table(entry)) continue;
            string name = entry->object_name;
            tables.push_back(name);
        }

        for (auto name : tables) {
            auto& sheet = sheets[name];
            sheet.name = name;

            MdbTableDef* table = mdb_read_table_by_name(mdb, (char*)name.c_str(), MDB_TABLE);
            if (table) {
                mdb_read_columns(table);
                mdb_rewind_table(table);

                char** bound_values = (char**)g_malloc(table->num_cols * sizeof(char *));
                int* bound_lens = (int*)g_malloc(table->num_cols * sizeof(int));
                for (int i=0;i<table->num_cols;i++) { // bind columns
                    bound_values[i] = (char*)g_malloc0(MDB_BIND_SIZE);
                    mdb_bind_column(table, i+1, bound_values[i], &bound_lens[i]);
                }

                // header
                Row row;
                for (size_t i=0; i<table->num_cols; i++) {
                    MdbColumn* col = (MdbColumn*)g_ptr_array_index(table->columns,i);
                    Cell C(i, 0);
                    C.ID   = "";
                    C.type = "s";
                    C.data = col->name;
                    row.cells.push_back(C);
                }
                sheet.rows.push_back(row);
                sheet.NRows++;
                sheet.NCols = max(sheet.NCols, row.cells.size());

                // data
                while(mdb_fetch_row(table)) {
                    Row row;
                    for (int i=0;i<table->num_cols;i++) {
                        Cell C(i, sheet.NRows);
                        C.ID   = "";
                        C.type = "s";

                        MdbColumn* col = (MdbColumn*)g_ptr_array_index(table->columns,i);
                        if (bound_lens[i]) {
                            char* value;
                            size_t length;
                            if (col->col_type == MDB_OLE) {
                                value = (char*)mdb_ole_read_full(mdb, col, &length);
                            } else {
                                value = bound_values[i];
                                length = bound_lens[i];
                            }

                            C.data = value;
                            if (col->col_type == MDB_OLE) free(value);
                        }

                        row.cells.push_back(C);
                    }
                    sheet.rows.push_back(row);
                    sheet.NRows++;
                    sheet.NCols = max(sheet.NCols, row.cells.size());
                }

                for (int i=0;i<table->num_cols;i++) g_free(bound_values[i]);
                g_free(bound_values);
                g_free(bound_lens);
                mdb_free_tabledef(table);
            }
        }


        mdb_close(mdb);

        /**
        TODO: continue development
        look at: https://github.com/mdbtools/mdbtools/tree/dev/src/util
        and: https://leanpub.com/InsideEA/read
        */

        return;
    }
#endif

    cout << "VRSpreadsheet::read Error: unknown extention " << ext << endl;
}

void VRSpreadsheet::write(string folder, string ext) {
    //cout << "VRSpreadsheet::write to " << folder << " with extention " << ext << endl;
    if (ext == "csv") {
        makedir(folder);
        for (auto s : sheets) writeSheet(s.first, folder+"/"+s.first+"."+ext);
    }
}

void VRSpreadsheet::writeSheet(string sheetName, string path) {
    //cout << "VRSpreadsheet::writeSheet " << sheetName << " to " << path << endl;
    string ext = getFileExtension(path);

    auto& sheet = sheets[sheetName];

    if (ext == ".csv") {
        ofstream out(path);
        bool firstRow = true;
        for (auto& row : sheet.rows) {
            if (!firstRow) out << endl;
            bool firstCell = true;
            for (auto& cell : row.cells) {
                if (!firstCell) out << ",";
                out << cell.data;
                firstCell = false;
            }
            firstRow = false;
        }
    }
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

map<string, string> VRSpreadsheet::parseWorkbook(string& data, string& relsData) {
    XML xml;
    xml.parse(data);
    XML rels;
    rels.parse(relsData);

    map<string, string> paths;
    auto relations = rels.getRoot();
    for (auto rel : relations->getChildren()) {
        string rID = rel->getAttribute("Id");
        paths[rID] = rel->getAttribute("Target");
    }

    map<string, string> res;
    auto sheets = xml.getRoot()->getChild("sheets");
    for (auto sheet : sheets->getChildren()) {
        string state = sheet->getAttribute("state");
        if (state == "hidden") continue;

        string name = sheet->getAttribute("name");
        string sheetId = sheet->getAttribute("id");
        string sheetPath = paths[sheetId];
        res[sheetPath] = name;
    }

    return res;
}

void VRSpreadsheet::addSheet(string& name, string& data, vector<string>& strings) {
    auto& sheet = sheets[name];
    sheet.name = name;
    sheet.xml = XML::create();
    sheet.xml->parse(data);
    auto root = sheet.xml->getRoot();

    sheet.NCols = 0;
    sheet.NRows = 0;

    auto sdata = root->getChild("sheetData");
    if (!sdata) {
        cout << "Warning, sheet " << name << " has no root, skip!" << endl;
        return;
    }

    for (auto row : sdata->getChildren()) {
        Row R;
        size_t ci = 0;
        for (auto col : row->getChildren()) {
            Cell C(ci, sheet.NRows);
            C.ID   = col->getAttribute("r");
            C.type = col->getAttribute("t");
            auto V = col->getChild("v");
            C.data = V ? V->getText() : "";
            if (C.type == "s") C.data = strings[toInt(C.data)];
            R.cells.push_back(C);
            ci++;
        }
        sheet.rows.push_back(R);
        sheet.NRows++;
        sheet.NCols = max(sheet.NCols, R.cells.size());
    }

    //cout << " VRSpreadsheet::addSheet " << name << " " << Vec2i(sheet.NRows, sheet.NCols) << endl;
}


vector<string> VRSpreadsheet::getSheets() {
    vector<string> res;
    for (auto s : sheets) res.push_back(s.first);
    return res;
}

size_t VRSpreadsheet::getNColumns(string sheet) {
    if (!sheets.count(sheet)) return 0;
    //cout << "VRSpreadsheet::getNColumns of " << sheet << endl;
    //cout << sheets[sheet].xml->getRoot()->getChild("sheetData")->toString() << endl;
    return sheets[sheet].NCols;
}

size_t VRSpreadsheet::getNRows(string sheet) {
    if (!sheets.count(sheet)) return 0;
    return sheets[sheet].NRows;
}

vector<string> VRSpreadsheet::getRow(string sheet, size_t i) {
    vector<string> res;
    if (!sheets.count(sheet)) return res;
    if (i >= sheets[sheet].rows.size()) return res;

    auto& row = sheets[sheet].rows[i];
    for (auto& cell : row.cells) {
        res.push_back(cell.data);
    }
    return res;
}

vector< vector<string> > VRSpreadsheet::getRows(string sheet) {
    vector< vector<string> > res;
    if (!sheets.count(sheet)) return res;

    for (auto& row : sheets[sheet].rows) {
        vector<string> data;
        for (auto& cell : row.cells) {
            data.push_back(cell.data);
        }
        res.push_back(data);
    }
    return res;
}

string VRSpreadsheet::getCell(string sheet, size_t i, size_t j) {
    if (!sheets.count(sheet)) return "";
    if (j >= sheets[sheet].NRows) return "";
    if (i >= sheets[sheet].NCols) return "";
    if (j >= sheets[sheet].rows.size()) return "";
    if (i >= sheets[sheet].rows[j].cells.size()) return "";
    return sheets[sheet].rows[j].cells[i].data;
}

void VRSpreadsheet::setCell(string sheet, size_t i, size_t j, string c) {
    if (!sheets.count(sheet)) return;
    while (j >= sheets[sheet].rows.size()) {
        sheets[sheet].rows.push_back(Row());
    }
    while (i >= sheets[sheet].rows[j].cells.size()) {
        sheets[sheet].rows[j].cells.push_back(Cell(i,j));
    }
    sheets[sheet].rows[j].cells[i].data = c;
}

VRSpreadsheet::Sheet::Sheet() {}
VRSpreadsheet::Sheet::Sheet(string n) : name(n) {}
