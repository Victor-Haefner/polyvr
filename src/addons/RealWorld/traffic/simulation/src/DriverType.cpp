#include "DriverType.h"

#include <boost/lexical_cast.hpp>
using boost::lexical_cast;

DriverType* DriverType::createDriverType(const ID id, const double probability, const double lawlessness, const double cautiousness) {

    DriverType *driver = new DriverType(id);

    // Try to set all parameter. They check the values internally, so just chain them together
    if (driver->setProbability(probability) && driver->setLawlessness(lawlessness) && driver->setCautiousness(cautiousness))
        return driver;

    // At least one parameter is invalid, return a null pointer
    delete driver;
    return NULL;
}

DriverType::DriverType(const ID id)
    // Create a boring and lawful driver as the default
    : id(id), probability(1), lawlessness(0), cautiousness(1) {
}

ID DriverType::getId() const {
    return id;
}

bool DriverType::setProbability(const double prob) {
    if (prob < 0)
        return false;
    probability = prob;
    return true;
}

double DriverType::getProbability() const {
    return probability;
}

bool DriverType::setLawlessness(double val) {
    if (val < 0 || val > 1)
        return false;
    lawlessness = val;
    return true;
}

double DriverType::getLawlessness() const {
    return lawlessness;
}

bool DriverType::setCautiousness(double caution) {
    if (caution < 0 || caution > 1)
        return false;
    cautiousness = caution;
    return true;
}

double DriverType::getCautiousness() const {
    return cautiousness;
}

string DriverType::toString(const bool extendedOutput) const {

    string str = string("DriverType #") + lexical_cast<string>(id) + ((extendedOutput)?"\n  ":" [")
        + "probability=" + lexical_cast<string>(probability) + ((extendedOutput)?"\n  ":"; ")
        + "lawlessness=" + lexical_cast<string>(lawlessness) + ((extendedOutput)?"\n  ":"; ")
        + "cautiousness=" + lexical_cast<string>(cautiousness) + ((extendedOutput)?"":"]");

    return str;
}
