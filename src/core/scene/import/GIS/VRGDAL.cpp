#include "VRGDAL.h"

#include <iostream>
#include <gdal/gdal.h>
#include <gdal/ogrsf_frmts.h>

OSG_BEGIN_NAMESPACE;

void loadSHP(string path, VRTransformPtr res) {
    OGRRegisterAll();
    OGRDataSource* poDS = OGRSFDriverRegistrar::Open( path.c_str(), FALSE );
    if( poDS == NULL ) { printf( "Open failed.\n" ); exit( 1 ); }

    cout << "opened file " << path << " with layers:" << endl;
    for (int i=0; i<poDS->GetLayerCount(); i++) {
        OGRLayer* poLayer = poDS->GetLayer(i);
        cout << " " << i << " " << poLayer->GetName() << endl;
        if (poLayer) {
            poLayer->ResetReading();

            OGRFeature* poFeature;
            while( (poFeature = poLayer->GetNextFeature()) != NULL ) {
                OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
                for( int iField = 0; iField < poFDefn->GetFieldCount(); iField++ ) {
                    OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn( iField );

                    if( poFieldDefn->GetType() == OFTInteger )
                        printf( "  %d,", poFeature->GetFieldAsInteger( iField ) );
                    else if( poFieldDefn->GetType() == OFTReal )
                        printf( "  %.3f,", poFeature->GetFieldAsDouble(iField) );
                    else if( poFieldDefn->GetType() == OFTString )
                        printf( "  %s,", poFeature->GetFieldAsString(iField) );
                    else printf( "  %s,", poFeature->GetFieldAsString(iField) );
                }

                OGRGeometry* poGeometry = poFeature->GetGeometryRef();
                if( poGeometry && wkbFlatten(poGeometry->getGeometryType()) == wkbPoint ) {
                    OGRPoint *poPoint = (OGRPoint *) poGeometry;
                    printf( "  %.3f,%3.f\n", poPoint->getX(), poPoint->getY() );
                } else printf( "no point geometry\n" );
                OGRFeature::DestroyFeature( poFeature );
            }
        }
    }

    OGRDataSource::DestroyDataSource( poDS );
}

OSG_END_NAMESPACE;
