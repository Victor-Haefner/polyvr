#ifndef WORDWRAP_H_INCLUDED
#define WORDWRAP_H_INCLUDED

#include <string>

using namespace std;

class WordWrap {
    private:
        int lastLineLength = 0;

    public:
        void reset();
        void wrap(string& s, int maxN);
        void test();
};

#endif // WORDWRAP_H_INCLUDED
