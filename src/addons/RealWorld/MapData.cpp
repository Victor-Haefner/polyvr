#include "MapData.h"

using namespace OSG;

MapData* MapData::diff(MapData* mapData) {
    return this;
//	var data = {};
//	data.bounds = this.bounds;
//
//	data.joints = [];
//	for (var i = 0; i < this.joints.length; i++) {
//		var id = this.joints[i].id;
//		var onlyInThis = true;
//		for (var j = 0; j < mapData.joints.length; j++) {
//			if (id === mapData.joints[j].id) onlyInThis = false;
//		};
//		if (onlyInThis) data.joints.push(this.joints[i]);
//	};
//
//	data.segments = [];
//	for (var i = 0; i < this.segments.length; i++) {
//		var id = this.segments[i].id;
//		var onlyInThis = true;
//		for (var j = 0; j < mapData.segments.length; j++) {
//			if (id === mapData.segments[j].id) onlyInThis = false;
//		};
//		if (onlyInThis) data.segments.push(this.segments[i]);
//	};
//
//	data.buildings = [];
//	for (var i = 0; i < this.buildings.length; i++) {
//		var id = this.buildings[i].id;
//		var onlyInThis = true;
//		for (var j = 0; j < mapData.buildings.length; j++) {
//			if (id === mapData.buildings[j].id) onlyInThis = false;
//		};
//		if (onlyInThis) data.buildings.push(this.buildings[i]);
//	};
//
//	return data;
}
