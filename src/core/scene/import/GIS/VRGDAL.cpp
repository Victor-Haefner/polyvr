#include "VRGDAL.h"

#include <iostream>
#include <gdal/gdal.h>
#include <gdal/ogrsf_frmts.h>
#include <OpenSG/OSGVector.h>
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/VRTransform.h"

OSG_BEGIN_NAMESPACE;

void loadSHP(string path, VRTransformPtr res) {
    OGRRegisterAll();
    OGRDataSource* poDS = OGRSFDriverRegistrar::Open( path.c_str(), FALSE );
    if( poDS == NULL ) { printf( "Open failed.\n" ); exit( 1 ); }

    VRGeoData data;

    auto toVec3f = [](OGRPoint& p) {
        return Vec3f( p.getX(), p.getZ(), -p.getY() );
    };

    auto handleGeometry = [&](OGRGeometry* geo) {
        auto type = wkbFlatten(geo->getGeometryType());
        OGRPoint pnt;
        if (type == wkbPoint) {
            OGRPoint* pnt = (OGRPoint*) geo;
            //cout << "  point " << pos << endl;
            data.pushVert( toVec3f(*pnt) );
            data.pushPoint();
            return;
        }
        if (type == wkbLineString) {
            OGRLineString* line = (OGRLineString*) geo;
            //cout << "  polyline: (" << line->getNumPoints() << ")";
            for (int i=0; i<line->getNumPoints(); i++) {
                line->getPoint(i, &pnt);
                data.pushVert( toVec3f(pnt) );
                if (i != 0) data.pushLine(); // TODO: add polylines to VRGeoData?
                //cout << "  p " << pos;
            }
            //cout << endl;
            return;
        }
        if (type == wkbPolygon) {
            //cout << "  polygon:" << endl;
            OGRPolygon* poly = (OGRPolygon*) geo;
            OGRLinearRing* ex = poly->getExteriorRing();
            //cout << "   outer bound:";
            for (int i=0; i<ex->getNumPoints(); i++) {
                ex->getPoint(i, &pnt);
                data.pushVert( toVec3f(pnt) );
                if (i != 0) data.pushLine();
                //cout << "  p " << pos;
            }
            //cout << endl;


            for (int i=0; i<poly->getNumInteriorRings(); i++) {
                OGRLinearRing* in = poly->getInteriorRing(i);

                //cout << "   inner bound:";
                for (int i=0; i<in->getNumPoints(); i++) {
                    in->getPoint(i, &pnt);
                    data.pushVert( toVec3f(pnt) );
                    if (i != 0) data.pushLine();
                    //cout << "  p " << pos;
                }
                //cout << endl;
            }
            return;
        }
        cout << "loadSHP::handleGeometry WARNING: type " << type << " not handled!\n";
    };

    cout << "opened file " << path << " with layers:" << endl;
    for (int i=0; i<poDS->GetLayerCount(); i++) {
        OGRLayer* poLayer = poDS->GetLayer(i);
        cout << " " << i << " " << poLayer->GetName() << endl;
        if (poLayer) {
            poLayer->ResetReading();

            OGRFeature* poFeature;
            while( (poFeature = poLayer->GetNextFeature()) != NULL ) {
                OGRFeatureDefn* poFDefn = poLayer->GetLayerDefn();
                cout << "  fields:";
                for( int field = 0; field < poFDefn->GetFieldCount(); field++ ) {
                    OGRFieldDefn* poFieldDefn = poFDefn->GetFieldDefn( field );

                    if ( poFieldDefn->GetType() == OFTInteger ) printf( "  %d,", poFeature->GetFieldAsInteger(field) );
                    if ( poFieldDefn->GetType() == OFTReal ) printf( "  %.3f,", poFeature->GetFieldAsDouble(field) );
                    if ( poFieldDefn->GetType() == OFTString ) printf( "  %s,", poFeature->GetFieldAsString(field) );
                }
                cout << endl;
                OGRGeometry* geo = poFeature->GetGeometryRef();
                if (geo) handleGeometry(geo);
                OGRFeature::DestroyFeature( poFeature );
            }
        }
    }

    OGRDataSource::DestroyDataSource( poDS );

    res->addChild( data.asGeometry(path) );
}

OSG_END_NAMESPACE;
