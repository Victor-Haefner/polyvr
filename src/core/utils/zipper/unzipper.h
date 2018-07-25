#pragma once

#include <vector>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <memory>
#include <map>

using namespace std;

class ZipEntry;

class Unzipper {
  public:
    Unzipper(istream& buffer);
    Unzipper(vector<unsigned char>& buffer);
    Unzipper(const string& zipname);
    Unzipper(const string& zipname, const string& password);

    ~Unzipper(void);

    vector<ZipEntry> entries();

    bool extract(const string& destination, const map<string, string>& alternativeNames);
    bool extract(const string& destination=string());
    bool extractEntry(const string& name, const string& destination = string());
    bool extractEntryToStream(const string& name, ostream& stream);
    bool extractEntryToMemory(const string& name, vector<unsigned char>& vec);

    void close();

  private:
    string password;
    string zipname;
    istream& m_ibuffer;
    vector<unsigned char>& m_vecbuffer;
    bool m_usingMemoryVector;
    bool m_usingStream;
    bool m_open;

    struct Impl;
    Impl* m_impl;
};


class ZipEntry {
  private:
    typedef struct
    {
      unsigned int tm_sec;
      unsigned int tm_min;
      unsigned int tm_hour;
      unsigned int tm_mday;
      unsigned int tm_mon;
      unsigned int tm_year;
    } tm_s;

  public:
    ZipEntry(const string& name, unsigned long long int compressed_size, unsigned long long int uncompressed_size,
      int year, int month, int day, int hour, int minute, int second, unsigned long dosdate)
      : name(name), compressedSize(compressed_size), uncompressedSize(uncompressed_size), dosdate(dosdate)
    {
      // timestamp YYYY-MM-DD HH:MM:SS
      stringstream str;
      str << year << "-" << month << "-" << day <<
        " " << hour << ":" << minute << ":" << second;
      timestamp = str.str();

      unixdate.tm_year = year;
      unixdate.tm_mon = month;
      unixdate.tm_mday = day;
      unixdate.tm_hour = hour;
      unixdate.tm_min = minute;
      unixdate.tm_sec = second;
    }

    bool valid() { return !name.empty(); }

    string name, timestamp;
    unsigned long long int compressedSize, uncompressedSize;
    unsigned long dosdate;
    tm_s unixdate;
};

