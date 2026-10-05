#include "WordWrap.h"

#include <iostream>

void WordWrap::reset() { lastLineLength = 0; }

void WordWrap::wrap(string& m, int maxN) {
    size_t pointer = 0;

    while (pointer < m.size()) {
        int availN = max(0, maxN - lastLineLength); // chars left on line before wrap
        size_t leftN = max((size_t)0, m.size() - pointer); // chars left to process

        size_t nextBreak = m.rfind('\n', pointer + availN);
        if (nextBreak != string::npos && nextBreak >= pointer) {
            lastLineLength = 0;
            pointer = nextBreak+1;
            continue;
        }

        if (leftN <= availN) { lastLineLength += leftN; break; } // everything fits, done

        size_t nextSpace = m.rfind(' ',  pointer + availN);
        if (nextSpace == string::npos || nextSpace < pointer) { // no valid space found for wrapping
            size_t wordEnd = m.find_first_of(" \n", pointer);

            if (wordEnd == string::npos) { lastLineLength += leftN; break; }

            if (m[wordEnd] == '\n') { // found newline
                pointer = wordEnd + 1;
                lastLineLength = 0;
                continue;
            }

            nextSpace = wordEnd; // wrap at next space
        }

        // normal wrap
        m[nextSpace] = '\n';
        lastLineLength = 0;
        pointer = nextSpace+1;
        while (pointer < m.size() && m[pointer] == ' ') pointer++;
    }
}

void WordWrap::test() {
    string m = "abc 123 456 fgh ert jlk\n123 123\n\n\n345 456 567 sdf\n asd\nwer \n \n asd \n   asd dd e";
    cout << m << endl;
    wrap(m, 9);
    cout << m << endl;
}






