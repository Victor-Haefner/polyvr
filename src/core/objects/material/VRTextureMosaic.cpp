#include "VRTextureMosaic.h"

#include <OpenSG/OSGImage.h>

using namespace OSG;

void VRTextureMosaic::add(VRTexturePtr tex, Vec2i pos, Vec2i ID) {
    Entry entry;
    entry.tex = tex;
    entry.pos = pos;
    entry.size = Vec2i( tex->getSize()[0], tex->getSize()[1] );
    entries[ID] = entry;

    auto s = getSize();
    if (s[0] == 0) {
        ImageMTRecPtr img = Image::create();
        img->set( tex->getImage() );
        setImage( img );
        return;
    }

    resize( Vec3i(max(pos[0]+tex->getSize()[0], s[0]), max(pos[1]+tex->getSize()[1], s[1]), 1), Vec3i() );
    paste(tex, Vec3i(pos[0], pos[1], 0) );
}

VRTextureMosaic::VRTextureMosaic() {}
VRTextureMosaic::~VRTextureMosaic() {}

VRTextureMosaicPtr VRTextureMosaic::create() { return shared_ptr<VRTextureMosaic>(new VRTextureMosaic() ); }
VRTextureMosaicPtr VRTextureMosaic::ptr() { return static_pointer_cast<VRTextureMosaic>( shared_from_this() ); }

Vec2i VRTextureMosaic::getChunkPosition(Vec2i ID) { return entries[ID].pos; }
Vec2i VRTextureMosaic::getChunkSize(Vec2i ID) { return entries[ID].size; }
