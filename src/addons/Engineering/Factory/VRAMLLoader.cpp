#include "VRAMLLoader.h"
#include "core/utils/toString.h"

using namespace OSG;

template<> string typeName(const VRAMLLoader& t) { return "VRAMLLoader"; }

VRAMLLoader::VRAMLLoader() {}
VRAMLLoader::~VRAMLLoader() {}

VRAMLLoaderPtr VRAMLLoader::create()  { return VRAMLLoaderPtr(new VRAMLLoader()); }

void VRAMLLoader::read(string path) {
    ;
}

void VRAMLLoader::write(string path) {
    ;
}


