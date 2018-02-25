#ifndef TOSTRINGBIS_H_INCLUDED
#define TOSTRINGBIS_H_INCLUDED

template<typename T> int toValue(string s, std::shared_ptr<T>& t) {
    cout << "Warning in toValue<shared_ptr<T>> (toString.h): ignore data '" << s << "'" << endl;
    t = 0;
    return true;
}

#endif // TOSTRINGBIS_H_INCLUDED
