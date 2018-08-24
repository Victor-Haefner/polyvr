#ifndef TOSTRINGBIS_H_INCLUDED
#define TOSTRINGBIS_H_INCLUDED

template<typename T> int toValue(string s, std::shared_ptr<T>& t) {
    if (s != "0") cout << "Warning in toValue<shared_ptr<T>> (toStringBis.h): ignore data '" << s << "'" << endl;
    t = 0;
    return true;
}

#endif // TOSTRINGBIS_H_INCLUDED
