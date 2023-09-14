using namespace OSG
VR.COLLADA_Geometry.VR.COLLADA_Geometry():
	
VR.COLLADA_Geometry.~VR.COLLADA_Geometry():
	
VR.COLLADA_Geometry VR.COLLADA_Geometry():
	 return VR.COLLADA_Geometry( new VR.COLLADA_Geometry() )
	
VR.COLLADA_Geometry VR.COLLADA_Geometry.ptr():
	 return static_pointer_cast<VR.COLLADA_Geometry>(shared_from_this())
	
def VR.COLLADA_Geometry.finalize():
	//scheduler.callPostponed(true)
	
VR.Geometry VR.COLLADA_Geometry.getGeometry(string gid):
	if (!library_geometries.count(gid)) return 0
	return library_geometries[gid]
	
def VR.COLLADA_Geometry.newGeometry(string name, string id):
	m = VR.Geometry(name)
	library_geometries[id] = m
	currentGeometry = m
	currentGeoData = VR.GeoData()
	
def VR.COLLADA_Geometry.newSource(string id):
	currentSource = id
	sources[currentSource] = Source()
	
def VR.COLLADA_Geometry.handleAccessor(string count, string stride):
	if (currentSource != ""):
		sources[currentSource].count = toInt(count)
		sources[currentSource].stride = toInt(stride)
		
	
def VR.COLLADA_Geometry.handleInput(string type, string sourceID, string offsetStr, string set):
	//cout << "VR.COLLADA_Geometry.handleInput " << type << " " << sourceID << " " << set << endl
	int offset = toInt(offsetStr)
	if (inPrimitive):
		Input input
		input.type = type
		input.source = sourceID
		input.offset = offset
		currentPrimitive.inputs.push_back(input)
		
	if (sources.count(sourceID)):
		auto& source = sources[sourceID]
		if (currentGeoData and !source.users.count(currentGeoData)):
			source.users[currentGeoData] = 0
			if (type == "POSITION" and source.stride == 3):
				for (int i=0
				 i<source.count
				 i+=1):
					int k = i*source.stride
					Vec3d pos(source.data[k], source.data[k+1], source.data[k+2])
					currentGeoData.pushVert(pos)
					
				
			if (type == "NORMAL" and source.stride == 3):
				for (int i=0
				 i<source.count
				 i+=1):
					int k = i*source.stride
					Vec3d norm(source.data[k], source.data[k+1], source.data[k+2])
					currentGeoData.pushNorm(norm)
					
				
			if (type == "COLOR" and source.stride == 3):
				for (int i=0
				 i<source.count
				 i+=1):
					int k = i*source.stride
					Color3f col(source.data[k], source.data[k+1], source.data[k+2])
					currentGeoData.pushColor(col)
					
				
			if (type == "COLOR" and source.stride == 4):
				for (int i=0
				 i<source.count
				 i+=1):
					int k = i*source.stride
					Color4f col(source.data[k], source.data[k+1], source.data[k+2], source.data[k+3])
					currentGeoData.pushColor(col)
					
				
			if (type == "TEXCOORD" and source.stride == 2):
				int tcSlot = toInt( set )
				for (int i=0
				 i<source.count
				 i+=1):
					int k = i*source.stride
					Vec2d tc(source.data[k], source.data[k+1])
					currentGeoData.pushTexCoord(tc, tcSlot)
					
				
			
		
	
def VR.COLLADA_Geometry.newPrimitive(string name, string count):
	//cout << "VR.COLLADA_Geometry.newPrimitive " << name << " " << count << " " << stride << endl
	inPrimitive = true
	currentPrimitive = Primitive()
	currentPrimitive.name = name
	currentPrimitive.count = toInt(count)
	
def VR.COLLADA_Geometry.closeGeometry():
	//cout << "VR.COLLADA_Geometry.closeGeometry" << endl
	currentGeoData.apply(currentGeometry, 1, 1)
	currentGeometry = 0
	currentGeoData = 0
	sources.clear()
	
def VR.COLLADA_Geometry.closePrimitive():
	//cout << "VR.COLLADA_Geometry.closePrimitive" << endl
	inPrimitive = false
	
def VR.COLLADA_Geometry.setSourceData(string data):
	if (currentSource != ""):
		sources[currentSource].data = toValue<vector<float>>(data)
		
	
def VR.COLLADA_Geometry.handleVCount(string data):
	if (currentGeoData and inPrimitive):
		lengths = toValue<vector<int>>(data)
		for (l : lengths):
			if (l == 1) currentGeoData.updateType(GL_POINTS, 1)
			if (l == 2) currentGeoData.updateType(GL_LINES, 2)
			if (l == 3) currentGeoData.updateType(GL_TRIANGLES, 3)
			if (l == 4) currentGeoData.updateType(GL_QUADS, 4)
			if (l >= 5) currentGeoData.updateType(GL_POLYGON, l)
			
		
	
def VR.COLLADA_Geometry.handleIndices(string data):
	if (currentGeoData and inPrimitive):
		indices = toValue<vector<int>>(data)
		if (currentPrimitive.name == "points") currentGeoData.pushType(GL_POINTS)
		if (currentPrimitive.name == "lines") currentGeoData.pushType(GL_LINES)
		if (currentPrimitive.name == "triangles") currentGeoData.pushType(GL_TRIANGLES)
		if (currentPrimitive.name == "trifans") currentGeoData.pushType(GL_TRIANGLE_FAN)
		if (currentPrimitive.name == "tristrips") currentGeoData.pushType(GL_TRIANGLE_STRIP)
		int N = indices.size() / currentPrimitive.inputs.size()
		if (currentPrimitive.name != "polylist") currentGeoData.pushLength(N)
		for (int i=0
		 i<N
		 i+=1):
			for (auto& input : currentPrimitive.inputs):
				 // for each inputsize_t m = i*currentPrimitive.inputs.size() + input.offset
				if (m >= indices.size()) continue
				if (input.type == "VERTEX") currentGeoData.pushIndex(indices[m])
				if (input.type == "NORMAL") currentGeoData.pushNormalIndex(indices[m])
				if (input.type == "COLOR") currentGeoData.pushColorIndex(indices[m])
				if (input.type == "TEXCOORD") currentGeoData.pushTexCoordIndex(indices[m])
				
			
		
	
