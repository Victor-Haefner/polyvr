#include "filesystem.h"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

bool FILESYSTEM::isFile(const string& path) {
    return fs::is_regular_file(path);
}

bool FILESYSTEM::isDir(const string& path) {
    return fs::is_directory(path);
}

bool FILESYSTEM::exist(const string& path) {
    return fs::exists(path);
}

string FILESYSTEM::baseName(const string& path) {
    fs::path p(path);
    return p.stem().string();
}

string FILESYSTEM::fileName(const string& path) {
    fs::path p(path);
    return p.filename().string();
}

string FILESYSTEM::dirName(const string& path) {
    fs::path p(path);
    return p.parent_path().string();
}

string FILESYSTEM::suffix(const string& path) {
    return fs::extension(path);
}

bool FILESYSTEM::createDir(const string & dir) {
    fs::path p(dir);
    return fs::create_directories(p);
}

bool FILESYSTEM::remove(const string & path) {
    return fs::remove_all(path);
}

string FILESYSTEM::normalize(const string & path) {
    if (!exist(path)) return path;
    return fs::canonical(path).string();
}

vector<string> FILESYSTEM::getFiles(const string& dir) {
    vector<string> files;
    fs::path path(dir);
    fs::recursive_directory_iterator end;

    for (fs::recursive_directory_iterator i(path); i != end; ++i) {
        const fs::path cp = (*i);
        files.push_back(cp.string());
    }
    return files;
}







