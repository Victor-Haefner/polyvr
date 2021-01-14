#ifndef VRSPREADSHEET_H_INCLUDED
#define VRSPREADSHEET_H_INCLUDED

#include <map>
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include "VRUtilsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSpreadsheet : public std::enable_shared_from_this<VRSpreadsheet> {
	public:
		struct Cell {
			string ID;
			string type;
			string data;
		};

		struct Row {
			string ID;
			vector<Cell> cells;
		};

		struct Sheet {
			string name;
			XMLPtr xml;

			int NCols = 0;
			int NRows = 0;

			vector<Row> rows;

			Sheet();
			Sheet(string name);
		};

	private:
		map<string, Sheet> sheets;

		void addSheet(string& name, string& data, vector<string>& strings);
		map<string, string> parseWorkbook(string& data);
		vector<string> parseStrings(string& data);

		Vec2i convCoords(string c);

	public:
		VRSpreadsheet();
		~VRSpreadsheet();

		static VRSpreadsheetPtr create();
		VRSpreadsheetPtr ptr();

		void read(string path);

		vector<string> getSheets();
		size_t getNColumns(string sheet);
		size_t getNRows(string sheet);
		string getCell(string sheet, size_t i, size_t j);
};

OSG_END_NAMESPACE;

#endif //VRSPREADSHEET_H_INCLUDED
