#include "VRAMLLoader.h"
#include "core/utils/toString.h"

#include <iostream>
#include <libxml++/libxml++.h>
#include <OpenSG/OSGVector.h>

using namespace OSG;

template<> string typeName(const VRAMLLoader& t) { return "VRAMLLoader"; }

VRAMLLoader::VRAMLLoader() {}
VRAMLLoader::~VRAMLLoader() {}

VRAMLLoaderPtr VRAMLLoader::create()  { return VRAMLLoaderPtr(new VRAMLLoader()); }

void traverseXML(xmlpp::Element* e, string D = "") {
    cout << D << e->get_name() << endl;
    for (auto enode : e->get_children()) {
        auto element = dynamic_cast<xmlpp::Element*>(enode);
        if (!element) continue;
        traverseXML(element, D + " ");
    }
}

xmlpp::Element* getChild(xmlpp::Element* e, string name) {
    return dynamic_cast<xmlpp::Element*>( e->get_first_child( name ) );
}

vector<xmlpp::Element*> getChildren(xmlpp::Element* e, string name) {
    vector<xmlpp::Element*> res;
    for (auto n : e->get_children(name)) {
        auto e = dynamic_cast<xmlpp::Element*>( n );
        if (e) res.push_back(e);
    }
    return res;
}

void VRAMLLoader::read(string path) {
    string ns = "{http://www.dke.de/CAEX}";

    xmlpp::DomParser parser;
    try { parser.parse_file(path); }
    catch(const exception& ex) { cout << "VRAMLLoader::read Error: " << ex.what() << endl; return; }

    auto root = parser.get_document()->get_root_node();
    auto hierarchy = getChild(root, "InstanceHierarchy");

    for (auto mach : getChildren(hierarchy, "InternalElement")) {
        double x, y, z, length, width, height;
        double dx, dy, dz, ux, uy, uz;
        x = y = z = length = width = height = 0;
        dx = dy = dz = ux = uy = uz = 0;

        double mspeed = 1;
        vector<Vec3d> wSpace(2); //[[position],[size]]
        string path;
        string typ = "M";
        double clear = 10;
        string mid = mach->get_attribute_value("ID");
        string mname = mach->get_attribute_value("Name");
    }

    /*

		for mach in internalElements:

			x = y = z = length = width = height = 0
			dx = dy = dz = ux = uy = uz = 0
			mspeed = 1
			wSpace = [[0,0,0],[0,0,0]] #[[position],[size]]
			path = ''
			typ = 'M'
			clear = 10
			mid = mach.get('ID')
			mname = mach.get('Name')

			#Get basic information about the entities
			attributes = mach.findall(ns + 'Attribute')
			for attr in attributes:
				n = attr.get('Name')

				if n == 'position':
					#The attribute contains 3 sub-attributes
					values = attr.findall(ns + 'Attribute')
					for val in values:
						vn = val.get('Name')
						vf = val.find(ns + 'Value')
						if vf is None: break #Attribute "Value" missing: Keep default value.
						elif vn == 'x': x = float(vf.text)
						elif vn == 'y': y = float(vf.text)
						elif vn == 'z': z = float(vf.text)

				elif n == 'orientation':
					vecs = attr.findall(ns + 'Attribute')
					for vec in vecs:
						vn = vec.get('Name')
						if vn == 'direction_vector':
							values = vec.findall(ns + 'Attribute')
							for val in values:
								vnn = val.get('Name')
								vf = val.find(ns + 'Value')
								if vf is None: break
								elif vnn == 'dx': dy = float(vf.text)
								elif vnn == 'dy': dx = float(vf.text)
								elif vnn == 'dz': dz = float(vf.text)

						elif vn == 'up_vector':
							values = vec.findall(ns + 'Attribute')
							for val in values:
								vnn = val.get('Name')
								vf = val.find(ns + 'Value')
								if vf is None: break
								elif vnn == 'ux': ux = float(vf.text)
								elif vnn == 'uy': uy = float(vf.text)
								elif vnn == 'uz': uz = float(vf.text)

				elif n == 'model':
					vf = attr.find(ns + 'Value')
					if vf is not None and vf.text is not None:
						path = vf.text

				elif n == 'type':
					vf = attr.find(ns + 'Value')
					if vf is not None and vf.text is not None:
						typ = vf.text

				elif n == 'clearance':
					vf = attr.find(ns + 'Value')
					if vf is not None and vf.text is not None:
						clear = float(vf.text)

				elif n == 'size':
					values = attr.findall(ns + 'Attribute')
					for val in values:
						vn = val.get('Name')
						vf = val.find(ns + 'Value')
						if vf is None: break
						elif vn == 'length': length = float(vf.text)
						elif vn == 'height': height = float(vf.text)
						elif vn == 'width': width = float(vf.text)

				elif n == 'workspace':
					values = attr.findall(ns + 'Attribute')
					for val in values:
						vn = val.get('Name')
						if vn == 'position':
							subvalues = val.findall(ns + 'Attribute')
							for sval in subvalues:
								vnn = sval.get('Name')
								vff = sval.find(ns + 'Value')
								if vff is None: continue
								elif vnn == 'x': wSpace[0][0] = float(vff.text)
								elif vnn == 'y': wSpace[0][1] = float(vff.text)
								elif vnn == 'z': wSpace[0][2] = float(vff.text)
						elif vn == 'size':
							subvalues = val.findall(ns + 'Attribute')
							for sval in subvalues:
								vnn = sval.get('Name')
								vff = sval.find(ns + 'Value')
								if vff is None: continue
								elif vnn == 'length': wSpace[1][0] = float(vff.text)
								elif vnn == 'height': wSpace[1][1] = float(vff.text)
								elif vnn == 'width': wSpace[1][2] = float(vff.text)

			if typ == 'G':
				VR.setground(length, width)
				continue #Entity is the ground, nothing more to do. Take next entity.

			xmlProcesses = mach.findall(ns + 'InternalElement')
			processes = []
			for proc in xmlProcesses:
				procSpeed = 0
				doConsume = True
				n = proc.get('Name')
				procID = proc.get('ID')
				attributes = proc.findall(ns + 'Attribute')
				for attr in attributes:
					vn = attr.get('Name')
					if vn == 'speed':
						vf = attr.find(ns + 'Value')
						if vf is not None and vf.text is not None:
							procSpeed = float(vf.text)
					if vn == 'consume':
						vf = attr.find(ns + 'Value')
						if vf is not None and vf.text is not None:
							doConsume = bool(int(vf.text))

				p = VR.Machine.Process(None, n, procID, procSpeed, doConsume)
				processes.append(p)

				#Get a processes interfaces
				#externalInterfaces contains XML-elements of the current entities interfaces
				#interfaces contains Interface-elements that has been translated from its corresponding XML-element
				#allInterfaces contains all Interface-elements of all entities for future use (Build graph with InternalLinks)
				externalInterfaces = []
				if typ != 'R' and typ != 'O': #Ressources, Obstacles arent allowed to have interfaces.
					externalInterfaces = proc.findall(ns + 'ExternalInterface')
				interfaces = []
				for inter in externalInterfaces:
					iname = inter.get('Name')
					iid = inter.get('ID')
					idir = ''
					ix = iy = iz = 0
					ispeed = 0
					iblocking = True
					imodel = ''
					itransport = None

					attributes = inter.findall(ns + 'Attribute')
					for attr in attributes:
						n = attr.get('Name')
						if n == 'direction':
							idir = attr.find(ns + 'Value').text

						elif n == 'position':
							values = attr.findall(ns + 'Attribute')
							for val in values:
								vn = val.get('Name')
								vf = val.find(ns + 'Value')
								if vf is None: break
								elif vn == 'x': ix = float(vf.text)
								elif vn == 'y': iy = float(vf.text)
								elif vn == 'z': iz = float(vf.text)

						elif n == 'speed':
							vf  = attr.find(ns + 'Value')
							if vf is not None and vf.text is not None:
								ispeed = float(vf.text)

						elif n == 'model':
							vf = attr.find(ns + 'Value')
							if vf is not None:
								imodel = vf.text

						elif n == 'blocking':
							iblockingText = attr.find(ns + 'Value').text
							if iblockingText == 'False': iblocking = False

						elif n == 'transportation':
							vf = attr.find(ns + 'Value')
							if vf is not None:
								itransport = vf.text


					i = VR.Interface(iid, idir, iname, p, pPos=[ix,iy,iz], pModel=imodel, pBlock=iblocking, pSpeed=ispeed, pTrans=itransport)
					p.addInterface(i)
					interfaces.append(i)

				#Get all links of the current entity
				allLinks.extend(proc.findall(ns + 'InternalLink'))
				allInterfaces.extend(interfaces)

			#Create entity
			m = None
			userSet = False
			if x != 0 or z != 0: userSet = True

			if typ == 'M' or typ == 'RS' or typ == 'RD' or typ == 'BasicRobot':
				if typ == 'BasicRobot':
					m = VR.BasicRobot(pID=mid, pPath=path, pName=mname, pPos=[x,y,z],
									pUser=userSet, pSize=[length, height, width],
									pClear=clear, pDirVec=[dx,dy,dz], pUpVec=[ux,uy,uz],
									pType=typ, pSpace=wSpace)
				else:
					m = VR.Machine(pID=mid, pPath=path, pName=mname, pPos=[x,y,z],
									pUser=userSet, pSize=[length, height, width],
									pClear=clear, pDirVec=[dx,dy,dz], pUpVec=[ux,uy,uz],
									pType=typ, pSpace=wSpace)
				for mp in processes:
					m.addProcess(mp)
					mp.setParent(m)

			elif typ == 'O': m = VR.Obstacle(pID=mid, pPath=path, pName=mname,
											pPos=[x,y,z], pUser=userSet, pSize=[length, height, width],
											pDirVec=[dx,dy,dz], pUpVec=[ux,uy,uz])

			elif typ == 'R': m = VR.Ressource(pID=mid, pPath=path, pPos=[x,y,z],
											pSize=[length, height, width],
											pDirVec=[dx,dy,dz], pUpVec=[ux,uy,uz])

			else:
				VR.console.printLoadFailure('INVALID_TYPE', mname, typ)
				continue
			VR.console.printLoadSuccess('LOAD_GEO', self.loadPath, mname, typ, mid, str([x,y,z]), path)

		#######################################################

		#Build graph of interfaces
		#Each interface is a node
		#Each link is a directed edge between an output-interface and an input-interface
		for link in allLinks:
			partnerAID = link.get('RefPartnerSideA')
			partnerBID = link.get('RefPartnerSideB')
			a = VR.model.findID(partnerAID, allInterfaces)
			b = VR.model.findID(partnerBID, allInterfaces)
			if a is None:
				VR.console.printLoadFailure('INTERFACE_NOT_FOUND', partnerAID)
				continue
			if b is None:
				VR.console.printLoadFailure('INTERFACE_NOT_FOUND', partnerBID)
				continue
			if a.direction == b.direction:
				VR.console.printLoadFailure('INVALID_LINK', a.direction, b.direction, partnerAID, partnerBID)
				continue
			a.destination = b
			b.destination = a
			if a.parent is not None and b.parent is not None:
				aname = a.parent.parent.name + ':' + a.parent.name
				bname = b.parent.parent.name + ':' + b.parent.name
				VR.console.printLoadSuccess('LOAD_LINK', aname, a.direction, bname, b.direction)
    */


}

void VRAMLLoader::write(string path) {



	/*
	def saveData(self):
		import VR
		import xml.etree.ElementTree as ET
		import xml.dom.minidom
		from itertools import chain

		#CAEX-File-Header, inherited from AMLEditor
		root = ET.Element('CAEXFile')
		root.attrib['FileName'] = self.fileName + self.extension + self.fileExtension
		root.attrib['xmlns:xsi'] = 'http://www.w3.org/2001/XMLSchema-instance'
		root.attrib['xmlns'] = 'http://www.dke.de/CAEX'
		root.attrib['xsi:schemaLocation'] = 'http://www.dke.de/CAEX CAEX_ClassModel_V.3.0.xsd'

		instanceHierarchy = ET.SubElement(root, 'InstanceHierarchy')
		instanceHierarchy.attrib['Name'] = 'InstanceHierarchy'

		def newAttribute(parent, valName, valString, valType='Attribute'):
			"""Creates new sub-xml-element "Attribute" and a sub-sub-xml-element "Value".
				<Attribute Name="valName"><Value>valString</Value></Attribute>"""
			subel = ET.SubElement(parent, valType)
			subel.attrib['Name'] = valName
			if valString != '':
				val = ET.SubElement(subel, 'Value')
				val.text = valString
			return subel

		def newMultiAttribute(parent, multiName, valNames, vals):
			"""Creates new sub-xml-element with multiple sub-sub-xml-elements
				given by (valNames, vals)."""
			attr = newAttribute(parent, multiName, '')
			for n, s in zip(valNames, vals):
				newAttribute(attr, n, str(s))

		def newInterface(parent, inter):
			intel = ET.SubElement(parent, 'ExternalInterface')
			intel.attrib['Name'] = inter.name
			intel.attrib['ID'] = inter.id
			newAttribute(intel, 'direction', inter.direction)
			newMultiAttribute(intel, 'position', ['x','y','z'], inter.getPosition())
			if inter.direction == 'in':
				newAttribute(intel, 'blocking', str(inter.blocking))
			elif inter.direction == 'out':
				newAttribute(intel, 'speed', str(inter.speed))
				newAttribute(intel, 'model', inter.model)

		def newLink(parent, inter):
			if inter is None or inter.destination is None: return
			linkel = ET.SubElement(parent, 'InternalLink')
			linkel.attrib['RefPartnerSideA'] = inter.id
			linkel.attrib['RefPartnerSideB'] = inter.destination.id
			linkel.attrib['Name'] = inter.name + '-' + inter.destination.name

		#Ground
		xmlg = ET.SubElement(instanceHierarchy, 'InternalElement')
		xmlg.attrib['Name'] = 'Ground'
		newAttribute(xmlg, 'type', 'G')
		newMultiAttribute(xmlg, 'position', ['x','y','z'], [VR.groundPrim[0], 0, VR.groundPrim[1]])
		newMultiAttribute(xmlg, 'size', ['length', 'width', 'height'], [VR.groundPrim[2], VR.groundPrim[3], VR.getSetting('groundHeight')])

		#Entities
		for m in chain(VR.machines, VR.obstacles):
			#Entity header
			xmlel = ET.SubElement(instanceHierarchy, 'InternalElement')
			xmlel.attrib['Name'] = m.name
			xmlel.attrib['ID'] = m.id

			#Basic Attributes valid for all entitites
			newAttribute(xmlel, 'type', m.typ)
			newMultiAttribute(xmlel, 'position', ['x','y','z'], m.getPosition())
			newMultiAttribute(xmlel, 'size', ['length','height','width'], m.getPrimSize())

			dirVec = m.getDir()
			upVec = m.getUp()
			dirVec = [round(dirVec[0], 3), round(dirVec[1], 3), round(dirVec[2]), 3]
			upVec = [round(upVec[0], 3), round(upVec[1], 3), round(upVec[2]), 3]
			ori = newAttribute(xmlel, 'orientation', '')
			newMultiAttribute(ori, 'direction_vector', ['dx','dy','dz'], dirVec)
			newMultiAttribute(ori, 'up_vector', ['ux','uy','uz'], upVec)
			newAttribute(xmlel, 'model', m.path)

			#Machine-specific attributes
			if m.typ == 'O' or m.typ == 'R': continue

			newAttribute(xmlel, 'clearance', str(m.clearance))
			#workspace
			ws = newAttribute(xmlel, 'workspace', '')
			newMultiAttribute(ws, 'position', ['x', 'y', 'z'], m.wBoxPrims[0])
			newMultiAttribute(ws, 'size', ['length', 'height', 'width'], m.wBoxPrims[1])

			#Processes
			for p in m.procs:
				xmlp = newAttribute(xmlel, p.name, '', 'InternalElement')
				newAttribute(xmlp, 'speed', str(p.speed))

				#Interfaces
				for i in chain(p.inputs, p.outputs):
					newInterface(xmlp, i)

				#Links
				for l in p.outputs:
					newLink(xmlp, l)

		#Write file, minidom is used for proper indentation
		etstring = ET.tostring(root)
		dom = xml.dom.minidom.parseString(etstring)
		finalXML = dom.toprettyxml()
		saveFile = open(self.savePath, 'w')
		saveFile.write(finalXML)
		VR.console.printSaveSuccess(self.savePath)
		*/

}


