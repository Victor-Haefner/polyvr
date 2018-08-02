#ifndef SYSTEM
#define SYSTEM

#include <string>
#include <vector>

using namespace std;

class FILESYSTEM {
    public:
        static bool isFile(const string& path);
        static bool isDir(const string& path);
        static bool exist(const string& path);

        static string baseName(const string& path);
        static string fileName(const string& path);
        static string dirName(const string& path);
        static string suffix(const string& path);

        static bool createDir(const string& dir);
        static bool remove(const string& path);

        static string normalize(const string& path);

        static vector<string> getFiles(const string& dir);
};

#endif // SYSTEM
