
#include <codecvt>

#define restrict
#define template dwg_template
#define USE_WRITE
#include <dwg_api.h>
#undef template

#include "VRDWG.h"

#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"
#include "core/math/boundingbox.h"
#include "core/math/Layer2D.h"
#include "core/math/pose.h"
#include "core/tools/VRAnnotationEngine.h"

#include <OpenSG/OSGQuaternion.h>

OSG_BEGIN_NAMESPACE;

Matrix4d getExtrusionMatrix(Vec3d& ext) {
    // This is called the "Arbitrary Axis Algorithm" to calculate the OCS x-axis from the extrusion z-vector
    Vec3d ax, ay, az;
    az = ext;
    az.normalize();

    if ((fabs (az[0]) < 1 / 64.0) && (fabs (az[1]) < 1 / 64.0)) {
        ax = Vec3d(0,1,0).cross(az);
    } else {
        ax = Vec3d(0,0,1).cross(az);
    }

    ax.normalize();
    ay = az.cross(ax);
    ay.normalize();

    Matrix4d T(ax[0], ax[1], ax[2], 0,
               ay[0], ay[1], ay[2], 0,
               az[0], az[1], az[2], 0,
                   0,     0,     0, 1);
    //return Vec3d(pt.dot(ax), pt.dot(ay), pt.dot(az));
    return T;
}

float d2r = Pi/180.0;

Vec3d asVec3d(BITCODE_3BD& v) { return Vec3d(v.x,v.y,v.z); }

string getLayerName(Dwg_Object_LAYER* layer, Dwg_Data& dwg) {
    if (!layer) return "GLOBAL";

    string name;
    char* cname = 0;

    /*Dwg_Object* obj = layer->parent->ownerhandle->obj;
    if (dwg.header.from_version >= R_13 && dwg.header.from_version < R_2000) {
        if (!(cname = dwg_find_table_extname (&dwg, obj))) cname = layer->name;
    } else cname = layer->name;*/
    cname = layer->name;

    if (dwg.header.from_version >= R_2007) { // since r2007 unicode, converted to utf-8
        std::wstring_convert<std::codecvt_utf8<char16_t>,char16_t> ucs2conv;
        name = ucs2conv.to_bytes((char16_t*)cname);
    } else name = cname?cname:"NoName";
    return name;
}

Color3f asColor3f(BITCODE_CMC& C) {
    int r,g,b;
    auto c = C.rgb;

    unsigned char i = -1;
    if (C.index > 0 && C.index < 256) i = C.index;
    else if (c>0xc3000000) i = c;

    if (i >= 0) {
        const Dwg_RGB_Palette palette = dwg_rgb_palette()[i];
        r = palette.r;
        g = palette.g;
        b = palette.b;
    } else {
        r = (c/256/256)%256;
        g = (c/256)%256;
        b = c%256;
        //cout << "C " << Vec3i(r,g,b) << endl;
    }

    return Color3f(r/255.0, g/255.0, b/255.0);
}

void printColor(Dwg_Color color) {
    cout << "Color ";
    if (color.name) cout << string(color.name);
    if (color.book_name) cout << string(color.book_name);
    if (color.index < 0) cout << " (off)";
    else if (color.index == 0) cout << " (by block)";
    else if (color.index == 256) cout << " (by layer)";
    else cout << " (index " << int(color.index) << ")";
    cout << " rgb " << color.rgb << " -> " << asColor3f(color) << " flag: " << color.flag << endl;

    printf(".rgb: 0x%06x\n", (unsigned)color.rgb);
}

struct DWGLayer {
    VRTransformPtr root;
    VRGeoData geo;
    VRAnnotationEnginePtr ann;
};

size_t aCount = 0;
struct DWGContext {
    Dwg_Data* dwg = 0;
    map<Dwg_Object_LAYER*, DWGLayer> layers;

    vector<Vec3d> offset_stack;
    vector<Vec3d> scale_stack;
    vector<Vec3d> extrusion_stack;
    vector<double> rot_angle_stack;
    vector<Dwg_Object_LAYER*> layer_stack;

    map<string, int> objectHistogram;
    map<string, int> entityHistogram;

    Matrix4d transformation;
    double rot_angle = 0;

    void compute_current_transformation() {
	    transformation.setIdentity();

		rot_angle = 0;
		for (float f : rot_angle_stack) rot_angle += f;

	    //if (offset_stack.size() > 0) cout << "compute_current_transformation " << offset_stack.size() << endl;
		for (uint i=0; i<offset_stack.size(); i++) {
			Vec3d o  = offset_stack[i];
			Vec3d s  = scale_stack[i];
			Vec3d e  = extrusion_stack[i];
			double f = rot_angle_stack[i];

			Matrix4d T,S,E;
			T.setTranslate(o);
			T.setRotate(Quaterniond(Vec3d(0,0,1),f));
			S.setScale(s);
			E = getExtrusionMatrix(e);

			T.mult(E); // maybe multLeft ?
			T.mult(S);
			transformation.mult(T);
			//cout << "  scale " << s << " pos " << o << "  extr " << e << "  ang " << f << endl;
		}
		//if (offset_stack.size() > 0) cout << endl;
	}

	double scaleLength(double l) {
        Vec3d s = Vec3d(1,0,0);
		transformation.mult(s,s);
		return l * s.length();
	}

    void addPoint(Pnt3d p, Dwg_Object_LAYER* layer) {
        VRGeoData& geo = layers[layer].geo;
		transformation.mult(p,p);
        geo.pushVert(p);
        geo.pushPoint();
	}

	void addLine(Pnt3d vec1, Pnt3d vec2, Color3f col, Dwg_Object_LAYER* layer) {
        VRGeoData& geo = layers[layer].geo;
		transformation.mult(vec1, vec1);
		transformation.mult(vec2, vec2);
        geo.pushVert(vec1);
        geo.pushVert(vec2);
        geo.pushColor(col);
        geo.pushColor(col);
        geo.pushLine();
        //cout << "addLine " << col << endl;
	}

    /** DWG arcs always rotate counterclockwise! */
	void addArc(Pnt3d c, double r, double a1, double a2, Color3f col, Dwg_Object_LAYER* layer, Vec3d eBox = Vec3d(1,1,1)) {
        //aCount++;
        //if (aCount < 54 || aCount > 55) return;
        //if (aCount != 54) return;
        //cout << " addArc, c: " << c << ", r: " << r << ", a12: " << Vec2d(a1, a2) << ", i:" << aCount << endl;
        VRGeoData& geo = layers[layer].geo;

		/*Pnt3d sp = c + Vec3d(cos(a1),sin(a1),0)*r;
		Pnt3d ep = c + Vec3d(cos(a2),sin(a2),0)*r;
		transformation.mult(sp,sp);
		transformation.mult(ep,ep);
		a1 = atan2(sp[1] - c[1], sp[0] - c[0]) + Pi;
		a2 = atan2(ep[1] - c[1], ep[0] - c[0]) + Pi;
		if (transformation.det() < 0) swap(a1, a2);*/

		if (a2 < a1) a2 += 2*Pi; // make sure to rotate counterclockwise!

		transformation.mult(c, c);

		if (r < 1e-6) r = 1; // elipse -> use box
        else r = scaleLength(r);
        double da = 0.1;
        int N = max(int(abs(a2-a1)/da),1);
        da = (a2-a1)/N;
        for (int i=0; i<=N; i++) {
            double a = a1+da*i;
            Pnt3d p = c + Vec3d(cos(a)*eBox[0], sin(a)*eBox[1], 0)*r;
            geo.pushVert(p);
            geo.pushColor(col);
            if (i > 0) geo.pushLine();
        }
        //cout << "  computed params, da: " << da << ", N: " << N << ", sr: " << r << endl;
	}

	void addCircle(Vec3d c, float r, Color3f col, Dwg_Object_LAYER* layer) {
		addArc( c, r, 0, 2*Pi, col, layer );
	}

	void addEllipse(Vec3d c, float a, float b, Color3f col, Dwg_Object_LAYER* layer) {
		Vec3d box = Vec3d(a, b,0);
		transformation.mult(box, box);
		for (int i=0; i<3; i++) box[i] = abs(box[i]);
		addArc( c, 0, 0, 2*Pi, col, layer, box );
	}

    void addText(Vec3d p, string t, double height, Dwg_Object_LAYER* layer) {
        return;

        //cout << "addText " << p << " " << t << endl;
        if (!layers[layer].ann) {
            layers[layer].ann = VRAnnotationEngine::create("text");
            layers[layer].ann->setSize(height);
        }
        auto ann = layers[layer].ann;
        ann->add(p,t);
    }

	/*void addDWGArc(Vec3d c, float r, float a1, float a2, Dwg_Object_LAYER* layer) { // TODO: maybe usefull, transforms start and end points!
		//compute start and end point, transform them and compute back to angles
		Vec3d sp = Vec3d(cos(a1),sin(a1),0)*r + c;
		Vec3d ep = Vec3d(cos(a2),sin(a2),0)*r + c;
		transformation.mult(sp,sp);
		transformation.mult(ep,ep);
		transformation.mult(c,c);

		Vec3d s = Vec3d(1,0,0);
		transformation.mult(s,s);
		r *= s.length();

		//calc start and end angle------------------------------------
		a1 = atan2(sp[1] - c[1], sp[0] - c[0]) + Pi;
		a2 = atan2(ep[1] - c[1], ep[0] - c[0]) + Pi;

		Vec2d ang = Vec2d(a1, a2); //check for mirror transformation
		if (transformation.det() < 0) ang = Vec2d(a2, a1);
		addArc( c, r, ang[0], ang[1], layer );
	}*/
};

Color3f getLayerColor(Dwg_Object_LAYER* layer) {
    if (!layer) return Color3f(0.2,0.5,0);
    Color3f col = asColor3f(layer->color); //
    //short colShrt = layer->color_rs; //
    //layer->flag;
    //Dwg_Object_MATERIAL* mat = 0; //
    //if (layer->material && layer->material->obj) mat = layer->material->obj->tio.object->tio.MATERIAL; //
    //bool pflag = layer->plotflag;
    //Dwg_Object_STYLE* pstyle = 0;
    //if (layer->plotstyle && layer->plotstyle->obj) pstyle = layer->plotstyle->obj->tio.object->tio.PLACEHOLDER

    /*cout << "getLayerColor \"" << getLayerName(layer) << "\" flag: " << layer->flag << " pstyle: " << pstyle << endl;
    if (pstyle) {
        string sn = layer->plotstyle->obj->dxfname;
        cout << " " << sn << endl;
    }*/

    //cout << "getLayerColor \"" << getLayerName(layer) << "\" (" << col << ") " << colShrt << " mat: " << mat << " flag: " << layer->flag << " pflag: " << pflag << " pstyle: " << pstyle << endl;

    return col;
}

// transform a 3D point via its OCS (extrusion) to 2D
Vec3d transform_OCS(Vec3d pt, Vec3d ext) {
    if (ext[0] == 0.0 && ext[1] == 0.0 && ext[2] ==  1.0) return pt;
    if (ext[0] == 0.0 && ext[1] == 0.0 && ext[2] == -1.0) return Vec3d(-pt[0], pt[1], pt[2]);

    // This is called the "Arbitrary Axis Algorithm" to calculate the OCS x-axis from the extrusion z-vector
    Vec3d ax, ay, az;
    az = ext;
    az.normalize();

    if ((fabs (az[0]) < 1 / 64.0) && (fabs (az[1]) < 1 / 64.0)) {
        ax = Vec3d(0.0, 1.0, 0.0).cross(az);
    } else ax = Vec3d(0.0, 0.0, 1.0).cross(az);

    ax.normalize();

    ay = az.cross(ax);
    ay.normalize();
    return Vec3d(pt.dot(ax), pt.dot(ay), pt.dot(az));
}

/*Dwg_Object_LAYER* dwg_get_entity_layer_safer (const Dwg_Object_Entity *ent) {
    if (!ent || !ent->layer || !ent->layer->obj || !ent->layer->obj->tio.object) return 0;
    return ent->layer->obj->tio.object->tio.LAYER;
}*/

Dwg_Object_LAYER* getEntityLayer(Dwg_Object* obj, DWGContext& data, bool checkName = true) {
    Dwg_Object_LAYER* layer = dwg_get_entity_layer(obj->tio.entity);
    if (!checkName) return layer;
    if (getLayerName(layer, *data.dwg) == "0") { // wrong layer
        if (data.layer_stack.size() > 0) layer = data.layer_stack.back();
    }
    return layer;
}

void process_BLOCK_HEADER(Dwg_Object_Ref* ref, DWGContext& data, bool onlyRoot);

void process_POINT(Dwg_Object* obj, DWGContext& data) {
    Dwg_Entity_POINT* point = obj->tio.entity->tio.POINT;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);
    Pnt3d P = transform_OCS( Vec3d(point->x, point->y, point->z), asVec3d(point->extrusion) );
    data.addPoint(P, layer);
}

Color3f getEntityColor(dwg_obj_ent* ent) {
    int err;
    Dwg_Color* dcol = (Dwg_Color*)dwg_ent_get_color(ent, &err);
    /*cout << "process_LINE " << hev << " " << hfv << " " << huv << " mat: " << mat << endl;
    cout << " layer: " << getLayerName(layer, *data.dwg) << " " << getLayerColor(layer) << endl;

    printColor(layer->color);
    printColor(*dcol);*/
    return asColor3f(*dcol);
}

void process_LINE(Dwg_Object* obj, DWGContext& data) {
    Dwg_Entity_LINE* line = obj->tio.entity->tio.LINE;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);

    Dwg_Object_MATERIAL* mat = 0;
    if (obj->tio.entity->material && obj->tio.entity->material->obj) mat = obj->tio.entity->material->obj->tio.object->tio.MATERIAL;
    bool hev = obj->tio.entity->has_edge_visualstyle;
    bool hfv = obj->tio.entity->has_face_visualstyle;
    bool huv = obj->tio.entity->has_full_visualstyle;
    if (mat || hev || hfv || huv) cout << "process_LINE " << hev << " " << hfv << " " << huv << " mat: " << mat << endl;

    bool vis = !obj->tio.entity->invisible;
    if (!vis) return;

    Color3f col = getEntityColor(obj->tio.entity);
    Pnt3d P1 = transform_OCS( asVec3d(line->start), asVec3d(line->extrusion) );
    Pnt3d P2 = transform_OCS( asVec3d(line->end  ), asVec3d(line->extrusion) );
    data.addLine(P1, P2, col, layer);
}

//convert the bulge of lwpolylines to arcs
void bulgeToArc(float bulge, Vec3d s, Vec3d e, Vec3d& cen, Vec3d& arc) {
	float cotbce = (1.0/bulge - bulge)*0.5;

	// Compute center point and radius
	Vec3d D = e-s;
	Vec3d T = Vec3d(-D[1], D[0], 0);
	if (abs(D[2]) > abs(D[0])) T = Vec3d(0, D[2], -D[1]);
	cen = (s + e + T*cotbce)*0.5;

	Vec3d Ls = s-cen;
	Vec3d Le = e-cen;
	float rad = Ls.length();

	Vec3d N = D.cross(T);
	N.normalize();

	// Compute start and end angles
	float sa = atan2(Ls[1], Ls[0]);
	float ea = atan2(Le[1], Le[0]);

	//check new start and end point
	Vec3d sTest = cen + Vec3d(cos(sa), sin(sa), 0)*rad;
	if ((sTest-s).squareLength() > 1) cout << "Error: test in cvtbulge failed!" << endl;

	while (sa < 0.0 || ea < 0.0) { // move angle window
        sa += 2.0*Pi;
        ea += 2.0*Pi;
	}
	if (bulge < 0.0) swap(ea,sa); // Swap angles if clockwise
	while (ea < sa) {
        ea += 2.0*Pi;
	}

	arc = Vec3d(rad, sa, ea);
}

void process_POLYLINE_2D(Dwg_Object* obj, DWGContext& data) {
    Dwg_Entity_POLYLINE_2D* line = obj->tio.entity->tio.POLYLINE_2D;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data, false);
    Color3f col = asColor3f(obj->tio.entity->color);

    int N = line->num_owned-1;
    if (line->flag & 512) N++; // closed

    for (int i=0; i<N; i++) {
        auto v1_2D = line->vertex[i]->obj->tio.entity->tio.VERTEX_2D;
        auto v2_2D = line->vertex[(i+1)%line->num_owned]->obj->tio.entity->tio.VERTEX_2D;
        BITCODE_3BD p1 = {v1_2D->point.x, v1_2D->point.y, 0};
        BITCODE_3BD p2 = {v2_2D->point.x, v2_2D->point.y, 0};
        Pnt3d P1 = asVec3d(p1);
        Pnt3d P2 = asVec3d(p2);

        double bulge = v1_2D->bulge;
        if (abs(bulge) < 1e-3) data.addLine(P1, P2, col, layer);
        else {
            //if (extr) bulge *= -1;
            Vec3d cen, arc;
            bulgeToArc(bulge, Vec3d(P1), Vec3d(P2), cen, arc);
            data.addArc(Pnt3d(cen), arc[0], arc[1], arc[2], col, layer);
        }
    }
}

void process_LWPOLYLINE(Dwg_Object* obj, DWGContext& data) {
    Dwg_Entity_LWPOLYLINE* line = obj->tio.entity->tio.LWPOLYLINE;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);
    Color3f col = getEntityColor(obj->tio.entity);

    bool vis = !obj->tio.entity->invisible;
    if (!vis) return;

    size_t N = line->num_points-1;
    if (line->flag & 512) N++; // closed

    for (size_t i=0; i<N; i++) {
        auto p1_2D = line->points[i];
        auto p2_2D = line->points[(i+1)%line->num_points];
        BITCODE_3BD p1 = {p1_2D.x, p1_2D.y, 0};
        BITCODE_3BD p2 = {p2_2D.x, p2_2D.y, 0};
        Pnt3d P1 = asVec3d(p1);
        Pnt3d P2 = asVec3d(p2);

        if (i < line->num_bulges) {
            double bulge = line->bulges[i];
            if (abs(bulge) < 1e-3) data.addLine(P1, P2, col, layer);
            else {
                //if (extr) bulge *= -1;
                Vec3d cen, arc;
                bulgeToArc(bulge, Vec3d(P1), Vec3d(P2), cen, arc);
                data.addArc(Pnt3d(cen), arc[0], arc[1], arc[2], col, layer);
            }
        } else data.addLine(P1, P2, col, layer);
    }

    //line->vertexids;
    //line->num_vertexids;
}

void process_CIRCLE(Dwg_Object* obj, DWGContext& data) {
    Dwg_Entity_CIRCLE* circle = obj->tio.entity->tio.CIRCLE;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);
    bool vis = !obj->tio.entity->invisible;
    if (!vis) return;
    Color3f col = getEntityColor(obj->tio.entity);
    data.addCircle(asVec3d(circle->center), circle->radius, col, layer);
}

void process_ARC(Dwg_Object* obj, DWGContext& data) {
    Dwg_Entity_ARC* arc = obj->tio.entity->tio.ARC;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);
    bool vis = !obj->tio.entity->invisible;
    if (!vis) return;
    Color3f col = getEntityColor(obj->tio.entity);
    data.addArc(asVec3d(arc->center), arc->radius, arc->start_angle, arc->end_angle, col, layer);
}

void process_TEXT(Dwg_Object* obj, DWGContext& data) {
    Dwg_Entity_TEXT* text = obj->tio.entity->tio.TEXT;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);
#if LIBREDWG_VERSION_MINOR >= 11
    double x = text->ins_pt.x;
    double y = text->ins_pt.y;
#else
    double x = text->insertion_pt.x;
    double y = text->insertion_pt.y;
#endif
    char* t = text->text_value;
    data.addText(Vec3d(x,y,0), t?t:"", text->height, layer);
}

void process_MTEXT(Dwg_Object* obj, DWGContext& data) {
    Dwg_Entity_MTEXT* text = obj->tio.entity->tio.MTEXT;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);
#if LIBREDWG_VERSION_MINOR >= 11
    double x = text->ins_pt.x;
    double y = text->ins_pt.y;
#else
    double x = text->insertion_pt.x;
    double y = text->insertion_pt.y;
#endif
    char* t = text->text;
    data.addText(Vec3d(x,y,0), t?t:"", text->text_height, layer);
}

void process_INSERT(Dwg_Object* obj, DWGContext& data) {
    Dwg_Entity_INSERT* insert = obj->tio.entity->tio.INSERT;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);

    data.offset_stack.push_back( asVec3d(insert->ins_pt) );
    data.scale_stack.push_back( asVec3d(insert->scale) );
    data.extrusion_stack.push_back( asVec3d(insert->extrusion) );
    data.rot_angle_stack.push_back( insert->rotation );
    data.layer_stack.push_back( layer );
    data.compute_current_transformation();

    //Dwg_Object_BLOCK_HEADER* block = insert->block_header->obj->tio.object->tio.BLOCK_HEADER;
    process_BLOCK_HEADER(insert->block_header, data, false);

    data.offset_stack.pop_back();
    data.scale_stack.pop_back();
    data.extrusion_stack.pop_back();
    data.rot_angle_stack.pop_back();
    data.layer_stack.pop_back();
    data.compute_current_transformation();
}

void process_VIEWPORT(Dwg_Object* obj, DWGContext& data) {
    //Dwg_Entity_VIEWPORT* viewport = obj->tio.entity->tio.VIEWPORT;

    /*data.offset_stack.push_back( asVec3d(insert->ins_pt) );
    data.scale_stack.push_back( asVec3d(insert->scale) );
    data.extrusion_stack.push_back( asVec3d(insert->extrusion) );
    data.rot_angle_stack.push_back( insert->rotation );
    data.compute_current_transformation();

    //Dwg_Object_BLOCK_HEADER* block = insert->block_header->obj->tio.object->tio.BLOCK_HEADER;
    process_BLOCK_HEADER(insert->block_header, data, false);

    data.offset_stack.pop_back();
    data.scale_stack.pop_back();
    data.extrusion_stack.pop_back();
    data.rot_angle_stack.pop_back();
    data.compute_current_transformation();*/
}

void process_HATCH(Dwg_Object* obj, DWGContext& data) {;} // TODO
void process_ATTDEF(Dwg_Object* obj, DWGContext& data) {;} // TODO
void process_SOLID(Dwg_Object* obj, DWGContext& data) {;} // TODO
void process_LEADER(Dwg_Object* obj, DWGContext& data) {;} // TODO
void process_DIMENSION_RADIUS(Dwg_Object* obj, DWGContext& data) {;} // TODO
void process_MLINE(Dwg_Object* obj, DWGContext& data) {;} // TODO
void process_SPLINE(Dwg_Object* obj, DWGContext& data) {;} // TODO
void process_3DFACE(Dwg_Object* obj, DWGContext& data) {;} // TODO
void process_3DSOLID(Dwg_Object* obj, DWGContext& data) {;} // TODO
void process_ELLIPSE(Dwg_Object* obj, DWGContext& data) {;} // TODO
void process_DIMENSION_LINEAR(Dwg_Object* obj, DWGContext& data) {;} // TODO

int Nlines = 0;

void process_object(Dwg_Object* obj, DWGContext& data) {
    if (!obj) { cout << "Warning: process_object, object invalid!" << endl; return; }

    string dxfname = obj->dxfname ? obj->dxfname : "";
    data.entityHistogram[dxfname] += 1;

    //cout << "process_object " << obj->type << endl;

    switch (obj->type) {
        case DWG_TYPE_LINE: process_LINE(obj, data); break;
        case DWG_TYPE_INSERT: process_INSERT(obj, data); break;
        case DWG_TYPE_CIRCLE: process_CIRCLE(obj, data); break;
        case DWG_TYPE_TEXT: process_TEXT(obj, data); break;
        case DWG_TYPE_VIEWPORT: process_VIEWPORT(obj, data); break;
        case DWG_TYPE_MTEXT: process_MTEXT(obj, data); break;
        case DWG_TYPE_POLYLINE_2D: process_POLYLINE_2D(obj, data); break;
        case DWG_TYPE_LWPOLYLINE: process_LWPOLYLINE(obj, data); break;
        case DWG_TYPE_ARC: process_ARC(obj, data); break;
        case DWG_TYPE_HATCH: process_HATCH(obj, data); break;
        case DWG_TYPE_ATTDEF: process_ATTDEF(obj, data); break;
        case DWG_TYPE_POINT: process_POINT(obj, data); break;
        case DWG_TYPE_SOLID: process_SOLID(obj, data); break;
        case DWG_TYPE_LEADER: process_LEADER(obj, data); break;
        case DWG_TYPE_DIMENSION_RADIUS: process_DIMENSION_RADIUS(obj, data); break;
        case DWG_TYPE_MLINE: process_MLINE(obj, data); break;
        case DWG_TYPE_SPLINE: process_SPLINE(obj, data); break;
        case DWG_TYPE__3DFACE: process_3DFACE(obj, data); break;
        case DWG_TYPE__3DSOLID: process_3DSOLID(obj, data); break;
        case DWG_TYPE_ELLIPSE: process_ELLIPSE(obj, data); break;
        case DWG_TYPE_DIMENSION_LINEAR: process_DIMENSION_LINEAR(obj, data); break;
        default:
            string name = obj->name ? obj->name : "";
            cout << "process_object, unhandled type: " << dxfname << "   \t" << obj->type << endl;
            break;
    }
}

void process_BLOCK_HEADER(Dwg_Object_Ref* ref, DWGContext& data, bool onlyRoot) {
    if (!ref || !ref->obj) { /*cout << "Warning: process_BLOCK_HEADER, ref invalid!" << endl;*/ return; }

    //string name = ref->obj->name ? ref->obj->name : "";
    //string dxfname = ref->obj->dxfname ? ref->obj->dxfname : "";

    Dwg_Object_BLOCK_HEADER* block = ref->obj->tio.object->tio.BLOCK_HEADER;
    if (onlyRoot && block->num_inserts != 0) return;

    /*if (block->layout && block->layout->obj) {
        Dwg_Object_LAYOUT* layout = block->layout->obj->tio.object->tio.LAYOUT;
        layout->current_style_sheet
    }*/

    //process_BLOCK_HEADER(block, data);

    //if (block->num_inserts > 0) cout << " process_BLOCK_HEADER Ninserts: " << block->num_inserts << "   \tNowned: " << block->num_owned << endl;

    /*if (block->layout && block->layout->obj) {
        Dwg_Object_LAYOUT* layout = block->layout->obj->tio.object->tio.LAYOUT;

        double scale = layout->scale_factor;
        Vec3d origin = asVec3d(layout->ucs_origin);
        Vec3d axisX  = asVec3d(layout->ucs_x_axis);
        Vec3d axisY  = asVec3d(layout->ucs_y_axis);

        data.scale = scale;

        cout << " process_BLOCK_HEADER s: " << scale << "   \tO: " << origin << "   \tX: " << axisX << "   \tY: " << axisY << endl;
    } //else cout << " process_BLOCK_HEADER no block layout! " << endl;*/

    Dwg_Object* obj = get_first_owned_entity(ref->obj);
    while (obj) {
        process_object(obj, data);
        obj = get_next_owned_entity(ref->obj, obj);
    }
}

// block -> inserts
// block -> layout        <------- not usefull
//  layout -> viewports
//  layout -> active_viewport
//   viewport -> layers
//    layers -> entities

// block references (insert entities) are not yet exploded, UCS and paper space transformations per entity

void loadDWG(string path, VRTransformPtr res, map<string, string> options) {
    DWGContext data;
    Dwg_Data dwg;
    int r = dwg_read_file(path.c_str(), &dwg); // dxf_read_file(); // ---------------- !!!
    if (r != 0) { cout << "\n\nloadDWG failed!\n" << endl; } //return; }
    data.dwg = &dwg;

    auto root = VRTransform::create(path);

    /*for (int i = 0; i < dwg.num_objects; i++) {
        Dwg_Object& o = dwg.object[i];
        string type = o.dxfname;
        data.objectHistogram[type] += 1;
    }*/

    /*dwg.block_control;
    dwg.layer_control;
    dwg.style_control;
    dwg.ltype_control;
    dwg.view_control;
    dwg.ucs_control;
    dwg.vport_control;
    dwg.appid_control;
    dwg.dimstyle_control;
    dwg.vport_entity_control;*/

    Dwg_Object_BLOCK_CONTROL* block_control = &dwg.block_control;
    process_BLOCK_HEADER(dwg.header_vars.BLOCK_RECORD_MSPACE, data, true); // first all entities in the model space
    for (int i=0; i < block_control->num_entries; i++) { // then all entities in the blocks
        process_BLOCK_HEADER(block_control->entries[i], data, true);
    }
    process_BLOCK_HEADER(dwg.header_vars.BLOCK_RECORD_PSPACE, data, true); // and last all entities in the paper space

    bool doSplitByColors = false;
    if (options.count("doSplitByColors"))
        toValue(options["doSplitByColors"], doSplitByColors);

    cout << " got " << data.layers.size() << " layers" << endl;

    for (auto& l : data.layers) {
        Dwg_Object_LAYER* layer = l.first;
        DWGLayer& context = l.second;
        if (layer) if (!layer->on || layer->frozen) continue;

        context.root = VRTransform::create( getLayerName(layer, dwg) );
        if (context.ann) context.root->addChild(context.ann);
        root->addChild(context.root);

        auto mat = VRMaterial::create("mat");
        mat->setLineWidth(1);
        mat->setLit(0);
        mat->setDiffuse( getLayerColor(layer) );

        if (!doSplitByColors) {
            auto geo = context.geo.asGeometry( "primitives" );
            geo->setMaterial(mat);
            context.root->addChild(geo);
        } else {
            auto geos = context.geo.splitByVertexColors();
            for (auto geo : geos) {
                geo->setName("primitives");
                geo->getMaterial()->setLit(false);
                context.root->addChild(geo);
            }
        }
    }
    //dwg_free(&dwg); // writes a lot to console..

    map<string, int> hist;
    for (unsigned int i=0; i < dwg.num_objects; i++) {
        Dwg_Object& obj = dwg.object[i];
        if (obj.type == DWG_TYPE_LINE || obj.type == DWG_TYPE_INSERT) {
            Dwg_Object_LAYER* layer = dwg_get_entity_layer(obj.tio.entity);
            string name = getLayerName(layer, dwg);
        }
    }

    for (int i=0; i < dwg.layer_control.num_entries; i++) {
        Dwg_Object* obj = dwg.layer_control.entries[i]->obj;
        if (!obj || obj->type != DWG_TYPE_LAYER) continue;
        Dwg_Object_LAYER* layer = dwg.layer_control.entries[i]->obj->tio.object->tio.LAYER;
        string name = getLayerName(layer, dwg);
    }

    res->addChild(root);

    /*cout << "layer / entity historgam" << endl;
    for (auto h : hist) {
        cout << " " << h.first << " " << h.second << endl;
    }

    cout << "DWG entity historgam" << endl;
    for (auto e : data.entityHistogram) cout << " " << e.first << ": " << e.second << endl;
    cout << "DWG object historgam" << endl;
    for (auto e : data.objectHistogram) cout << " " << e.first << ": " << e.second << endl;*/
}

VRGeometryPtr dwgArcTest() {
    DWGContext data;
    float a = 6.28/10.0;
    data.layers[0] = DWGLayer();
    Color3f col;
    for (int i=-5; i<5; i++) data.addArc(Pnt3d(i, 0, 0), 0.4, 0, i*6.28/5.0, col, 0);
    for (int i=-5; i<5; i++) data.addArc(Pnt3d(i, 1, 0), 0.4, i*6.28/5.0, 0, col, 0);
    for (int i=-5; i<5; i++) data.addArc(Pnt3d(i, 2, 0), 0.4, i*6.28/5.0-a, i*6.28/5.0+a, col, 0);
    for (int i=-5; i<5; i++) data.addArc(Pnt3d(i, 3, 0), 0.4, i*6.28/5.0+a, i*6.28/5.0-a, col, 0);
    for (int i=-5; i<5; i++) data.addArc(Pnt3d(i, 4, 0), 0.4, i*6.28/5.0-a*3, i*6.28/5.0+a*3, col, 0);
    for (int i=-5; i<5; i++) data.addArc(Pnt3d(i, 5, 0), 0.4, i*6.28/5.0+a*3, i*6.28/5.0-a*3, col, 0);
    auto geo = data.layers[0].geo.asGeometry( "arcTest" );
    auto m = VRMaterial::create("arcMat");
    m->setLit(0);
    m->setDiffuse(Color3f(0,0.7,0.4));
    m->setLineWidth(3);
    geo->setMaterial(m);
    return geo;
}

void writeDWG(VRObjectPtr obj, string path) {
    auto plane = Pose::create(Vec3d(0,0,0), Vec3d(0,1,0), Vec3d(0,0,1));
    Layer2D projection;
    projection.project(obj, plane);
    for (auto l : projection.getLines()) {
        //drawLine(l.p1, l.p2, l.c1, l.c2);
    }

    Dwg_Data data;
    data.header.version = R_13; //R_2000;

    dwg_add_object(&data);
    Dwg_Object& lineObj = data.object[data.num_objects-1];
    dwg_setup_LINE(&lineObj);
    Dwg_Entity_LINE* line = lineObj.tio.entity->tio.LINE;

    BITCODE_3BD p1;
    BITCODE_3BD p2;
    p1.x = 0;
    p1.y = 0;
    p1.z = 0;
    p2.x = 10;
    p2.y = 10;
    p2.z = 10;
    line->start = p1;
    line->end   = p2;

    return; // TODO: for now this crashes

    dwg_write_file(path.c_str(), &data);
}

OSG_END_NAMESPACE;
