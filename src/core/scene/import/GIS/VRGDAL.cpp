#include "VRGDAL.h"

#include <iostream>
#include <gdal/gdal.h>
#include <gdal/gdal_priv.h>
#include <gdal/ogrsf_frmts.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGImage.h>
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/VRTransform.h"

OSG_BEGIN_NAMESPACE;

void loadPDF(string path, VRTransformPtr res) {
    loadTIFF(path, res);
}

void loadSHP(string path, VRTransformPtr res) {
    OGRRegisterAll();
    OGRDataSource* poDS = OGRSFDriverRegistrar::Open( path.c_str(), FALSE );
    if( poDS == NULL ) { printf( "Open failed.\n" ); return; }

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
            //cout << "  Polygon:" << endl;
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

void loadTIFF(string path, VRTransformPtr res) {
    GDALAllRegister();
    GDALDataset* poDS = (GDALDataset *) GDALOpen( path.c_str(), GA_ReadOnly );
    if( poDS == NULL ) { printf( "Open failed.\n" ); return; }

    // general information
    double adfGeoTransform[6];
    printf( "Driver: %s/%s\n", poDS->GetDriver()->GetDescription(), poDS->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );
    printf( "Size is %dx%dx%d\n", poDS->GetRasterXSize(), poDS->GetRasterYSize(), poDS->GetRasterCount() );
    if( poDS->GetProjectionRef()  != NULL ) printf( "Projection is `%s'\n", poDS->GetProjectionRef() );
    if( poDS->GetGeoTransform( adfGeoTransform ) == CE_None ) {
        printf( "Origin = (%.6f,%.6f)\n", adfGeoTransform[0], adfGeoTransform[3] );
        printf( "Pixel Size = (%.6f,%.6f)\n", adfGeoTransform[1], adfGeoTransform[5] );
    }

    // get first block
    auto getBand = [&](int i) {
        int nBlockXSize, nBlockYSize;
        int bGotMin, bGotMax;
        double adfMinMax[2];
        GDALRasterBand* poBand = poDS->GetRasterBand( 1 );
        poBand->GetBlockSize( &nBlockXSize, &nBlockYSize );
        //printf( "Block=%dx%d Type=%s, ColorInterp=%s\n", nBlockXSize, nBlockYSize, GDALGetDataTypeName(poBand->GetRasterDataType()), GDALGetColorInterpretationName( poBand->GetColorInterpretation()) );
        adfMinMax[0] = poBand->GetMinimum( &bGotMin );
        adfMinMax[1] = poBand->GetMaximum( &bGotMax );
        if( ! (bGotMin && bGotMax) ) GDALComputeRasterMinMax((GDALRasterBandH)poBand, TRUE, adfMinMax);
        //printf( "Min=%.3fd, Max=%.3f\n", adfMinMax[0], adfMinMax[1] );
        //if( poBand->GetOverviewCount() > 0 ) printf( "Band has %d overviews.\n", poBand->GetOverviewCount() );
        //if( poBand->GetColorTable() != NULL ) printf( "Band has a color table with %d entries.\n", poBand->GetColorTable()->GetColorEntryCount() );
        return poBand;
    };

    vector<float> data;
    for (int i=1; i<=poDS->GetRasterCount(); i++) {
        auto band = getBand(i);
        int nXSize = band->GetXSize();
        int nYSize = band->GetYSize();
        vector<float> line(nXSize*nYSize);
        //GDALRWFlag eRWFlag, int nXOff, int nYOff, int nXSize, int nYSize, void * pData, int nBufXSize, int nBufYSize, GDALDataType eBufType, int nPixelSpace, int nLineSpace )
        band->RasterIO( GF_Read, 0, (i-1)*nYSize, nXSize, nYSize, &line[0], nXSize, nYSize, GDT_Float32, 0, 0 );
        //for (int j=0; j<nXSize*nYSize; j++) line[j] *= 0.001; // hack
        data.insert(data.end(), line.begin(), line.end());
    }

    // TODO: print some test data

    int sizeX = poDS->GetRasterXSize();
    int sizeY = poDS->GetRasterYSize();
    //vector<float> data(sizeX*sizeY, 0.5);
    //for (int i=0; i<sizeX*sizeY; i++) data[i] = 0.5;

    // Image::OSG_A_PF, Image::OSG_ALPHA_INTEGER_PF, GL_RGBA32F_ARB, GL_ALPHA32F_ARB

    // setup object
    auto t = VRTexture::create();
    t->setInternalFormat(GL_ALPHA32F_ARB); // important for unclamped float
    auto img = t->getImage();
    img->set( Image::OSG_A_PF, sizeX, sizeY, 1, 1, 1, 0, (const uint8_t*)&data[0], Image::OSG_FLOAT32_IMAGEDATA, true, 1);
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

    // TODO: optimize!
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

            geo.pushVert(Vec3f(px1,0,py1), Vec3f(0,1,0), Vec2f(tcx1,tcy1));
            geo.pushVert(Vec3f(px1,0,py2), Vec3f(0,1,0), Vec2f(tcx1,tcy2));
            geo.pushVert(Vec3f(px2,0,py2), Vec3f(0,1,0), Vec2f(tcx2,tcy2));
            geo.pushVert(Vec3f(px2,0,py1), Vec3f(0,1,0), Vec2f(tcx2,tcy1));
            geo.pushQuad();
        }
    }
    auto g = geo.asGeometry(path);
    //geo.pushPatch(4);


    //auto g = VRGeometry::create(path);
    //g->setPrimitive("Plane", "1 1 140 140");

    g->setMaterial(m);
    res->addChild( g );
    GDALClose(poDS);
}

OSG_END_NAMESPACE;
