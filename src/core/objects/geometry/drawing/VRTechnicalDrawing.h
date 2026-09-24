#ifndef VRTECHNICALDRAWING_H_INCLUDED
#define VRTECHNICALDRAWING_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRDrawingFwd.h"
#include "core/objects/VRTransform.h"
#include "core/objects/material/VRMaterialFwd.h"

#include <OpenSG/OSGColor.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRTechnicalDrawing : public VRTransform {
    public:
        enum ObjectType {
            POINT = 0,
            LINE,
            QUAD,
            RECTANGLE,
            ARC,
            CIRCLE,
            ELLIPSE,
            POLYLINE,
            LABEL
        };

        enum ParameterType {
            POSITION = 0,
            POSITIONS,
            CENTER,
            RADIUS,
            BEGIN,
            END,
            SCALE,
            TEXT
        };

        struct Parameter {
            string data;
            ParameterType type;

            Parameter() {}
            Parameter(ParameterType type, void* src, size_t nBytes);
        };

        struct Object {
            size_t ID = 0;
            string name;
            ObjectType type;
            string style;
            map<ParameterType, Parameter> parameters;
            Matrix4d transform;

            vector<size_t> children;

            bool changed = true;

            Object() {}
            Object(string name, ObjectType type, string style);
        };

        struct Label {
            string text;
            string style;
            Matrix4d transform;
        };

        struct Layer {
            string name;
            VRTransformPtr root;
            map<string, VRAnnotationEnginePtr> annotationStyles;
            map<string, VRGeoDataPtr> geoAggregators;
            map<string, VRGeometryPtr> geometries;
            map<size_t, Object> objects;

            bool changed = true;

            Layer() {}
            Layer(string name);
        };

        struct Material {
            Color3f color;
        };

        struct Context {
            string layer;
            Matrix4d transform;
        };

	private:
	    Context context;
	    map<string, Material> materials;
	    map<string, Layer> layers;

	    VRMaterialPtr mat;

	    template<typename T> Parameter packParam(ParameterType type, T data);
	    template<typename T> Parameter packParam(ParameterType type, vector<T> data);
	    template<typename T> Parameter packParam(ParameterType type, string data);
	    template<typename T> void unpackParam(Parameter p, T& t);
	    template<typename T> void unpackParam(Parameter p, vector<T>& t);
	    template<typename T> void unpackParam(Parameter p, string& t);

		void updateGeometry(Layer& l, Object& o);

	public:
		VRTechnicalDrawing(string name);
		~VRTechnicalDrawing();

		static VRTechnicalDrawingPtr create(string name = "drawing");
		VRTechnicalDrawingPtr ptr();

		void updateGeometries();

		bool hasMaterial(string mID);
		Material getMaterial(string mID);

		void addLayer(string name);
		void addMaterial(string mID);
		void addMarkup(string name, VRAnnotationEnginePtr style, string layer);
		Object& addObject(string name, ObjectType type, string style);

        void setColor(string mID, Color3f col);

		void setActiveLayer(string layer);
		void setActiveTransform(Matrix4d transform);

		void addPoint(string name, Pnt2d p, string style);
		void addLine(string name, Pnt2d p1, Pnt2d p2, string style);
		void addQuad(string name, Pnt2d p1, Pnt2d p2, Pnt2d p3, Pnt2d p4, string style);
		void addArc(string name, Pnt2d c, double r, double b, double e, Vec2d s, string style);
		void addCircle(string name, Pnt2d cp, double r, string style);
		void addEllipse(string name, Pnt2d cp, double a, double b, string style);
		void addPolyLine(string name, vector<Pnt2d> v, string style);
		void addLabel(string name, Pnt2d p, string text, string style);

		vector<VRObjectPtr> getLayers();
};

OSG_END_NAMESPACE;

#endif //VRTECHNICALDRAWING_H_INCLUDED
