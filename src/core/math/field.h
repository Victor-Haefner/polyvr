#ifndef FIELD_H_INCLUDED
#define FIELD_H_INCLUDED

#include <vector>

// 2D vector
template<class T> struct field {
    int width = 0;
    int height = 0;
    std::vector<T> data;
};

#endif // FIELD_H_INCLUDED
