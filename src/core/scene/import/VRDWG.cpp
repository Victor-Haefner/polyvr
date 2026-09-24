
#include <codecvt>

#define restrict
#define template dwg_template
#define USE_WRITE
#include <dwg_api.h>
#undef template

#include "VRDWG.h"

#include "core/objects/geometry/drawing/VRTechnicalDrawing.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"
#include "core/math/partitioning/boundingbox.h"
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

double d2r = Pi/180.0;

Vec3d asVec3d(BITCODE_2BD& v) { return Vec3d(v.x,v.y,0.0); }
Vec3d asVec3d(BITCODE_2RD& v) { return Vec3d(v.x,v.y,0.0); }
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
    string filePath;

    Dwg_Data dwg;
    map<Dwg_Object_LAYER*, DWGLayer> layers;
    Vec3d offset;

    vector<Vec3d> offset_stack;
    vector<Vec3d> scale_stack;
    vector<Vec3d> extrusion_stack;
    vector<double> rot_angle_stack;
    vector<Dwg_Object_LAYER*> layer_stack;

    map<string, int> objectHistogram;
    map<string, int> entityHistogram;

    bool inInsert = false;
    Matrix4d transformation;
    double rot_angle = 0;


    // --- new structure
    VRTechnicalDrawingPtr drawing;
    // ---

    DWGContext() {
        drawing = VRTechnicalDrawing::create();
    }

    void compute_current_transformation() {
	    transformation.setIdentity();

		rot_angle = 0;
		for (double f : rot_angle_stack) rot_angle += f;

	    //if (offset_stack.size() > 0) cout << "compute_current_transformation " << offset_stack.size() << endl;
		for (uint i=0; i<offset_stack.size(); i++) {
			Vec3d o  = offset_stack[i];
			if (i == 0) o += offset;
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
			//cout << "   " << i << "  scale " << s << " pos " << o << "  extr " << e << "  ang " << f << endl;
		}
		//if (offset_stack.size() > 0) cout << endl;
	}

	double scaleLength(double l) {
        Vec3d s = Vec3d(1,0,0);
		transformation.mult(s,s);
		return l * s.length();
	}

    void addPoint(Pnt3d p, string style, Dwg_Object_LAYER* layer) {
        drawing->setActiveTransform( transformation );
        drawing->addPoint("", Pnt2d(p), style);
	}

	void addLine(Pnt3d vec1, Pnt3d vec2, string style, Dwg_Object_LAYER* layer) {
        drawing->setActiveTransform( transformation );
        drawing->addLine("", Pnt2d(vec1), Pnt2d(vec2), style);
	}

	void addQuad(Pnt3d vec1, Pnt3d vec2, Pnt3d vec3, Pnt3d vec4, string style, Dwg_Object_LAYER* layer) {
        drawing->setActiveTransform( transformation );
        drawing->addQuad("", Pnt2d(vec1), Pnt2d(vec2), Pnt2d(vec3), Pnt2d(vec4), style);
	}

    /** DWG arcs always rotate counterclockwise! */
	void addArc(Pnt3d c, double r, double a1, double a2, string style, Dwg_Object_LAYER* layer, Vec3d eBox = Vec3d(1,1,1)) {
		if (r < 1e-6) r = 1; // elipse -> use box
        drawing->setActiveTransform( transformation );
        drawing->addArc("", Pnt2d(c), r, a1, a2, Vec2d(eBox), style);
	}

	void addCircle(Vec3d c, double r, string style, Dwg_Object_LAYER* layer) {
		addArc( c, r, 0, 2*Pi, style, layer );
	}

	void addEllipse(Vec3d c, double a, double b, string style, Dwg_Object_LAYER* layer) {
		Vec3d box = Vec3d(a, b,0);
		transformation.mult(box, box);
		for (int i=0; i<3; i++) box[i] = abs(box[i]);
		addArc( c, 0, 0, 2*Pi, style, layer, box );
	}

	struct MString {
        string txt;
        string font;
        string A; // ???
        int line = 0;
	};

	vector<MString> parseMarkupString(string s) {
        vector<string> lines = splitString(s, "\\P");
        vector<MString> strings;
        for (size_t i=0; i<lines.size(); i++) {
            string& s = lines[i];
            MString ms;
            ms.line = i;

            if (s.size() > 0)
                if (s[0] == '{' && s[s.size()-1] == '}') s = subString(s, 1, s.size()-2);

            for (size_t j=0; j<s.size(); j++) {
                if (s[j] == '\\') {
                    if (s[j+1] == 'f') {
                        int scP = s.find(';', j);
                        int n = scP-j-2;
                        ms.font = s.substr(j+2, n);
                        j += n+2; continue;
                    }
                    if (s[j+1] == 'A') {
                        int scP = s.find(';', j);
                        int n = scP-j-2;
                        ms.A = s.substr(j+2, n);
                        j += n+2; continue;
                    }
                }

                ms.txt += s[j];
            }

            strings.push_back(ms);
        }
        return strings;
	}

    void addText(Vec3d p, Vec3d x, Vec2d box, string t, double height, Dwg_Object_LAYER* layer) {
        auto strings = parseMarkupString(t);

        float h = height*0.9;
        float w = h*0.6;
        float lD = h*0.7;

        string style = "txt_"+toString(h)+"_"+toString(x);
        drawing->setActiveTransform( transformation );

        size_t Nmax = 0;
        for (auto& ms : strings) Nmax = max(Nmax, ms.txt.size());

        Vec3d u = x.cross(Vec3d(0,0,-1));
        p += u * ( strings.size()-1 ) * (h+lD) * 0.5; // center vertically
        p -= x * Nmax * w * 0.5; // center horizontally
        p = Vec3d(x.dot(p),u.dot(p), 0);

        for (auto& ms : strings) {
            drawing->addLabel("", Pnt2d(p), ms.txt, style);
            p[1] -= h+lD;
        }
    }

	/*void addDWGArc(Vec3d c, double r, double a1, double a2, Dwg_Object_LAYER* layer) { // TODO: maybe usefull, transforms start and end points!
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
Vec3d transform_OCS(Vec3d pt, Vec3d ext, DWGContext& data, bool isDir = false) {
    Vec3d offset;
    if (!data.inInsert && !isDir) offset = data.offset;

    if (ext[0] == 0.0 && ext[1] == 0.0 && ext[2] ==  1.0) return pt +offset;
    if (ext[0] == 0.0 && ext[1] == 0.0 && ext[2] == -1.0) return Vec3d(-pt[0], pt[1], pt[2]) +offset;

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
    return Vec3d(pt.dot(ax), pt.dot(ay), pt.dot(az)) +offset;
}

Matrix4d setupTransform(Vec3d ext, DWGContext& data) {
    Matrix4d m;

    Vec3d offset;
    if (!data.inInsert) offset = data.offset;

    if (ext[0] == 0.0 && ext[1] == 0.0 && ext[2] == -1.0) m.setScale(-1,1,1);
    else {
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
        m[0] = Vec4d(ax[0], ay[0], az[0], 0);
        m[1] = Vec4d(ax[1], ay[1], az[1], 0);
        m[2] = Vec4d(ax[2], ay[2], az[2], 0);
    }

    m.setTranslate( offset );
    return m;
}

/*Dwg_Object_LAYER* dwg_get_entity_layer_safer (const Dwg_Object_Entity *ent) {
    if (!ent || !ent->layer || !ent->layer->obj || !ent->layer->obj->tio.object) return 0;
    return ent->layer->obj->tio.object->tio.LAYER;
}*/

Dwg_Object_LAYER* getEntityLayer(Dwg_Object* obj, DWGContext& data, bool checkName = true) {
    Dwg_Object_LAYER* layer = dwg_get_entity_layer(obj->tio.entity);
    if (!checkName) return layer;
    if (getLayerName(layer, data.dwg) == "0") { // wrong layer
        if (data.layer_stack.size() > 0) layer = data.layer_stack.back();
    }
    return layer;
}

void process_BLOCK_HEADER(Dwg_Object_Ref* ref, DWGContext& data, bool onlyRoot);

Color3f getEntityColor(dwg_obj_ent* ent) {
    int err;
    Dwg_Color* dcol = (Dwg_Color*)dwg_ent_get_color(ent, &err);
    /*cout << "process_LINE " << hev << " " << hfv << " " << huv << " mat: " << mat << endl;
    cout << " layer: " << getLayerName(layer, *data.dwg) << " " << getLayerColor(layer) << endl;

    printColor(layer->color);
    printColor(*dcol);*/
    return asColor3f(*dcol);
}


string process_Material(Dwg_Object* obj, DWGContext& data) {
    Dwg_Object_MATERIAL* mat = 0;
    if (obj->tio.entity->material && obj->tio.entity->material->obj) mat = obj->tio.entity->material->obj->tio.object->tio.MATERIAL;
    bool hev = obj->tio.entity->has_edge_visualstyle;
    bool hfv = obj->tio.entity->has_face_visualstyle;
    bool huv = obj->tio.entity->has_full_visualstyle;
    //if (mat || hev || hfv || huv) cout << "process_LINE " << hev << " " << hfv << " " << huv << " mat: " << mat << endl;

    Color3f col = getEntityColor(obj->tio.entity);
    string mID = "col_"+toString(col);
    if (!data.drawing->hasMaterial(mID)) {
        data.drawing->addMaterial(mID);
        data.drawing->setColor(mID, col);
    }
    return mID;
}

void process_POINT(Dwg_Object* obj, DWGContext& data) {
    string matID = process_Material(obj, data);
    Dwg_Entity_POINT* point = obj->tio.entity->tio.POINT;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);
    Pnt3d P = transform_OCS( Vec3d(point->x, point->y, point->z), asVec3d(point->extrusion), data );
    data.addPoint(P, matID, layer);
}

void process_LINE(Dwg_Object* obj, DWGContext& data) {
    Dwg_Entity_LINE* line = obj->tio.entity->tio.LINE;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);



    bool vis = !obj->tio.entity->invisible;
    if (!vis) return;

    string matID = process_Material(obj, data);

    Color3f col = getEntityColor(obj->tio.entity);
    Pnt3d P1 = transform_OCS( asVec3d(line->start), asVec3d(line->extrusion), data );
    Pnt3d P2 = transform_OCS( asVec3d(line->end  ), asVec3d(line->extrusion), data );
    data.addLine(P1, P2, matID, layer);
}

//convert the bulge of lwpolylines to arcs
void bulgeToArc(double bulge, Vec3d s, Vec3d e, Vec3d& cen, Vec3d& arc) {
	double cotbce = (1.0/bulge - bulge)*0.5;

	// Compute center point and radius
	Vec3d D = e-s;
	Vec3d T = Vec3d(-D[1], D[0], 0);
	if (abs(D[2]) > abs(D[0])) T = Vec3d(0, D[2], -D[1]);
	cen = (s + e + T*cotbce)*0.5;

	Vec3d Ls = s-cen;
	Vec3d Le = e-cen;
	double rad = Ls.length();

	Vec3d N = D.cross(T);
	N.normalize();

	// Compute start and end angles
	double sa = atan2(Ls[1], Ls[0]);
	double ea = atan2(Le[1], Le[0]);

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
    string matID = process_Material(obj, data);

    bool closed = line->flag & 1;
    bool curve_fit = line->flag & 2;
    bool spline_fit = line->flag & 4;
    bool is3d = line->flag & 8;
    bool is3dmesh = line->flag & 16;
    bool mesh_closed_in_n = line->flag & 32;
    bool polyface_mesh = line->flag & 64;
    bool ltype_continuous = line->flag & 128;

    int N = line->num_owned-1;
    if (closed) N++;

    for (int i=0; i<N; i++) {
        auto v1_2D = line->vertex[i]->obj->tio.entity->tio.VERTEX_2D;
        auto v2_2D = line->vertex[(i+1)%line->num_owned]->obj->tio.entity->tio.VERTEX_2D;
        BITCODE_3BD p1 = {v1_2D->point.x, v1_2D->point.y, 0};
        BITCODE_3BD p2 = {v2_2D->point.x, v2_2D->point.y, 0};
        Pnt3d P1 = transform_OCS( asVec3d(p1), asVec3d(line->extrusion), data );
        Pnt3d P2 = transform_OCS( asVec3d(p2), asVec3d(line->extrusion), data );

        double bulge = v1_2D->bulge;
        if (abs(bulge) < 1e-3) data.addLine(P1, P2, matID, layer);
        else {
            //if (extr) bulge *= -1;
            Vec3d cen, arc;
            bulgeToArc(bulge, Vec3d(P1), Vec3d(P2), cen, arc);
            data.addArc(Pnt3d(cen), arc[0], arc[1], arc[2], matID, layer);
        }
    }
}

void process_LWPOLYLINE(Dwg_Object* obj, DWGContext& data) {
    Dwg_Entity_LWPOLYLINE* line = obj->tio.entity->tio.LWPOLYLINE;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);
    string matID = process_Material(obj, data);

    bool vis = !obj->tio.entity->invisible;
    if (!vis) return;

    size_t N = line->num_points-1;
    if (line->flag & 1) N++; // closed

    for (size_t i=0; i<N; i++) {
        auto p1_2D = line->points[i];
        auto p2_2D = line->points[(i+1)%line->num_points];
        BITCODE_3BD p1 = {p1_2D.x, p1_2D.y, 0};
        BITCODE_3BD p2 = {p2_2D.x, p2_2D.y, 0};
        Pnt3d P1 = transform_OCS( asVec3d(p1), asVec3d(line->extrusion), data );
        Pnt3d P2 = transform_OCS( asVec3d(p2), asVec3d(line->extrusion), data );

        if (i < line->num_bulges) {
            double bulge = line->bulges[i];
            if (abs(bulge) < 1e-3) data.addLine(P1, P2, matID, layer);
            else {
                //if (extr) bulge *= -1;
                Vec3d cen, arc;
                bulgeToArc(bulge, Vec3d(P1), Vec3d(P2), cen, arc);
                data.addArc(Pnt3d(cen), arc[0], arc[1], arc[2], matID, layer);
            }
        } else data.addLine(P1, P2, matID, layer);
    }

    //line->vertexids;
    //line->num_vertexids;
}

void process_CIRCLE(Dwg_Object* obj, DWGContext& data) {
    Dwg_Entity_CIRCLE* circle = obj->tio.entity->tio.CIRCLE;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);
    bool vis = !obj->tio.entity->invisible;
    if (!vis) return;

    string matID = process_Material(obj, data);
    Vec3d center = transform_OCS( asVec3d(circle->center), asVec3d(circle->extrusion), data );
    data.addCircle(center, circle->radius, matID, layer);
}

void process_ARC(Dwg_Object* obj, DWGContext& data) {
    Dwg_Entity_ARC* arc = obj->tio.entity->tio.ARC;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);
    bool vis = !obj->tio.entity->invisible;
    if (!vis) return;

    string matID = process_Material(obj, data);
    float a1 = arc->start_angle;
    float a2 = arc->end_angle;
    Vec3d ext = asVec3d(arc->extrusion);
    if (ext[2] < 0) {
        a1 = Pi-a1;
        a2 = Pi-a2;
    }
    Pnt3d center = transform_OCS( asVec3d(arc->center), ext, data );
    data.addArc(center, arc->radius, a1, a2, matID, layer);
    //cout << "process_ARC " << center << ", ar " << arc->radius << ", as " << a1 << ", ae " << a2 << endl;
}

string convertText(void* ent, string entType, string fieldName) {
    char* text_value = nullptr;
    int isnew = 0;
    dwg_dynapi_entity_utf8text(ent, entType.c_str(), fieldName.c_str(), &text_value, &isnew, NULL);
    if (!text_value) return "";

    string txt = string(text_value);
    if (isnew) free(text_value);
    return txt;
}

void process_TEXT(Dwg_Object* obj, DWGContext& data) {
    Dwg_Entity_TEXT* text = obj->tio.entity->tio.TEXT;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);
    Vec3d tp;
#if LIBREDWG_VERSION_MINOR >= 11
    tp = asVec3d( text->ins_pt );
#else
    tp = asVec3d( text->insertion_pt );
#endif
    string txt = convertText(text, "TEXT", "text_value");
    Vec3d p = transform_OCS( tp, asVec3d(text->extrusion), data );
    data.addText(p, Vec3d(1,0,0), Vec2d(0,0), txt, text->height, layer);
}


void extract_binary_chunk(string dwg_filename, string output_filename, long address, size_t size) {
    FILE *dwg_file = fopen(dwg_filename.c_str(), "rb");
    FILE *output_file = fopen(output_filename.c_str(), "wb");
    fseek(dwg_file, address, SEEK_SET);

    char *buffer = (char *)malloc(size);
    size_t bytes_read = fread(buffer, 1, size, dwg_file);
    size_t bytes_written = fwrite(buffer, 1, bytes_read, output_file);

    // Cleanup
    free(buffer);
    fclose(dwg_file);
    fclose(output_file);

    printf("Binary chunk extracted successfully to %s\n", output_filename.c_str());
}

void process_MTEXT(Dwg_Object* obj, DWGContext& data) {
    Dwg_Entity_MTEXT* text = obj->tio.entity->tio.MTEXT;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);
    Vec3d tp, td;
#if LIBREDWG_VERSION_MINOR >= 11
    tp = asVec3d( text->ins_pt );
    td = asVec3d( text->x_axis_dir );
#else
    tp = asVec3d( text->insertion_pt );
    td = asVec3d( text->x_axis_dir );
#endif
    string txt = convertText(text, "MTEXT", "text");
    Vec3d p = transform_OCS( tp, asVec3d(text->extrusion), data );
    Vec3d d = transform_OCS( td, asVec3d(text->extrusion), data, true );
    Vec2d b = Vec2d( text->extents_width, text->extents_height );
    data.addText(p, d, b, txt, text->text_height, layer);

    /*if ( abs(tp[0]-3458135.236093999) < 1e-3 && abs(tp[1]-5439668.847597805) < 1e-3 ) {
        cout << " MTEXT " << p << " '" << txt << "' " << endl;
        cout << "  ins_pt: " << asVec3d(text->ins_pt) << endl;
        cout << "  extrusion: " << asVec3d(text->extrusion) << endl;
        cout << "  x_axis_dir: " << asVec3d(text->x_axis_dir) << endl;
        cout << "  rect_height: " << text->rect_height << endl;
        cout << "  rect_width: " << text->rect_width << endl;
        cout << "  text_height: " << text->text_height << endl;
        cout << "  attachment: " << text->attachment << endl;
        cout << "  flow_dir: " << text->flow_dir << endl;
        cout << "  extents_width: " << text->extents_width << endl;
        cout << "  extents_height: " << text->extents_height << endl;

        size_t a = obj->address;
        size_t s = obj->size*10;

        extract_binary_chunk(data.filePath, "dwgChunkMTEXT.bin", a, s);
    }*/
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
    data.inInsert = true;
    process_BLOCK_HEADER(insert->block_header, data, false);
    if (data.offset_stack.size() <= 1) data.inInsert = false;

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

void process_SOLID(Dwg_Object* obj, DWGContext& data) { // a filled quad
    Dwg_Entity_SOLID* solid = obj->tio.entity->tio.SOLID;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);

    Dwg_Object_MATERIAL* mat = 0;
    if (obj->tio.entity->material && obj->tio.entity->material->obj) mat = obj->tio.entity->material->obj->tio.object->tio.MATERIAL;
    bool hev = obj->tio.entity->has_edge_visualstyle;
    bool hfv = obj->tio.entity->has_face_visualstyle;
    bool huv = obj->tio.entity->has_full_visualstyle;
    if (mat || hev || hfv || huv) cout << "process_SOLID " << hev << " " << hfv << " " << huv << " mat: " << mat << endl;

    bool vis = !obj->tio.entity->invisible;
    if (!vis) return;

    string matID = process_Material(obj, data);
    Pnt3d P1 = transform_OCS( asVec3d(solid->corner1), asVec3d(solid->extrusion), data );
    Pnt3d P2 = transform_OCS( asVec3d(solid->corner2), asVec3d(solid->extrusion), data );
    Pnt3d P3 = transform_OCS( asVec3d(solid->corner3), asVec3d(solid->extrusion), data );
    Pnt3d P4 = transform_OCS( asVec3d(solid->corner4), asVec3d(solid->extrusion), data );
    data.addQuad(P1, P2, P3, P4, matID, layer);
}

void process_HATCH(Dwg_Object* obj, DWGContext& data) { // TODO: this is a fill, like a pattern
    Dwg_Entity_HATCH* solid = obj->tio.entity->tio.HATCH;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);

    bool vis = !obj->tio.entity->invisible;
    if (!vis) return;

    string matID = process_Material(obj, data);
    // TODO
}

void process_ATTDEF(Dwg_Object* obj, DWGContext& data) {;} // TODO
void process_LEADER(Dwg_Object* obj, DWGContext& data) {;} // TODO
void process_DIMENSION_RADIUS(Dwg_Object* obj, DWGContext& data) {;} // TODO
void process_MLINE(Dwg_Object* obj, DWGContext& data) {;} // TODO
void process_SPLINE(Dwg_Object* obj, DWGContext& data) {;} // TODO
void process_3DFACE(Dwg_Object* obj, DWGContext& data) {;} // TODO
void process_3DSOLID(Dwg_Object* obj, DWGContext& data) {;} // TODO
void process_ELLIPSE(Dwg_Object* obj, DWGContext& data) {;} // TODO

void process_DIMENSION_LINEAR(Dwg_Object* obj, DWGContext& data) { // TODO: not tested
    Dwg_Entity_DIMENSION_LINEAR* dimLin = obj->tio.entity->tio.DIMENSION_LINEAR;
    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);
    bool vis = !obj->tio.entity->invisible;
    if (!vis) return;

    string matID = process_Material(obj, data);

    Vec3d p1 = asVec3d(dimLin->xline1_pt);
    Vec3d p2 = asVec3d(dimLin->xline2_pt);

    Color3f col = getEntityColor(obj->tio.entity);
    Pnt3d P1 = transform_OCS( p1, asVec3d(dimLin->extrusion), data );
    Pnt3d P2 = transform_OCS( p2, asVec3d(dimLin->extrusion), data );
    data.addLine(P1, P2, matID, layer);

    Vec3d tp = (p1+p2)*0.5;
    Vec3d td = Vec3d(1,0,0);
    string txt = convertText(dimLin, "DIMENSION_LINEAR", "blockname");
    Vec3d p = transform_OCS( tp, asVec3d(dimLin->extrusion), data );
    Vec3d d = transform_OCS( td, asVec3d(dimLin->extrusion), data, true );
    data.addText(p, d, Vec2d(), txt, 0.5, layer);
}

int Nlines = 0;

void process_object(Dwg_Object* obj, DWGContext& data) {
    if (!obj) { cout << "Warning: process_object, object invalid!" << endl; return; }

    string dxfname = obj->dxfname ? obj->dxfname : "";
    data.entityHistogram[dxfname] += 1;

    //cout << "process_object " << obj->type << endl;

    Dwg_Object_LAYER* layer = getEntityLayer(obj, data);
    string lName = getLayerName(layer, data.dwg);
    data.drawing->setActiveLayer( lName );

    switch (obj->type) {
        case DWG_TYPE_POINT: process_POINT(obj, data); break;
        case DWG_TYPE_LINE: process_LINE(obj, data); break;
        case DWG_TYPE_CIRCLE: process_CIRCLE(obj, data); break;
        case DWG_TYPE_ARC: process_ARC(obj, data); break;
        //case DWG_TYPE_ELLIPSE: process_ELLIPSE(obj, data); break;
        case DWG_TYPE_POLYLINE_2D: process_POLYLINE_2D(obj, data); break;
        case DWG_TYPE_LWPOLYLINE: process_LWPOLYLINE(obj, data); break;
        case DWG_TYPE_SOLID: process_SOLID(obj, data); break;
        //case DWG_TYPE_VIEWPORT: process_VIEWPORT(obj, data); break;
        case DWG_TYPE_HATCH: process_HATCH(obj, data); break; // TODO
        /*case DWG_TYPE_ATTDEF: process_ATTDEF(obj, data); break;
        case DWG_TYPE_LEADER: process_LEADER(obj, data); break;
        case DWG_TYPE_DIMENSION_RADIUS: process_DIMENSION_RADIUS(obj, data); break;
        case DWG_TYPE_MLINE: process_MLINE(obj, data); break;
        case DWG_TYPE_SPLINE: process_SPLINE(obj, data); break;
        case DWG_TYPE__3DFACE: process_3DFACE(obj, data); break;
        case DWG_TYPE__3DSOLID: process_3DSOLID(obj, data); break;*/
        case DWG_TYPE_DIMENSION_LINEAR: process_DIMENSION_LINEAR(obj, data); break;
        case DWG_TYPE_TEXT: process_TEXT(obj, data); break;
        case DWG_TYPE_MTEXT: process_MTEXT(obj, data); break;
        case DWG_TYPE_INSERT: process_INSERT(obj, data); break;
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

void openDWGFile(string path, DWGContext& data) {
    data.filePath = path;
    int r = 0;
    string lpath = path; toLower(lpath);
    if (endsWith(lpath, "dwg")) r = dwg_read_file(path.c_str(), &data.dwg);
    if (endsWith(lpath, "dxf")) r = dxf_read_file(path.c_str(), &data.dwg);
    if (r != 0) { cout << "\n\nloadDWG failed!\n" << endl; }
}

void loadDWG(string path, VRTransformPtr res, map<string, string> options) {
    DWGContext data;
    if (options.count("offset")) toValue(options["offset"], data.offset);

    openDWGFile(path, data);

    size_t nLayers = dwg_get_layer_count( &data.dwg );
    auto layers = dwg_get_layers( &data.dwg );
    cout << " got " << nLayers << " layers" << endl;

    for (int i = 0; i < nLayers; i++) {
        Dwg_Object_LAYER* layer = layers[i];
        string name = getLayerName(layer, data.dwg);
        data.layers[layer] = DWGLayer();
        data.drawing->addLayer(name);
    }

    Dwg_Object_BLOCK_CONTROL* block_control = &data.dwg.block_control;
    process_BLOCK_HEADER(data.dwg.header_vars.BLOCK_RECORD_MSPACE, data, true); // first all entities in the model space
    for (int i=0; i < block_control->num_entries; i++) { // then all entities in the blocks
        process_BLOCK_HEADER(block_control->entries[i], data, true);
    }
    process_BLOCK_HEADER(data.dwg.header_vars.BLOCK_RECORD_PSPACE, data, true); // and last all entities in the paper space

    bool doSplitByColors = false;
    if (options.count("doSplitByColors")) toValue(options["doSplitByColors"], doSplitByColors);

    map<string, int> hist;
    for (unsigned int i=0; i < data.dwg.num_objects; i++) {
        Dwg_Object& obj = data.dwg.object[i];
        if (obj.type == DWG_TYPE_LINE || obj.type == DWG_TYPE_INSERT) {
            Dwg_Object_LAYER* layer = dwg_get_entity_layer(obj.tio.entity);
            string name = getLayerName(layer, data.dwg);
        }
    }

    for (int i=0; i < data.dwg.layer_control.num_entries; i++) {
        Dwg_Object* obj = data.dwg.layer_control.entries[i]->obj;
        if (!obj || obj->type != DWG_TYPE_LAYER) continue;
        Dwg_Object_LAYER* layer = data.dwg.layer_control.entries[i]->obj->tio.object->tio.LAYER;
        string name = getLayerName(layer, data.dwg);
    }

    data.drawing->updateGeometries();
    res->addChild( data.drawing );
}

VRGeometryPtr dwgArcTest() {
    DWGContext data;
    double a = 6.28/10.0;
    data.layers[0] = DWGLayer();
    string mID;
    for (int i=-5; i<5; i++) data.addArc(Pnt3d(i, 0, 0), 0.4, 0, i*6.28/5.0, mID, 0);
    for (int i=-5; i<5; i++) data.addArc(Pnt3d(i, 1, 0), 0.4, i*6.28/5.0, 0, mID, 0);
    for (int i=-5; i<5; i++) data.addArc(Pnt3d(i, 2, 0), 0.4, i*6.28/5.0-a, i*6.28/5.0+a, mID, 0);
    for (int i=-5; i<5; i++) data.addArc(Pnt3d(i, 3, 0), 0.4, i*6.28/5.0+a, i*6.28/5.0-a, mID, 0);
    for (int i=-5; i<5; i++) data.addArc(Pnt3d(i, 4, 0), 0.4, i*6.28/5.0-a*3, i*6.28/5.0+a*3, mID, 0);
    for (int i=-5; i<5; i++) data.addArc(Pnt3d(i, 5, 0), 0.4, i*6.28/5.0+a*3, i*6.28/5.0-a*3, mID, 0);
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
