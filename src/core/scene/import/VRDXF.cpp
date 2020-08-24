#include "VRDXF.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/utils/toString.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGQuaternion.h>

#include <algorithm>
#include <string>
#include <fstream>
#include <regex>

#define DODXF 1
#if DODXF == 1

OSG_BEGIN_NAMESPACE;
using namespace std;

string lower(string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

string strip(const string& str) {
    auto strBegin = str.find_first_not_of(" \t");
    if (strBegin == string::npos) return "";
    auto strEnd = str.find_last_not_of(" \t");
    return str.substr(strBegin, strEnd - strBegin + 1);
}

struct Object;
struct DXFLayer;

typedef map<string, vector<Object*>> DATA; // type is key

struct Object {
    bool block = false;
    bool visited_flag = false;
    DXFLayer* layer = 0;
    vector<int> extrusion;
    string type;
    string name;
    DATA data;

	Object(string type="", bool block=false) {
	    this->block = block;
	    this->type = type;
	}

	vector<Object*> get_type(string kind="") {
	    if (data.count(kind) == 0) return vector<Object*>();
		return data[kind];
	}

	vector<Object*> get_type(int kind = -1) {
	    return get_type(toString(kind));
	}

	float asFloat() {
	    cout << "asFloat " << name << " " << type << " " << block << " " << visited_flag << endl;
        return toFloat(name);
	}

	float asInt() {
	    cout << "asInt " << name << " " << type << " " << block << " " << visited_flag << endl;
        return toInt(name);
	}

	string asString() {
	    return name;
	}

	void add(Object* obj) {
        data[obj->type].push_back(obj);
	}

	void rem(Object* obj) {
        for (auto& d : data) {
            auto& vec = d.second;
            vec.erase(std::remove(vec.begin(), vec.end(), obj), vec.end());
        }
	}
};

Object* get_item(DATA& data, int type) {
    for (auto item : data) if (toInt(item.second[0]->type) == type) return item.second[0];
    return 0;
}

Object* get_name(DATA& data) { return get_item(data, 2); }
Object* get_layer(DATA& data) { return get_item(data, 8); }

Object* objectify(Object* obj);

struct Segment {
    Vec3d p1,p2;
    Segment(Vec3d p1, Vec3d p2) : p1(p1), p2(p2) { ; }
};

struct VRArc {
    Vec3d c;
    float r = 0;
    Vec2d a;
    Vec3d box;
    VRArc(Vec3d c, float r, float a1, float a2) : c(c), r(r), a(a1,a2) {;}
};

struct VRLayer {
    string name;
    vector<Vec3d> points;
    vector<Segment> lines;
    vector<VRArc> arcs;
    void addPoint( Vec3d p ) { points.push_back(p); }
    void addLine( Segment l ) { lines.push_back(l); }
    void addArc( VRArc a ) { arcs.push_back(a); }
    VRLayer(string name) { this->name = name; }
};

struct View {
    string name;
    map<string, VRLayer*> layers;
    View(string name) { this->name = name; }
};

struct base_parser {
    int N = 16;
    float rescale = 1;
    float rot_angle = 0;
    Matrix4d transformation;
    vector<Vec3d> offset_stack;
    vector<Vec3d> scale_stack;
    //vector<> rotation_stack;
    vector<float> rot_angle_stack;
    Vec2d last_controle_point;
    int element_id = 0;
    map<string, View*> viewports;
    map<string, Object*> elem_dict;
    VRLayer* layer;

//helper functions-------------------------------
	void compute_current_transformation() {
	    transformation.setIdentity();
	    transformation.setScale(rescale);
		for (uint i=0; i<offset_stack.size(); i++) {
			Vec3d o = offset_stack[i];
			Vec3d s = scale_stack[i];
			float f = rot_angle_stack[i];

			Matrix4d t,S;
			t.setRotate(Quaterniond(0,0,1,f));
			t.setTranslate(o);
			S.setScale(s);
			t.mult(S);
			transformation.mult(t);
		}

		rot_angle = 0;
		for (float f : rot_angle_stack) rot_angle += f;
	}

	//creates a layer if not present and returns it
	VRLayer* getLayer(string v_name, string l_name) {
		if (viewports.count(v_name) == 0) viewports[v_name] = new View(v_name);
		auto v = viewports[v_name];
		if (v->layers.count(l_name) == 0) v->layers[l_name] = new VRLayer(l_name);
		return v->layers[l_name];
	}

	void addPoint(Vec3d p) {
		transformation.mult(p,p);
		layer->addPoint(p);
	}

	void addLine(Vec3d vec1, Vec3d vec2) {
		transformation.mult(vec1, vec1);
		transformation.mult(vec2, vec2);
		layer->addLine( Segment(vec1,vec2) );
	}

	void addCircle(Vec3d c, float r) {
		transformation.mult(c,c);
		Vec3d s = Vec3d(1,0,0);
		transformation.mult(s,s);
		r *= s.length();
		layer->addArc( VRArc(c, r, 0, 2*Pi) );
	}

	void addEllipse(Vec3d c, float a, float b) {
		transformation.mult(c,c);
		Vec3d box = Vec3d(a, b,0);
		transformation.mult(box, box);
		for (int i=0; i<3; i++) box[i] = abs(box[i]);

		VRArc e(c, 1, 0, 2*Pi);
		e.box = box;
		layer->addArc(e);
	}

	void addArc(Vec3d c, float r, float a1, float a2, float bulge = 0) {
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

		layer->addArc( VRArc(c, r, ang[0], ang[1]) );
	}
};

/*void get_dxf_layer(data) {
	value = None
	for i, item in enumerate(data) {
		if item[0] == 8:
			value = item[1]
			break

	}

	return item, value, i
};*/

struct Primitive : public Object {
    string space;
    string color_index;
    vector<Vec3d> points;
    Vec3d ext;
    Vec3d scale;

    Primitive(Object* obj) : Object("") {
        if (obj == 0) return;
		name = obj->get_type("2")[0]->name;
        space = obj->get_type("67")[0]->name;
        color_index = obj->get_type("62")[0]->name;
        data = obj->data;
        type = obj->type;
        obj->visited_flag = true;
    }

	Vec3d get_vec3f(int i1, int i2, int i3) {
		Vec3d p;
		for (auto item : data) {
            int k = item.second[0]->asInt();
            float v = item.second[1]->asFloat();
			if (k == i1) p[0] = v;
			if (k == i2) p[1] = v;
			if (k == i3) p[2] = v;
		}
		return p;
	}

	Vec4d get_vec4f(int i1, int i2, int i3, int i4) {
		Vec4d p;
		for (auto item : data) {
            int k = item.second[0]->asInt();
            float v = item.second[1]->asFloat();
			if (k == i1) p[0] = v;
			if (k == i2) p[1] = v;
			if (k == i3) p[2] = v;
			if (k == i3) p[3] = v;
		}
		return p;
	}

	Vec4i get_vec4i(int i1, int i2, int i3, int i4) {
		Vec4i p;
		for (auto item : data) {
            int k = item.second[0]->asInt();
            int v = item.second[1]->asInt();
			if (k == i1) p[0] = v;
			if (k == i2) p[1] = v;
			if (k == i3) p[2] = v;
			if (k == i3) p[3] = v;
		}
		return p;
	}

	void get_p1() { Vec3d p = get_vec3f(10,20,30); points.push_back(p); }
	void get_p2() { Vec3d p = get_vec3f(11,21,31); points.push_back(p); }
	void get_extrusion() { ext = get_vec3f(210,220,230); }
	void get_scale() { scale = get_vec3f(41,42,43); }
};

struct DXFLine : public Primitive {
	DXFLine(Object* obj) : Primitive(obj) {
	    get_p1();
	    get_p2();
    }
};

struct DXFCircle : public Primitive {
    float radius = 0;

	DXFCircle(Object* obj) : Primitive(obj) {
		radius = obj->get_type(40)[0]->asFloat();
		get_p1();
		get_extrusion();
	}
};

struct DXFArc : public DXFCircle {
    float start_angle;
    float end_angle;

	DXFArc(Object* obj) : DXFCircle(obj) {
		start_angle = obj->get_type(50)[0]->asFloat();
		end_angle = obj->get_type(51)[0]->asFloat();
	}
};

struct DXFEllipse : public DXFArc {
	DXFEllipse(Object* obj) : DXFArc(obj) {
	    get_p2();
	    Vec3d m = points[1];
		radius = m.length();
	}
};

struct DXFLayer : public Primitive {
    string flags;
    bool frozen = false;

	DXFLayer(Object* obj) : Primitive(obj) {
		flags = obj->get_type(70)[0]->asString();
		frozen = (int(flags[0]) == 1); // byte coded, 1 = frozen
	}
};

struct BlockRecord : public Primitive {
    string insertion_units;
    string insert_units;

	BlockRecord(Object* obj) : Primitive(obj) {
		insertion_units = obj->get_type(70)[0]->asString();
		insert_units = obj->get_type(1070)[0]->asString();
	}
};

struct Block : public Primitive {
    string flags;
    string path;
    string description;

	Block(Object* obj) : Primitive(obj) {
		flags = obj->get_type(70)[0]->asString();
		data["entities"].push_back( objectify(obj) );
		name = obj->get_type(3)[0]->asString(); // else (2)
		path = obj->get_type(1)[0]->asString();
		description = obj->get_type(4)[0]->asString();
		get_p1();
	}
};

struct Insert : public Primitive {
    int block;
    float rotation;
    Vec4i table;

	Insert(Object* obj) : Primitive(obj) {
		block = toInt(name);
		rotation = obj->get_type(50)[0]->asFloat();
		table = get_vec4i(70,71,44,45); // rows, rspace, columns, cspace
        get_p1();
		get_extrusion();
		get_scale();
	}
};

struct Vertex : public Primitive {
    Vec3d loc;
    float bulge = 0;
    float swidth = 0;
    float ewidth = 0;
    string flags;

	Vertex(Object* obj = 0) : Primitive(obj) {
		if (obj == 0) return;
		loc = get_vec3f(10,20,30);
		swidth = obj->get_type(40)[0]->asFloat();
		ewidth = obj->get_type(41)[0]->asFloat();
		bulge = obj->get_type(42)[0]->asFloat();
		flags = obj->get_type(70)[0]->asString();
	}
};

struct Polyline : public Primitive {
    string elevation;
    string flags;
    bool closed = false;

	Polyline(Object* obj) : Primitive(obj) {
		elevation = obj->get_type(30)[0]->asString();
		flags = obj->get_type(70)[0]->asString();
		closed = int(flags[0]) == 1; // byte coded, 1 = closed, 128 = plinegen
        get_extrusion();
	}
};

struct LWpolyline : public Polyline {
    int num_points = 0;
    string elevation;
    string flags;
    bool closed = false;
    vector<Vertex*> points;

	LWpolyline(Object* obj) : Polyline(obj) {
		num_points = obj->get_type(90)[0]->asInt();
		elevation = obj->get_type(38)[0]->asString();
		get_points();
	}

	void get_points() {
		Vertex* v = new Vertex();
		for (auto item : data) {
            int k = item.second[0]->asInt();
            float f = item.second[1]->asFloat();
			if (k == 10) v->loc[0] = f;
			if (k == 20) v->loc[1] = f;
			if (k == 30) v->loc[2] = f;
			if (k == 40) v->swidth = f;
			if (k == 41) v->ewidth = f;
			if (k == 42) { v->bulge = f; points.push_back(v); }
		}
	}
};

struct Face : public Primitive {
    vector<Vec3d> points;

	Face(Object* obj) : Primitive(obj) {
        get_points();
	}

	void get_points() {
		Vec3d a,b,c,d;
		for (auto item : data) {
            int k = item.second[0]->asInt();
            float v = item.second[1]->asFloat();
			if (k == 10) a[0] = v;
			if (k == 20) a[1] = v;
			if (k == 30) { a[2] = v; points.push_back(a); }
			if (k == 11) b[0] = v;
			if (k == 21) b[1] = v;
			if (k == 31) { b[2] = v; points.push_back(b); }
			if (k == 12) c[0] = v;
			if (k == 22) c[1] = v;
			if (k == 32) { c[2] = v; points.push_back(c); }
			if (k == 13) d[0] = v;
			if (k == 23) d[1] = v;
			if (k == 33) { d[2] = v; points.push_back(d); }
        }
	}
};

Object* findObject(ifstream& infile, string kind="") {
	Object* obj = 0;
	string line;
	while(getline(infile,line)) {
		string data = strip(lower(line));
		if (obj == 0 && data == "0") obj = (Object*)1; // We"re still looking for our object code
		if (obj == (Object*)1) { // we are in an object definition
			if (data == kind) { obj = new Object(data); break; }
			if (!regex_match(data, regex(".*\\d.*"))) { obj = new Object(data); break; }
			obj = 0;
		}
	}

	return obj;
}

Object* handleObject(ifstream& infile) {
	// Add data to an object until end of object is found.
	string line;
	getline(infile,line);
	string stripped = strip(lower(line));
	if (stripped == "section") return new Object("section"); // this would be a problem
	else if (stripped == "endsec") return new Object("endsec"); // this means we are done with a section
	else { // add data to the object until we find a new object
		auto obj = new Object(stripped);
		obj->name = obj->type;
		bool done = false;
		vector<Object*> data;
		while (!done) {
			getline(infile, line);
			stripped = strip(lower(line));
			if (data.size() == 0) {
				if (stripped == "0") return obj; //we"ve found an object, time to return
				else data.push_back(new Object(stripped)); // first part is always an int
			} else {
				data.push_back(new Object(stripped));
				for (auto& d : data) obj->add(d);
				data = vector<Object*>();
			}
		}
	}
	return 0;
}

Object* handleTable(Object* table, ifstream& infile) {
	// Special handler for dealing with nested table objects.
	auto item = get_name(table->data);
	if (item) { // We should always find a name
		table->rem(item);
		table->name = lower(item->name);
	}
	// This next bit is from handleObject
	// handleObject should be generalized to work with any section like object
	while (true) {
		auto obj = handleObject(infile);
		if (obj->type == "table") {
			cout << "Warning: previous table not closed!\n";
			return table;
		}
		else if (obj->type == "endtab") return table; // this means we are done with the table
		else table->add(obj); // add objects to the table until one of the above is found
	}
}


Object* handleBlock(Object* block, ifstream& infile) {
	// Special handler for dealing with nested table objects.
	auto item = get_name(block->data);
	if (item) { // We should always find a name
		block->rem(item);
		block->name = lower(item->name);
	}
	// This next bit is from handleObject
	// handleObject should be generalized to work with any section like object
	while (true) {
		auto obj = handleObject(infile);
		if (obj->type == "block") {
			cout <<  "Warning: previous block not closed!\n";
			return block;
        }
		else if (obj->type == "endblk") return block; // this means we are done with the table
		else block->add(obj); // add objects to the table until one of the above is found
    }
}

struct StateMachine {
    ifstream infile;
    Object* section;
    Object* drawing;

	StateMachine(string path) : infile(path) {
	    drawing = new Object("drawing");
        section = findObject(infile, "section");
        if (section) start_section();
    }

    ~StateMachine() { infile.close(); }

    void end_section() {
        section = findObject(infile, "section");
        if (section) start_section();
    }

    void start_section() {
        bool done = false;
        vector<Object*> data;
        while (!done) {
            string line;
            getline(infile,line);
            string stripped = strip(lower(line));

            if (data.size() == 0) { // if we haven"t found a dxf code yet
                if (stripped == "0") { // we've found an object
                    while (true) { // no way out unless we find an end section or a new section
                        Object* obj = handleObject(infile);
                        if (obj->type == "section") { // shouldn"t happen
                            cout << "Warning: failed to close previous section!\n";
                            end_section();
                            break;
                        } else if (obj->type == "endsec") { // This section is over, look for the next
                            drawing->add(section);
                            end_section();
                            break;
                        } else if (obj->type == "table") { // tables are collections of data
                            obj = handleTable(obj, infile); // we need to find all there contents
                            section->add(obj); // before moving on
                        } else if (obj->type == "block") { // the same is true of blocks
                            obj = handleBlock(obj, infile); // we need to find all there contents
                            section->add(obj); // before moving on
                        } else section->add(obj); // found another sub-object
                    }
                } else data.push_back(new Object(stripped));
            } else { // we have our code, now we just need to convert the data and add it to our list.
                data.push_back(new Object(stripped));
                for (auto d : data) section->add(d);
                data.clear();
            }
        }
    }
};

//convert the bulge of lwpolylines to arcs
void cvtbulge(float bulge, Vec3d s, Vec3d e, Vec3d& cen, Vec3d& arc) {
	float cotbce = (1.0/bulge - bulge)*0.5;

	// Compute center point and radius
	cen = s+e+Vec3d((s[1]-e[1])*cotbce, (e[0]-s[0])*cotbce, 0)*0.5;
	float rad = (cen-s).length();

	// Compute start and end angles
	float sa = atan2(s[1] - cen[1], s[0] - cen[0]);
	float ea = atan2(e[1] - cen[1], s[1] - cen[0]);

	//check new start and end point
	float c_x2 = cos(ea)*rad + cen[0];
	float c_y2 = sin(ea)*rad + cen[1];
	if (abs(c_x2 - e[0]) > 0.001) cout << "error in cvtbulge, x";
	if (abs(c_y2 - e[1]) > 0.001) cout << "error in cvtbulge, y";

	while (sa < 0.0) sa += 2.0*Pi; // Eliminate negative angles // does nothing
	while (ea < 0.0) ea += 2.0*Pi;
	if (bulge < 0.0) swap(ea,sa); // Swap angles if clockwise // works

	arc = Vec3d(rad, sa, ea);
}

struct dxf_parser : public base_parser {
    string viewport = "Default";

    dxf_parser() {
        rescale = 0.8;
		compute_current_transformation();
    }

	void parse_rec(Object* item, Object* drawing, bool first = false) {
		if (item->type == "viewport" || item->type == "vport") {
			viewport = item->name;
			getLayer(viewport, layer->name);
		}

		if (item->layer) layer = getLayer(viewport, item->layer->name);

		float d2r = Pi/180.0;

		//extrusion hack :/
		bool extr = false;
		if (item->extrusion.size() >= 3 && item->extrusion[2] == -1) {
            extr = true;
            offset_stack.push_back(Vec3d(0,0,0));
            scale_stack.push_back(Vec3d(-1,1,1));
            rot_angle_stack.push_back(0);
            compute_current_transformation();
		}

		if (item->type == "line") if (auto l = (DXFLine*)item) addLine(l->points[0], l->points[1]);
		if (item->type == "circle") if (auto l = (DXFCircle*)item) addCircle(l->points[0], l->radius);
		if (item->type == "ellipse") if (auto l = (DXFEllipse*)item) addEllipse(l->points[0], l->points[1][0], l->points[1][1]);
		if (item->type == "arc") if (auto l = (DXFArc*)item) addArc(l->points[0], l->radius, l->start_angle*d2r, l->end_angle*d2r);

		if (item->type == "lwpolyline") {
            auto l = (LWpolyline*)item;
            int pN = l->points.size();
			for (int i=0; i<pN; i++) {
				Vertex* p1 = l->points[i];
				Vertex* p2 = l->points[(i+1)%pN];
				if (i < pN-1 || l->closed) {
					if (abs(p1->bulge) < 1e-4) addLine(p1->loc, p2->loc);
					else {
						if (extr) p1->bulge *= -1;
						Vec3d cen;
						Vec3d arc;
						cvtbulge(p1->bulge, p1->loc, p2->loc, cen, arc);
						addArc(cen, arc[0], arc[1], arc[2]);
					}
				}
			}
		}

		if (item->type == "polyline") {
            auto l = (Polyline*)item;
            int pN = l->points.size();
			for (int i=0; i<pN; i++) {
				Vec3d p1 = l->points[i];
				Vec3d p2 = l->points[(i+1)%pN];
				if (i < pN-1 || l->closed) addLine(p1,p2);
			}
		}

		if (item->type == "3dface") {
            auto f = (Face*)item;
            int pN = f->points.size();
			for (int i=0; i<pN; i++) {
				Vec3d p1 = f->points[i];
				Vec3d p2 = f->points[(i+1)%pN];
                addLine(p1,p2);
			}
		}

		if (item->type == "insert") { // transformation
            auto i = (Insert*)item;
			auto b = drawing->data["block"][i->block]; // get block
			if (!b) return;
            auto block = (Block*)b;

			offset_stack.push_back(i->points[0]);
			scale_stack.push_back(Vec3d(i->scale[0],i->scale[1],1));
			rot_angle_stack.push_back(i->rotation*d2r);
			compute_current_transformation();

			for (auto child : block->data["entities"]) parse_rec(child, drawing);

			offset_stack.pop_back();
			scale_stack.pop_back();
			rot_angle_stack.pop_back();
			compute_current_transformation();
		}

		if (extr) {
			offset_stack.pop_back();
			scale_stack.pop_back();
			rot_angle_stack.pop_back();
			compute_current_transformation();
		}
    }

	Object* parse(string path) {
        StateMachine sm(path);
        auto& drawing = sm.drawing;
        if (!drawing) return 0;

        drawing->name = path;
        for (auto childMap : drawing->data) {
            for (auto obj : childMap.second) {
                auto item = get_name(obj->data);
                if (!item) continue;
                obj->rem(item);
                obj->name = lower(item->name);
                drawing->add(obj);
                obj = objectify(obj);
            }
        }

        for (auto child : drawing->data["entities"]) parse_rec(child, drawing, true);
        return drawing;
	}
};

Object* objectify(Object* obj) {
    Object* objects = new Object("objects");
	for (auto& dataItr : obj->data) {
	    auto& objs = dataItr.second;
        for (uint i=0; i<objs.size(); i++) {
            auto& item = objs[i];

            if (item->type == "list") continue;

            if (item->type == "line") { objects->add(new DXFLine(item)); continue; }
            if (item->type == "lwpolyline") { objects->add(new LWpolyline(item)); continue; }
            if (item->type == "circle") { objects->add(new DXFCircle(item)); continue; }
            if (item->type == "arc") { objects->add(new DXFArc(item)); continue; }
            if (item->type == "layer") { objects->add(new DXFLayer(item)); continue; }
            if (item->type == "3dface") { objects->add(new Face(item)); continue; }
            if (item->type == "ellipse") { objects->add(new DXFEllipse(item)); continue; }
            if (item->type == "block_record") { objects->add(new BlockRecord(item)); continue; }
            if (item->type == "block") { objects->add(new Block(item)); continue; }
            if (item->type == "insert") { objects->add(new Insert(item)); continue; }

            if (item->type == "table") {
                item = objectify(item); // tables have sub-objects
                objects->add(item);
                continue;
            }

            if (item->type == "polyline") {
                auto pline = new Polyline(item);
                while (objs[i]->type != "seqend") {
                    i++;
                    if (objs[i]->type != "vertex") break;
                    auto v = new Vertex(objs[i]);
                    if (v->loc[0] != 0) pline->points.push_back(v->loc);
                }

                objects->add(pline);
                continue;
            }

            objects->add(item);
		}
    }
	return objects;
}

// DXF PARSER-----------

void loadDXF(string path, VRTransformPtr res) { // TODO
    dxf_parser parser;
    auto drawing = parser.parse(path);
    if (!drawing) return;

    cout << "loadDXF results:\n";
    for (auto dMap : drawing->data) {
        cout << " type: " << dMap.first << endl;
    }

    //res->addChild( geo->asGeometry(path) );
}

OSG_END_NAMESPACE;

#endif
