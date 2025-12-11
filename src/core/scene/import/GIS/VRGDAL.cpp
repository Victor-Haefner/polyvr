#include "VRGDAL.h"

/*#define SIZEOF_UNSIGNED_LONG 4
#define SIZEOF_VOIDP 8
#define HAVE_LONG_LONG 1*/

#include <math.h>
#include <iostream>
#include <map>
#if defined(WIN32) || defined(__APPLE__)
#include <gdal.h>
#include <gdal_priv.h>
#include <gdal_version.h>
#include <ogrsf_frmts.h>
#else
#include <gdal/gdal.h>
#include <gdal/gdal_priv.h>
#include <gdal/gdal_version.h>
#include <gdal/ogrsf_frmts.h>
#endif
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGImage.h>
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/object/VRObjectT.h"
#include "core/objects/VRTransform.h"
#include "core/math/polygon.h"
#include "core/math/triangulator.h"
#include "core/utils/toString.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSemanticManager.h"
#include "addons/Semantics/Reasoning/VREntity.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include <OpenSG/OSGGeoProperties.h>

// TODO: why on earth do I need that?? should be in toString
template<> string typeName< vector<OSG::Vec3d> >( vector<OSG::Vec3d> const *) { return ""; }
template<> int toValue<vector<OSG::Vec3d > >(stringstream&, vector<OSG::Vec3d >&) { return 0; }
template<> string toString<vector<OSG::Vec3d > >(vector<OSG::Vec3d > const&) { return ""; }

#include <string>

OSG_BEGIN_NAMESPACE;

void loadPDF(string path, VRTransformPtr res, map<string, string> opts) {
    loadTIFF(path, res, opts);
}

auto toVec3d = [](const OGRPoint& p) { return Vec3d( p.getX(), p.getZ(), -p.getY() ); };
auto toOGRPoint = [](const Vec3d& p) { return OGRPoint( p[0], -p[2], p[1] ); };

void loadSHP(string path, VRTransformPtr res, map<string, string> opts) {
    OGRRegisterAll();
#if GDAL_VERSION_MAJOR < 2
 	OGRDataSource *poDS = OGRSFDriverRegistrar::Open(path.c_str(), false);
#else
	GDALDataset *poDS = (GDALDataset*) GDALOpenEx(path.c_str(), GDAL_OF_READONLY, NULL, NULL, NULL);
#endif

    if( poDS == NULL ) { printf( "Open failed.\n" ); return; }

    auto scene = VRScene::getCurrent();
    auto ontology = scene->getSemanticManager()->addOntology(path);
    auto Layer = ontology->addConcept("Layer");
    auto Shape = ontology->addConcept("Shape");
    auto mat = VRMaterial::create("points");

    GeoVec2fPropertyMTRecPtr tcs = GeoVec2fProperty::create();

    double pointSize = opts.count("pointSize") ? toValue<double>(opts["pointSize"]) : -1;
    double lineSize = opts.count("lineSize") ? toValue<double>(opts["lineSize"]) : -1;
    Vec3d offset = opts.count("offset") ? toValue<Vec3d>(opts["offset"]) : Vec3d();

    Layer->addProperty("type", "int");
    Layer->addProperty("features", "Shape");

    vector<Vec3d> geoCoords;

    auto handlePoint = [&](OGRGeometry* geo, VRGeoDataPtr data, size_t fI) {
        OGRPoint* pnt = (OGRPoint*)geo;
        Vec3d p = toVec3d(*pnt) - offset;
        Vec3d n = Vec3d(0,1,0);
        Vec2d tc1 = Vec2d(fI, geoCoords.size());
        geoCoords.push_back(p); // keep original coordinates

        if (pointSize > 0) {
            float W2 = pointSize; //0.00002;
            Vec3d x1 = Vec3d(1,0,0);
            Vec3d x2 = Vec3d(0,0,1);

            Vec3d p11 = p + x1*W2 + x2*W2;
            Vec3d p12 = p + x1*W2 - x2*W2;
            Vec3d p21 = p - x1*W2 + x2*W2;
            Vec3d p22 = p - x1*W2 - x2*W2;

            data->pushVert(p11, n, tc1);
            data->pushVert(p12, n, tc1);
            data->pushVert(p22, n, tc1);
            data->pushVert(p21, n, tc1);
            data->pushQuad();
        } else {
            data->pushVert(p, n, tc1);
            data->pushPoint();
        }
    };

    auto handleLine = [&](OGRGeometry* geo, VRGeoDataPtr data, size_t fI, bool verbose) {
        OGRLineString* line = (OGRLineString*) geo;

        Vec3d p, p1, p2, d, x;
        Vec3d n = Vec3d(0,1,0);

        OGRPoint pnt;
        if (lineSize > 0) {
            float W = lineSize; //0.000004;
            for (int i=1; i<line->getNumPoints(); i++) {
                line->getPoint(i-1, &pnt);
                p1 = toVec3d(pnt) - offset;

                line->getPoint(i, &pnt);
                p2 = toVec3d(pnt) - offset;

                Vec2d tc1 = Vec2d(fI, geoCoords.size());
                Vec2d tc2 = Vec2d(fI, geoCoords.size()+1);
                geoCoords.push_back(p1); // keep original coordinates
                geoCoords.push_back(p2); // keep original coordinates

                d = (p2-p1);
                d.normalize();
                x = d.cross(n);

                Vec3d p11 = p1 + x*W;
                Vec3d p12 = p1 - x*W;
                Vec3d p21 = p2 + x*W;
                Vec3d p22 = p2 - x*W;

                data->pushVert(p11, n, tc1);
                data->pushVert(p21, n, tc2);
                data->pushVert(p22, n, tc2);
                data->pushVert(p12, n, tc1);
                data->pushQuad();
            }
        } else {
            if (verbose) cout << " line " << line->getNumPoints() << endl;
            for (int i=0; i<line->getNumPoints(); i++) {
                line->getPoint(i, &pnt);
                p = toVec3d(pnt) - offset;
                Vec2d tc = Vec2d(fI, geoCoords.size());
                geoCoords.push_back(p); // keep original coordinates
                data->pushVert(p, n, tc);
                if (i > 0) data->pushLine();
                if (verbose) cout << "  line point " << p << endl;
            }
        }
    };

    auto handlePolygon = [&](OGRGeometry* geo, VRGeoDataPtr data, size_t fI) { // TODO: will not work with export
        OGRPolygon* poly = (OGRPolygon*) geo;
        OGRLinearRing* ex = poly->getExteriorRing();
        Vec2d tc = Vec2d(fI, 1);

        // outer bound
        VRPolygon outer;
        OGRPoint pnt;
        for (int i=0; i<ex->getNumPoints(); i++) {
            ex->getPoint(i, &pnt);
            outer.addPoint(toVec3d(pnt) - offset);
        }

        Triangulator t;
        t.add(outer);

        // inner bounds
        for (int i=0; i<poly->getNumInteriorRings(); i++) {
            OGRLinearRing* in = poly->getInteriorRing(i);
            VRPolygon inner;
            for (int i=0; i<in->getNumPoints(); i++) {
                in->getPoint(i, &pnt);
                inner.addPoint(toVec3d(pnt) - offset);
            }
            t.add(inner, false);
        }

        int n = t.append(data, false);
        for (auto i=0; i<n; i++) data->pushTexCoord(tc);
        for (auto i=0; i<n; i++) data->pushTexCoord(tc,1);
    };

    auto handleMultiPolygon = [&](OGRGeometry* geo, VRGeoDataPtr data, size_t fI) {
        cout << "loadSHP::handleGeometry WARNING: it's a multipolygon, not handled" << endl;
    };

    auto shpTypeName = [](int t) -> string {
        if (t == wkbPoint) return "Point";
        if (t == wkbLineString) return "LineString";
        if (t == wkbPolygon) return "Polygon";
        if (t == wkbMultiPolygon) return "MultiPolygon";
        return "UNKNOWN";
    };

    auto handleFeature = [&](OGRGeometry* geo, VRGeoDataPtr data, size_t fI, bool verbose) {
        auto type = wkbFlatten(geo->getGeometryType());
        if (verbose) cout << "handle feature " << shpTypeName(type) << endl;
        if (type == wkbPoint) handlePoint(geo, data, fI);
        else if (type == wkbLineString) handleLine(geo, data, fI, verbose);
        else if (type == wkbPolygon) handlePolygon(geo, data, fI);
        else if (type == wkbMultiPolygon) handleMultiPolygon(geo, data, fI);
        else cout << "loadSHP::handleGeometry WARNING: type " << type << " not handled!\n";
    };

    cout << "opened file " << path << " with layers:" << endl;

    for (int i=0; i<poDS->GetLayerCount(); i++) {
        OGRLayer* poLayer = poDS->GetLayer(i);
        if (!poLayer) continue;
        cout << " " << i << " " << poLayer->GetName() << ", geom type: " << shpTypeName(poLayer->GetGeomType()) << endl;
        string layer_name = poLayer->GetName();

        VRGeoDataPtr data = VRGeoData::create();
        poLayer->ResetReading();
        int gi = 0;
        auto entLayer = ontology->addEntity("layer", "Layer");
        entLayer->set("type", shpTypeName( (int)poLayer->GetGeomType() ));
        OGRFeatureDefn* poFDefn = poLayer->GetLayerDefn();
        size_t Nfields = poFDefn->GetFieldCount();
        vector<pair<string, OGRFieldType>> fields;

        for( int field = 0; field < poFDefn->GetFieldCount(); field++ ) {
            OGRFieldDefn* poFieldDefn = poFDefn->GetFieldDefn( field );
            string name = poFieldDefn->GetNameRef();
            OGRFieldType type = poFieldDefn->GetType();
            fields.push_back( { name, type } );
            cout << "   field name: " << poFieldDefn->GetNameRef() << ", type: " << poFieldDefn->GetType() << endl;

            if (type == OFTReal) Shape->addProperty(name, "float");
            else if (type == OFTInteger) Shape->addProperty(name, "int");
            //else if (type == OFTBinary) Shape->addProperty(name, "bool"); // TODO: should work, but test first!
            else Shape->addProperty(name, "string");
        }

        OGRFeature* poFeature = 0;
        vector<VRPropertyPtr> shapeProps;
        while( (poFeature = poLayer->GetNextFeature()) != NULL ) {
            auto entShape = ontology->addEntity("shape", "Shape");
            if (shapeProps.size() == 0) shapeProps = entShape->getProperties();

            //cout << " fields: ";
            for( int i = 0; i < Nfields; i++ ) {
                auto& field = fields[i];
                string& name = field.first;
                OGRFieldType& type = field.second;

                string value = poFeature->GetFieldAsString(i);
                if (value == "?") value = "F"; // GetFieldAsString may mess up..
                auto prop = shapeProps[i]->copy();
                entShape->addProperty(prop, name, value);
                //entShape->set(name, value);

                //cout << name;
                //if ( poFieldDefn->GetType() == OFTInteger ) printf( "  %d, ", poFeature->GetFieldAsInteger(field) );
                //if ( poFieldDefn->GetType() == OFTReal ) printf( "  %.3f, ", poFeature->GetFieldAsDouble(field) );
                //if ( poFieldDefn->GetType() == OFTString ) printf( "  %s, ", poFeature->GetFieldAsString(field) );
            }
            //cout << endl;

            OGRGeometry* geo = poFeature->GetGeometryRef();
            if (geo) handleFeature(geo, data, gi, false);
            OGRFeature::DestroyFeature( poFeature );
            entLayer->add("features", entShape->getName());
            gi++;
        }
        //cout << "  handled " << gi << " features" << endl;

        if (data->size() > 0) {
            auto vlayer = data->asGeometry(layer_name);
            vlayer->setEntity(entLayer);
            vlayer->addAttachment<vector<Vec3d>>("geoCoords", geoCoords);
            res->addChild(vlayer);
        }
    }

#if GDAL_VERSION_MAJOR < 2
 	OGRDataSource::DestroyDataSource(poDS);
#else
	GDALClose(poDS);
#endif
}

// TODO: use layer name as file name? export each layer as single file?
void writeSHP(VRObjectPtr obj, string path, map<string, string> options) {
    cout << "Write to SHP file " << path << endl;
    OGRRegisterAll();

    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
    if (driver == NULL) { cout << " Error: SHP file driver not found!" << endl; return; }
    GDALDataset* dataset = driver->Create(path.c_str(), 0, 0, 0, GDT_Unknown, nullptr);
    if (dataset == NULL) { cout << " Error: SHP dataset creation failed!" << endl; return; }

    for (auto child : obj->getChildren("Gometry")) {
        auto geo = dynamic_pointer_cast<VRGeometry>(child);
        if (!geo) continue;

        VREntityPtr layerEntity = geo->getEntity();
        if (!layerEntity) continue;

        vector<Vec3d> geoCoords = geo->getAttachment<vector<Vec3d>>("geoCoords");
        VRGeoData data(geo);
        if (data.size() == 0) continue;

        OGRwkbGeometryType layerType = OGRwkbGeometryType(layerEntity->getValue<int>("type", 0));

        cout << " add layer " << geo->getName() << endl;

        OGRLayer* layer = dataset->CreateLayer(geo->getBaseName().c_str(), nullptr, layerType, nullptr);
        if (layer == NULL) { cout << " Error: SHP layer creation failed!" << endl; continue; }

        //OGRFieldDefn fieldDefn("field_name", OFTString);
        //if (layer->CreateField(&fieldDefn) != OGRERR_NONE) { cout << " Error: SHP field creation failed!" << endl; continue; }

        // get continuous SHP features.. this may be incorrect if geometry was altered, this is filtered below
        vector<Vec2i> features = { Vec2i(0,0) };
        size_t lT = data.getTexCoord(0)[0];
        for (size_t i=1; i<data.size(); i++) {
            size_t t = data.getTexCoord(i)[0];
            if (t == lT) features[features.size()-1][1] = i;
            else features.push_back( Vec2i(i,i) );
            lT = t;
        }
        cout << "  found " << features.size() << " features" << endl;

        // check all edges
        map<int, vector<int>> edges;
        for (auto& prim : data) {
            if (prim.type != 1) continue;
            int i1 = prim.indices[0];
            int i2 = prim.indices[1];
            edges[i1].push_back(i2);
        }

        auto isValidEdge = [&](int i1, int i2) {
            if (!edges.count(i1)) return false;
            auto& v = edges[i1];
            return find(v.begin(), v.end(), i2) != v.end();
        };

        // filter edges
        if (layerType == wkbLineString) {
            vector<Vec2i> newFeatures;
            for (auto& f : features) {
                bool addToNew = false;
                size_t _f = f[1]; // to be able to change it later
                for (size_t i=f[0]; i<_f; i++) {
                    int i1 = i;
                    int i2 = i+1;
                    if (isValidEdge(i1,i2)) {
                        if (addToNew) {
                            newFeatures[newFeatures.size()-1][1] = i2;
                        }
                    } else {
                        if (!addToNew) { // first change to feature
                            addToNew = true;
                            f[1] = i; // cap
                        }
                        newFeatures.push_back(Vec2i(i2,i2));
                    }
                }
            }
            for (auto f : newFeatures) features.push_back(f);
        }

        vector<VREntityPtr> featAttribs = layerEntity->getAllEntities("features");
        for (auto prop : featAttribs[0]->getProperties()) {
            string type = prop->getType();
            OGRFieldType t = OFTString;
            if (type == "int") t = OFTInteger;
            if (type == "float") t = OFTReal;

            OGRFieldDefn fd(prop->getName().c_str(), t);
            layer->CreateField(&fd);
            cout << "add field: " << prop->getName() << ", " << t << endl;
        }

        // translate to OGR features
        for (auto& f : features) {
            OGRFeature* feature = OGRFeature::CreateFeature( layer->GetLayerDefn() );

            int fID = data.getTexCoord(f[0])[0];
            VREntityPtr featEnt = featAttribs[fID];

            for (auto prop : featEnt->getAll()) {
                feature->SetField(prop->getName().c_str(), prop->getValue().c_str());
                //if (prop->getName() == "RAIL-COST") cout << " set exported field: " << prop->getName() << ", " << prop->getValue() << endl;
                //cout << " set exported field: " << prop->toString() << endl;
            }

            if (layerType == wkbPoint) {
                Vec2d t = data.getTexCoord(f[0]);
                Vec3d p = geoCoords[t[1]];
                OGRPoint point = toOGRPoint(p);
                feature->SetGeometry(&point);
            }

            if (layerType == wkbLineString) {
                if (f[1] == f[0]) continue; // may happen if edge is deleted
                OGRLineString line;
                for (size_t i=f[0]; i<=f[1]; i++) {
                    Vec2d t = data.getTexCoord(i);
                    Vec3d p = geoCoords[t[1]];
                    OGRPoint point = toOGRPoint(p);
                    //line.addPoint(&point);
                    line.addPoint(point.getX(), point.getY(), point.getZ());
                }
                feature->SetGeometry(&line);
            }

            if (layer->CreateFeature(feature) != OGRERR_NONE) { cout << " Error: SHP feature creation failed!" << endl; continue; }
            OGRFeature::DestroyFeature(feature);
        }
    }

    cout << " wrote " << dataset->GetLayerCount() << " layers" << endl;

    GDALClose(dataset);
}

void loadTIFF(string path, VRTransformPtr res, map<string, string> opts) {
    // setup object
    auto t = loadGeoRasterData(path);
    auto m = VRMaterial::create("GeoTiff");
    m->setTexture(t);
    m->setLit(0);
    m->setTextureParams(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_MODULATE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

    int NXgeo = 140;
    int NYgeo = 140;
    float SXchunk = 2.0/NXgeo;
    float SYchunk = 2.0/NYgeo;
    float TCXchunk = 1.0/NXgeo;
    float TCYchunk = 1.0/NYgeo;

    VRGeoData geo;
    for (int i=0; i<NXgeo; i++) {
        float px1 = -1 + i*SXchunk;
        float px2 = px1 + SXchunk;
        float tcx1 = 0 + i*TCXchunk;
        float tcx2 = tcx1 + TCXchunk;

        for (int j=0; j<NYgeo; j++) {
            float py1 = -1 + j*SYchunk;
            float py2 = py1 + SYchunk;
            float tcy1 = 0 + j*TCYchunk;
            float tcy2 = tcy1 + TCYchunk;

            geo.pushVert(Vec3d(px1,0,py1), Vec3d(0,1,0), Vec2d(tcx1,tcy1));
            geo.pushVert(Vec3d(px1,0,py2), Vec3d(0,1,0), Vec2d(tcx1,tcy2));
            geo.pushVert(Vec3d(px2,0,py2), Vec3d(0,1,0), Vec2d(tcx2,tcy2));
            geo.pushVert(Vec3d(px2,0,py1), Vec3d(0,1,0), Vec2d(tcx2,tcy1));
            geo.pushQuad();
        }
    }
    auto g = geo.asGeometry(path);
    g->setMaterial(m);
    res->addChild( g );
}

VRTexturePtr loadGeoRasterData(string path, bool shout, float *heightoffset) {
    cout << "loadGeoRasterData " << path << endl;
    GDALAllRegister();
    GDALDataset* poDS = (GDALDataset *) GDALOpen( path.c_str(), GA_ReadOnly );
    if( poDS == NULL ) { printf( "Open failed.\n" ); return 0; }

    // general information
    double adfGeoTransform[6];
    if (shout) printf( "Driver: %s/%s\n", poDS->GetDriver()->GetDescription(), poDS->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );
    if (shout) printf( "Size is %dx%dx%d\n", poDS->GetRasterXSize(), poDS->GetRasterYSize(), poDS->GetRasterCount() );
    if( poDS->GetProjectionRef()  != NULL ) { if (shout) printf( "Projection is `%s'\n", poDS->GetProjectionRef() ); }
    if( poDS->GetGeoTransform( adfGeoTransform ) == CE_None ) {
        //if (!shout) printf( "loadGeoRasterData Origin = (%.6f,%.6f)\n", adfGeoTransform[0], adfGeoTransform[3] );
        if (shout) printf( "Origin = (%.6f,%.6f)\n", adfGeoTransform[0], adfGeoTransform[3] );
        if (shout) printf( "Pixel Size = (%.6f,%.6f)\n", adfGeoTransform[1], adfGeoTransform[5] );
    }

    // get first block
    auto getBand = [&](int i) {
        int nBlockXSize, nBlockYSize;
        int bGotMin, bGotMax;
        double adfMinMax[2];
        GDALRasterBand* poBand = poDS->GetRasterBand( 1 );
        poBand->GetBlockSize( &nBlockXSize, &nBlockYSize );
        adfMinMax[0] = poBand->GetMinimum( &bGotMin );
        adfMinMax[1] = poBand->GetMaximum( &bGotMax );
        if( ! (bGotMin && bGotMax) ) GDALComputeRasterMinMax((GDALRasterBandH)poBand, TRUE, adfMinMax);
        if (heightoffset) if (adfMinMax[0] > -100000) *heightoffset += (float( adfMinMax[0] ) +float( adfMinMax[1] ))/2.0; //sloppy offset, min often = NO DATA VALUE
        return poBand;
    };

    vector<float> data;
    for (int i=1; i<=poDS->GetRasterCount(); i++) {
        auto band = getBand(i);
        int nXSize = band->GetXSize();
        int nYSize = band->GetYSize();
        vector<float> line(nXSize*nYSize);
        CPLErr err = band->RasterIO( GF_Read, 0, (i-1)*nYSize, nXSize, nYSize, &line[0], nXSize, nYSize, GDT_Float32, 0, 0 );
        if (err == CE_None) data.insert(data.end(), line.begin(), line.end());
    }

    int sizeX = poDS->GetRasterXSize();
    int sizeY = poDS->GetRasterYSize();
    if (heightoffset && poDS->GetRasterCount() > 0) *heightoffset /= poDS->GetRasterCount();
    GDALClose(poDS);

    auto t = VRTexture::create();
#ifndef __EMSCRIPTEN__
    t->setFloatData(data, Vec3i(sizeX,sizeY,1), GL_RED, 0, GL_R32F);
#else
    t->setFloatData(data, Vec3i(sizeX,sizeY,1), Image::OSG_R_PF, 0, GL_R32F);
#endif
    cout << " loadGeoRasterData done" << endl;
    return t;
}

VRTexturePtr processGeoRasterData(string data, string driver, bool shout, float *heightoffset) { // TODO
	GDALDriverManager *poDM = GetGDALDriverManager();
	GDALDriver *poDriver = poDM->GetDriverByName("GTiff");
	//GDALDataset *poDS = poDriver->pfnOpen(&oOpenInfo);
    return 0;
}

void divideTiffIntoChunks(string pathIn, string pathOut, double minLat, double maxLat, double minLon, double maxLon, double res) {
    //cout << " gdal - " << pathIn << pathOut << endl;
    GDALAllRegister();
    GDALDataset* poDS = (GDALDataset *) GDALOpen( pathIn.c_str(), GA_ReadOnly );
    if( poDS == NULL ) { printf( "Open failed.\n" ); return; }

    // general information
    double adfGeoTransform[6];
    printf( "Driver: %s/%s\n", poDS->GetDriver()->GetDescription(), poDS->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );
    printf( "Size is %dx%dx%d\n", poDS->GetRasterXSize(), poDS->GetRasterYSize(), poDS->GetRasterCount() );
    //if( poDS->GetProjectionRef()  != NULL ) { printf( "Projection is `%s'\n", poDS->GetProjectionRef() ); }
    if( poDS->GetGeoTransform( adfGeoTransform ) == CE_None ) {  }

    cout << " filepath: " << pathIn << endl;
    //cout << " bandCount: " << poDS->GetRasterCount() << endl;

    ///https://wiki.openstreetmap.org/wiki/Mercator
    auto DEG2RAD = [&](double a) { return (a) / (180 / M_PI); };
    auto RAD2DEG = [&](double a) { return (a) * (180 / M_PI); };
    auto EARTH_RADIUS = 6378137;

    /* The following functions take their parameter and return their result in degrees */
    //auto y2lat_d = [&](double y)   { return RAD2DEG( atan(exp( DEG2RAD(y) )) * 2 - M_PI/2 ); };
    //auto x2lon_d = [&](double x)   { return x; };

    //auto lat2y_d = [&](double lat) { return RAD2DEG( log(tan( DEG2RAD(lat) / 2 +  M_PI/4 )) ); };
    //auto lon2x_d = [&](double lon) { return lon; };

    /* The following functions take their parameter in something close to meters, along the equator, and return their result in degrees */
    auto y2lat_m = [&](double y)   { return RAD2DEG(2 * atan(exp( y/EARTH_RADIUS)) - M_PI/2); };
    auto x2lon_m = [&](double x)   { return RAD2DEG(              x/EARTH_RADIUS           ); };

    /* The following functions take their parameter in degrees, and return their result in something close to meters, along the equator */
    auto lat2y_m = [&](double lat) { return log(tan( DEG2RAD(lat) / 2 + M_PI/4 )) * EARTH_RADIUS; };
    auto lon2x_m = [&](double lon) { return          DEG2RAD(lon)                 * EARTH_RADIUS; };

    double xBeg = adfGeoTransform[0];
    double yBeg = adfGeoTransform[3];
    double xEnd = adfGeoTransform[0] + poDS->GetRasterXSize()*adfGeoTransform[1];
    double yEnd = adfGeoTransform[3] + poDS->GetRasterYSize()*adfGeoTransform[5];
    double latBeg = y2lat_m(yBeg);
    double lonBeg = x2lon_m(xBeg);
    double latEnd = y2lat_m(yEnd);
    double lonEnd = x2lon_m(xEnd);

    /*
    Vec2d upperLeft = Vec2d(xBeg,yBeg);
    Vec2d lowerLeft = Vec2d(xBeg,yEnd);
    Vec2d upperRight = Vec2d(xEnd,yBeg);
    Vec2d lowerRight = Vec2d(xEnd,yEnd);
    cout << "upperLeft " << upperLeft << endl;
    cout << "lowerLeft " << lowerLeft << endl;
    cout << "upperRight " << upperRight << endl;
    cout << "lowerRight " << lowerRight << endl;
    cout << "minLat " << minLat << " maxLat " << maxLat << " minLon " << minLon << " maxLon " << maxLon << " res " << res << endl;
    cout << "LatO " << latBeg << " LonO " << lonBeg << endl;
    cout << "Lat1 " << latEnd << " Lon1 " << lonEnd << endl;
    */

    vector<double> bordersX;
    vector<double> bordersY;

    //double currentY = maxLat;
    double currentX = minLon;
    double currentY = maxLat;
    while (currentX <= maxLon) {
        bordersX.push_back(currentX);
        currentX += res;
    }
    while (currentY >= minLat) {
        bordersY.push_back(currentY);
        currentY -= res;
    }

    /*
    cout << "input xA " << xA << " yA " << yA << " xB " << xB << " yB " << yB << endl;
    cout << "file  xA " << adfGeoTransform[0] << " yA " << adfGeoTransform[3] << " xB " << xEnd << " yB " << yEnd << endl;
    cout << "Sy " << y2lat_m(adfGeoTransform[5]) << " Sx " << x2lon_m(adfGeoTransform[1]) << endl;
    */

    for (unsigned int yy = 0; yy < bordersY.size()-1; yy++){
        for (unsigned int xx = 0; xx < bordersX.size()-1; xx++) {
            if ( bordersY[yy] < latBeg  && bordersX[xx] > lonBeg && bordersY[yy+1] > latEnd  && bordersX[xx+1] < lonEnd ) {
                cout << " within bounds " << xx << "-" << yy << " | " << bordersY[yy] << " " << bordersX[xx] << " | " << bordersY[yy+1] << " " << bordersX[xx+1] << endl;
                double xxA = lon2x_m(bordersX[xx]);
                double xxB = lon2x_m(bordersX[xx+1]);
                double yyA = lat2y_m(bordersY[yy]);
                double yyB = lat2y_m(bordersY[yy+1]);
                //cout << "  borders  " << xxA << " " << yyA << " " << xxB << " " << yyB << endl;

                int xpA = (int)((xxA-adfGeoTransform[0])/adfGeoTransform[1]);
                int xpB = (int)((xxB-adfGeoTransform[0])/adfGeoTransform[1]);
                int ypA = (int)((yyA-adfGeoTransform[3])/adfGeoTransform[5]);
                int ypB = (int)((yyB-adfGeoTransform[3])/adfGeoTransform[5]);
                //cout << "  borders  " << xpA << " " << ypA << " " << xpB << " " << ypB << endl;

                int sizeX = xpB-xpA;
                int sizeY = ypB-ypA;

                //cout << " size " << sizeX << " " << sizeY << " " << sizeX*sizeY << endl;
                string savePath = "N"+to_string(bordersY[yy+1])+"E"+to_string(bordersX[xx])+"S"+to_string(res)+".tif";
                string savePath2 = "N"+to_string(bordersY[yy+1])+"E"+to_string(bordersX[xx])+"S"+to_string(res)+".png";
                vector<char> data;

                for (int y = 0; y < sizeY; y++) {
                    for (int x = 0; x < sizeX; x++) {
                        vector<char> pix(3);
                        CPLErr err;
                        err = poDS->GetRasterBand(1)->RasterIO( GF_Read, xpA+x, ypB-y, 1, 1, &pix[0], 1, 1, GDT_Byte, 0, 0 );
                        err = poDS->GetRasterBand(2)->RasterIO( GF_Read, xpA+x, ypB-y, 1, 1, &pix[1], 1, 1, GDT_Byte, 0, 0 );
                        err = poDS->GetRasterBand(3)->RasterIO( GF_Read, xpA+x, ypB-y, 1, 1, &pix[2], 1, 1, GDT_Byte, 0, 0 );
                        if (err == CE_None) data.insert(data.end(), pix.begin(), pix.end());
                    }
                }

                auto t = VRTexture::create();
                t->setInternalFormat(GL_RGB8UI);
                auto img = t->getImage();
                img->set( Image::OSG_RGB_PF, sizeX, sizeY, 1, 1, 1, 0, (const unsigned char*)&data[0], Image::OSG_UINT8_IMAGEDATA, true, 1);
                t->write(savePath2);
            }
            else { /*cout << " out of bounds " << xx << "-" << yy << " | " << bordersY[yy] << " " << bordersX[xx] << " | " << bordersY[yy+1] << " " << bordersX[xx+1] << endl;*/ }
        }
    }
    GDALClose(poDS);
}

void divideTiffIntoChunksEPSG(string pathIn, string pathOut, double minEasting, double maxEasting, double minNorthing, double maxNorthing, double pixelResolution, double chunkResolution, bool debug) {
    //cout << " gdal - " << pathIn << pathOut << endl;
    cout << "VRGDAL::divideTiffIntoChunksEPSG" << endl;
    GDALAllRegister();
    GDALDataset* poDS = (GDALDataset *) GDALOpen( pathIn.c_str(), GA_ReadOnly );
    if( poDS == NULL ) { printf( "Open failed.\n" ); return; }

    // general information
    double adfGeoTransform[6];
    printf( "Driver: %s/%s\n", poDS->GetDriver()->GetDescription(), poDS->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );
    printf( "Size is %dx%dx%d\n", poDS->GetRasterXSize(), poDS->GetRasterYSize(), poDS->GetRasterCount() );
    //if( poDS->GetProjectionRef()  != NULL ) { printf( "Projection is `%s'\n", poDS->GetProjectionRef() ); }
    if( poDS->GetGeoTransform( adfGeoTransform ) == CE_None ) {  }

    cout << " filepath: " << pathIn << endl;
    //cout << " bandCount: " << poDS->GetRasterCount() << endl;
    GDALRasterBand* poBand = poDS->GetRasterBand( 1 );
    auto rasterDataType = poBand->GetRasterDataType();
    //cout << " bandType: " << rasterDataType << endl;
    if ( rasterDataType != 6 && poDS->GetRasterCount() > 1) {
        GDALClose(poDS);
        cout << "ERROR: VRGDAL::DIVIDER OF CHUNKS - only implemented for FLOAT32, single RASTERBAND" << endl;
        return;
    }

    double xBeg = adfGeoTransform[0];
    double yBeg = adfGeoTransform[3];
    double xEnd = adfGeoTransform[0] + poDS->GetRasterXSize()*adfGeoTransform[1];
    double yEnd = adfGeoTransform[3] + poDS->GetRasterYSize()*adfGeoTransform[5];

    Vec2d upperLeft = Vec2d(xBeg,yBeg);
    Vec2d lowerLeft = Vec2d(xBeg,yEnd);
    Vec2d upperRight = Vec2d(xEnd,yBeg);
    Vec2d lowerRight = Vec2d(xEnd,yEnd);
    if (debug && false) {
        cout << "eastBeg " << to_string(xBeg) << endl;
        cout << "eastEnd " << to_string(xEnd) << endl;
        cout << "northBeg " << to_string(yBeg) << endl;
        cout << "northEnd " << to_string(yEnd) << endl;
    }
    vector<double> bordersX;
    vector<double> bordersY;

    if (minEasting < xBeg || maxNorthing > yBeg) {
        GDALClose(poDS);
        cout << "ERROR: VRGDAL::divideTiffIntoChunksEPSG - parameters out of bounds" << endl;
        if (minEasting < xBeg)  cout << "  minEasting too small" << endl;
        if (maxNorthing > yBeg)  cout << "  maxNorthing too big" << endl;
        return;
    }
    if (maxEasting > xEnd || minNorthing < yEnd){
        cout << "WARNING: VRGDAL::divideTiffIntoChunksEPSG - maxEasting or minNorthing out of geoTIFF's bounds" << endl;
        if (maxEasting > xEnd)  cout << "  maxEasting too big" << endl;
        if (minNorthing > yEnd)  cout << "  minNorthing too small" << endl;
    }

    double currentX = minEasting;
    double currentY = maxNorthing;
    while (currentX <= xEnd && currentX + 2*chunkResolution <= xEnd ) {
        bordersX.push_back(currentX);
        currentX += chunkResolution;
    }
    while (currentY >= yEnd && currentY - 2*chunkResolution >= yEnd) {
        bordersY.push_back(currentY);
        currentY -= chunkResolution;
    }

    cout << "Borders: E:"<< bordersX.size() << " N:"<< bordersY.size() << " Sum:"<< bordersX.size() * bordersY.size() << endl;

    int nOffsetX = int((bordersX[0]-xBeg)/adfGeoTransform[1]);
    int nOffsetY = int((bordersY[0]-yBeg)/adfGeoTransform[5]);
    cout << "nOffsetX:" << nOffsetX << " nOffsetY:" << nOffsetY << endl;

    int factorX = pixelResolution/adfGeoTransform[1];
    int factorY = pixelResolution/-adfGeoTransform[5];
    cout << " factorX:" << factorX << " factorY:" << factorY << endl;
    int sizeX = int(chunkResolution/pixelResolution)+1;
    int sizeY = int(chunkResolution/pixelResolution)+1;
    int sizeOrignalX = factorX * sizeX;
    int sizeOrignalY = factorY * sizeY;
    cout << " oX:" << sizeOrignalX << " oY:" << sizeOrignalY << endl;
    cout << " rX:" << sizeX << " rY:" << sizeY << endl;
    int xChSize = bordersX.size();
    int yChSize = bordersY.size();
    int fractions = 0;

    if (debug) {
        GDALClose(poDS);
        cout << "WARNING: VRGDAL::divideTiffIntoChunksEPSG - TESTMODE, WILL NOT EXECUTE" << endl;
        return;
    }

    for (int yCh = 0; yCh < yChSize; yCh ++){
        for (int xCh = 0; xCh < xChSize; xCh ++){
            string sRes = to_string(chunkResolution);
            int sAt = sRes.length();
            if (sRes.find(".")) {
                if (sRes.find(".")+fractions <= sRes.length()) sAt = sRes.find(".")+fractions;
            }
            string savePath = pathOut + "/dgm_E32"+to_string(bordersX[xCh]).substr(0,8)+"_N"+to_string(bordersY[yCh]).substr(0,9)+"_S"+to_string(chunkResolution).substr(0,sAt)+".tif";
            //cout << "writeGeoRasterData at " << savePath << " - X: "  << sizeX << " Y: " << sizeY << endl;

            //auto band = getBand(i); TODO: solve possibility of multiple bands existing
            ///READDATA:
            vector<vector<float>> originalData;
            for (int yy = 0; yy < sizeOrignalY; yy++){
                vector<float> line(sizeOrignalX);
                int nXOff = nOffsetX + xCh*(sizeOrignalX-factorX); //The pixel offset to the top left corner of the region of the band to be accessed. This would be zero to start from the left side.
                int nYOff = nOffsetY + yCh*(sizeOrignalY-factorY) + yy; //The line offset to the top left corner of the region of the band to be accessed. This would be zero to start from the top.
                CPLErr err = poBand->RasterIO( GF_Read, nXOff, nYOff, sizeOrignalX, 1, &line[0], sizeOrignalX, 1, rasterDataType, 0, 0 );
                if (err == CE_None) originalData.push_back(line);
            }
            ///DOWNSAMPLE:
            vector<float> data;
            for (int y = 0; y < sizeY; y++){
                for (int x = 0; x < sizeX; x++){
                    float nPixel = 0.0;
                        for (int v = y*factorY; v < (y+1)*factorY; v++){
                            for (int u = x*factorX; u < (x+1)*factorX; u++){
                                nPixel += originalData[v][u];
                            }
                        }
                        nPixel = nPixel / (factorX * factorY);
                        //if (nPixel > -3e38 && nPixel != 0.0) cout << to_string(nPixel) << endl;
                    data.push_back(nPixel);
                }
            }
            const char *pszFormat = "GTiff";
            GDALDriver *poDriver;
            poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
            if( poDriver == NULL )
                return;

            GDALDataset *poDstDS;
            char **papszOptions = NULL;
            poDstDS = poDriver->Create( savePath.c_str(), sizeX, sizeY, 1, rasterDataType, papszOptions );
            GDALRasterBand *newpoBand;
            newpoBand = poDstDS->GetRasterBand(1);

            string projWkt = poDS->GetProjectionRef();
            poDstDS->SetProjection( projWkt.c_str() );

            double newGeoTransform[6];
            newGeoTransform[0] = bordersX[xCh];//adfGeoTransform[0];//xBeg;
            newGeoTransform[1] = adfGeoTransform[1];
            newGeoTransform[2] = 0.0;
            newGeoTransform[3] = bordersY[yCh];//adfGeoTransform[3];//yBeg;
            newGeoTransform[4] = 0.0;
            newGeoTransform[5] = adfGeoTransform[5];
            poDstDS->SetGeoTransform( newGeoTransform );
            //cout << " origX:" << to_string(newGeoTransform[0]) << " origY:" << to_string(newGeoTransform[3]) << endl;

            float *yRow = (float*) CPLMalloc(sizeof(float)*sizeY);
            for (int y = 0; y < sizeY; y++){
                for (int x = 0; x < sizeX; x++) {
                    yRow[x] = data[y*sizeX+x];
                }
                CPLErr newerr = newpoBand->RasterIO( GF_Write, 0, y, sizeX, 1, yRow, sizeX, 1, GDT_Float32, 0, 0 );
            }
            GDALClose( (GDALDatasetH) poDstDS );
        }
    }

    GDALClose(poDS);
}

void writeGeoRasterData(string path, VRTexturePtr tex, double geoTransform[6], string params[3]) {
    try {
	    const char *pszFormat = "GTiff";
	    GDALDriver *poDriver;
	    poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
	    if( poDriver == NULL )
		return;
	    Vec3i texSize = tex->getSize();
	    int sizeX = texSize[0];
	    int sizeY = texSize[1];
	    cout << "writeGeoRasterData at " << path << " - X: "  << sizeX << " Y: " << sizeY << endl;
	    //double originLat = geoTransform[0];
	    //double originLon = geoTransform[3];
	    GDALDataset *poDstDS;
	    char **papszOptions = NULL;
	    poDstDS = poDriver->Create( path.c_str(), sizeX, sizeY, 1, GDT_Float32,
		                        papszOptions );
	    OGRSpatialReference oSRS;
	    char *pszSRS_WKT = NULL;
	    GDALRasterBand *poBand;
	    poDstDS->SetGeoTransform( geoTransform );
	    //TODO: CHANGE UTM and GeogCSM
	    oSRS.SetUTM( 11, TRUE );
	    oSRS.SetWellKnownGeogCS( "WGS84" );
	    oSRS.exportToWkt( &pszSRS_WKT );
	    poDstDS->SetProjection( pszSRS_WKT );
	    CPLFree( pszSRS_WKT );
	    poBand = poDstDS->GetRasterBand(1);

	    float *yRow = (float*) CPLMalloc(sizeof(float)*sizeY);
	    for (int y = 0; y < sizeY; y++){
		for (int x = 0; x < sizeX; x++) {
		    Vec3i pI(x,y,0);
		    auto fC = tex->getPixelVec(pI)[0];
		    yRow[x] = fC;
		}
		CPLErr err = poBand->RasterIO( GF_Write, 0, y, sizeX, 1, yRow, sizeX, 1, GDT_Float32, 0, 0 );
		if (err != CE_None) break;
	    }
	    /* Once we're done, close properly the dataset */
	    CPLFree(yRow);
	    GDALClose( (GDALDatasetH) poDstDS );
	    cout << "writeGeoRasterData done\n" << endl;
    } catch(...) {
         cout << "writeGeoRasterData failed! GDAL threw exception!" << endl;
    }
}

vector<double> getGeoTransform(string path) {
    vector<double> res(6,0);
    double adfGeoTransform[6];
    GDALAllRegister();
    GDALDataset* poDS = (GDALDataset *) GDALOpen( path.c_str(), GA_ReadOnly );
    if( poDS == NULL ) { printf( "Open failed.\n" ); return res; }

    // general information
    if( poDS->GetGeoTransform( adfGeoTransform ) == CE_None ) {}
    for (int i = 0; i < 6; i++) res[i] = adfGeoTransform[i];
    GDALClose(poDS);
    return res;
}


void analyzeSHP(string path, string out) {
    OGRRegisterAll();
#if GDAL_VERSION_MAJOR < 2
 	OGRDataSource *poDS = OGRSFDriverRegistrar::Open(path.c_str(), false);
#else
	GDALDataset *poDS = (GDALDataset*) GDALOpenEx(path.c_str(), GDAL_OF_READONLY, NULL, NULL, NULL);
#endif

    if( poDS == NULL ) { printf( "Open failed.\n" ); return; }

    ofstream fout(out);

    auto handlePoint = [&](OGRGeometry* geo) {
        OGRPoint* pnt = (OGRPoint*)geo;
        Vec3d p = toVec3d(*pnt);
        fout << "point " << p << endl;
    };

    auto handleLine = [&](OGRGeometry* geo) {
        OGRLineString* line = (OGRLineString*) geo;
        OGRPoint pnt;
        fout << "line" << endl;
        for (int i=0; i<line->getNumPoints(); i++) {
            line->getPoint(i, &pnt);
            Vec3d p = toVec3d(pnt);
            fout << " " << p << endl;
        }
        fout << endl;
    };

    auto handleRing = [&](OGRGeometry* geo) {
        OGRLinearRing* ring = (OGRLinearRing*) geo;
        OGRPoint pnt;
        fout << "ring" << endl;
        for (int i=0; i<ring->getNumPoints(); i++) {
            ring->getPoint(i, &pnt);
            Vec3d p = toVec3d(pnt);
            fout << " " << p << endl;
        }
        fout << endl;
    };

    auto handlePolygon = [&](OGRGeometry* geo) {
        OGRPolygon* poly = (OGRPolygon*) geo;
        fout << "polygon" << endl;
        handleRing( poly->getExteriorRing() );
        for (int i=0; i<poly->getNumInteriorRings(); i++) {
            handleRing( poly->getInteriorRing(i) );
        }
    };

    auto handleMultiPolygon = [&](OGRGeometry* geo) {
        OGRMultiPolygon* mpoly = (OGRMultiPolygon*) geo;
        fout << "multipolygon" << endl;
        for (int i=0; i<mpoly->getNumGeometries(); i++) {
            auto poly = mpoly->getGeometryRef(i);
            if (poly) handlePolygon( poly );
            else break;
        }
    };

    auto handleFeature = [&](OGRGeometry* geo) {
        fout << "feature" << endl;
        auto type = wkbFlatten(geo->getGeometryType());
        if (type == wkbPoint) handlePoint(geo);
        else if (type == wkbLineString) handleLine(geo);
        else if (type == wkbPolygon) handlePolygon(geo);
        else if (type == wkbMultiPolygon) handleMultiPolygon(geo);
        else fout << "loadSHP::handleGeometry WARNING: type " << type << " not handled!\n";
    };

    fout << "layers:" << endl;
    for (int i=0; i<poDS->GetLayerCount(); i++) {
        OGRLayer* poLayer = poDS->GetLayer(i);
        if (!poLayer) continue;
        fout << " " << i << " " << poLayer->GetName() << ", geom type: " << (int)poLayer->GetGeomType() << endl;

        poLayer->ResetReading();
        int gi = 0;
        OGRFeatureDefn* poFDefn = poLayer->GetLayerDefn();
        for( int field = 0; field < poFDefn->GetFieldCount(); field++ ) {
            OGRFieldDefn* poFieldDefn = poFDefn->GetFieldDefn( field );
            fout << "   field name: " << poFieldDefn->GetNameRef() << ", type: " << poFieldDefn->GetType() << endl;
        }

        int fieldI = 0;
        OGRFeature* poFeature = 0;
        while( (poFeature = poLayer->GetNextFeature()) != NULL ) {
            OGRFeatureDefn* poFDefn = poLayer->GetLayerDefn();
            fout << " fields:" << endl;
            for( int field = 0; field < poFDefn->GetFieldCount(); field++ ) {
                OGRFieldDefn* poFieldDefn = poFDefn->GetFieldDefn( field );
                string name = poFieldDefn->GetNameRef();
                OGRFieldType type = poFieldDefn->GetType();
                string value = poFeature->GetFieldAsString(field);
                if (value == "?") {
                    //if (type == OFTBinary) value = "F";
                    value = "F";
                }
                fieldI++;
                fout << " " << fieldI << " (" << name << ", " << value << ", " << type << ")" << endl;
            }
            fout << endl;

            OGRGeometry* geo = poFeature->GetGeometryRef();
            if (geo) handleFeature(geo);
            OGRFeature::DestroyFeature( poFeature );
        }
    }

    fout.close();


#if GDAL_VERSION_MAJOR < 2
 	OGRDataSource::DestroyDataSource(poDS);
#else
	GDALClose(poDS);
#endif
}

OSG_END_NAMESPACE;


// stubs
/*#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <libproj/proj.h>
#include <libproj/filemanager.hpp>
namespace NS_PROJ { class File; }
EMSCRIPTEN_KEEPALIVE
int proj_context_is_network_enabled(PJ_CONTEXT* ctx) { return 0; }
void NS_PROJ::FileManager::fillDefaultNetworkInterface(PJ_CONTEXT *ctx) {}
std::unique_ptr<NS_PROJ::File> NS_PROJ::pj_network_file_open(PJ_CONTEXT *ctx, const char *filename) { return 0; }
#endif*/
