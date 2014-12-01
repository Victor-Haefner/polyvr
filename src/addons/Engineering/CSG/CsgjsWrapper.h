/*
 * CsgjsWrapper.h
 *
 *  Created on: Dec 17, 2013
 *      Author: marcel
 */

#ifndef CSGJSWRAPPER_H_
#define CSGJSWRAPPER_H_

#include "OpenSG/OSGConfig.h" // Namespace defines
#include "core/objects/geometry/VRGeometry.h"
#include "csgjs.h"

OSG_BEGIN_NAMESPACE;

namespace CSGApp
{

class CsgjsWrapper
{
public:
	CsgjsWrapper(csgjs_model model);
	virtual ~CsgjsWrapper();
	static CsgjsWrapper *createSphere(float radius, int slices, int stacks);
	static CsgjsWrapper *createBox(int size);
	void setModel(csgjs_model model);
	csgjs_model *model();
	GeometryTransitPtr toOsgGeometry();

private:
	csgjs_model _model;
};

}

OSG_END_NAMESPACE;

#endif /* CSGJSWRAPPER_H_ */
