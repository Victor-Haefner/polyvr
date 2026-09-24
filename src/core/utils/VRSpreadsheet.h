#ifndef VRSPREADSHEET_H_INCLUDED
#define VRSPREADSHEET_H_INCLUDED

#include <map>
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include "VRUtilsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class Table : public std::enable_shared_from_this<Table> {
    private:
        vector<string> colOrder;
        map<string, vector<string>> data;
        size_t rowsN = 0;

    public:
		Table();
		~Table();

		static TablePtr create();
		TablePtr ptr();

		vector<string> getCol(string name);
		vector<string> getRow(size_t row);
		string get(string name, size_t row);
		vector<string> getColumns();
		vector< vector<string> > getRows();
		size_t getNCols();
		size_t getNRows();
		size_t getColSize(string name);
		void addCol(string name);
		void add(string name, string val);
		void set(string name, size_t row, string val);
};

class VRSpreadsheet : public std::enable_shared_from_this<VRSpreadsheet> {
	public:
		struct Cell {
		    size_t col = 0;
		    size_t row = 0;

			string ID;
			string type;
			string data;

			Cell(size_t c, size_t r) : col(c), row(r) {}
		};

		struct Row {
			string ID;
			vector<Cell> cells;
		};

		struct Sheet {
			string name;
			XMLPtr xml;

			size_t NCols = 0;
			size_t NRows = 0;

			vector<Row> rows;

			Sheet();
			Sheet(string name);
		};

	private:
		map<string, Sheet> sheets;

		void addSheet(string& name, string& data, vector<string>& strings);
		map<string, string> parseWorkbook(string& data, string& rels);
		vector<string> parseStrings(string& data);

		Vec2i convCoords(string c);

	public:
		VRSpreadsheet();
		~VRSpreadsheet();

		static VRSpreadsheetPtr create();
		VRSpreadsheetPtr ptr();

		TablePtr asTable(string sheet);

		void read(string path);
		void write(string folder, string ext);
		void writeSheet(string sheet, string path);

		vector<string> getSheets();
		size_t getNColumns(string sheet);
		size_t getNRows(string sheet);
		vector<string> getRow(string sheet, size_t i);
		vector< vector<string> > getRows(string sheet);
		string getCell(string sheet, size_t i, size_t j);

		void setCell(string sheet, size_t i, size_t j, string c);
};

OSG_END_NAMESPACE;

#endif //VRSPREADSHEET_H_INCLUDED
