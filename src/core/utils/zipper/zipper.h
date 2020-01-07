#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <memory>

using namespace std;

class Zipper {
	private:
		iostream& m_obuffer;
		vector<unsigned char>& m_vecbuffer;
		bool m_usingMemoryVector;
		bool m_usingStream;
		string zipname;
		bool m_open;
		string password;

		struct Impl;
		Impl* m_impl;

	public:
		enum zipFlags { Overwrite = 0x01, Append = 0x02, Store = 0x04, Faster = 0x08, Better = 0x10, NoPaths = 0x20 };

		Zipper(iostream& buffer);
		Zipper(vector<unsigned char>& buffer);
		Zipper(const string& zipname);
		Zipper(const string& zipname, const string& password);

		~Zipper(void);

		bool add(istream& source, const string& nameInZip = string(), zipFlags flags = Better);
		bool add(const string& fileOrFolderPath, zipFlags flags = Better);

		void open();
		void close();
};

