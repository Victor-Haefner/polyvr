#ifndef OPTIONS_H_INCLUDED
#define OPTIONS_H_INCLUDED

#include <iostream>
#include <map>
#include <string>
#include "core/utils/toString.h"

using namespace std;

class VROptions {
    private:
        map<string, string> options;
        map<string, string> descriptions;

        VROptions();
        void operator= (VROptions v);

        template <typename T>
        void addOption(T value, string name, string description = "") {
            cout << "  add option: " << name << " - " << description << ", default is '" << value << "'" << endl;
            options[name] = toString(value);
            descriptions[name] = description;
        }

    public:
        ~VROptions();

        int argc;
        char** argv;

        static VROptions* get();

        bool hasOption(string name);
        void printHelp();

        template <typename T>
        T getOption(string name) {
            if (options.count(name) == 0) {
                cout << "\nERROR, " << name << " is not an option!" << endl;
                return T();
            }

            return toValue<T>( options[name] );
        }

        template <typename T>
        void setOption(string name, const T val) {
            options[name] = toString(val);
        }

        void parse(int _argc, char** _argv);
};



#endif // OPTIONS_H_INCLUDED
