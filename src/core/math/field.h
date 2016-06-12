#ifndef FIELD_H_INCLUDED
#define FIELD_H_INCLUDED

#include <vector>

// 2D vector
template<class T> struct field {
    int width = 0;
    int height = 0;
    std::vector<T> data;

    T get(int i, int j) const {
        return data[i+j*width];
    }
};

#endif // FIELD_H_INCLUDED
