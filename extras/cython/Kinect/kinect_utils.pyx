import numpy as np
cimport numpy as np

DTYPE = np.float32
ctypedef np.float32_t DTYPE_t

def compute_kinect_pc(dIn, dOut):
	cdef np.ndarray[unsigned short, ndim=1] depth
	cdef np.ndarray[DTYPE_t, ndim=2] pOut
	depth = dIn.image
	pOut = dOut.mesh.points

	cdef np.ndarray[unsigned char, ndim=1] color
	cdef np.ndarray[DTYPE_t, ndim=2] cOut
	color = dIn.video
	cOut = dOut.mesh.colors
	
	cdef int i, j, k, N, d
	cdef float x, y, z, fx, fy, cx, cy
		
	fx = 520.841083
	fy = 519.896053
	cx = 320
	cy = 240
	N = 480*640

	for j in range(480):
		for i in range(640):
			k = i+j*640
			d = depth[k]

			cOut[k, 0] = 0.001525879*color[k*3 + 0]
			cOut[k, 1] = 0.001525879*color[k*3 + 1]
			cOut[k, 2] = 0.001525879*color[k*3 + 2]

			if (d > 1024):
				pOut[k, 0] = 0
				pOut[k, 1] = 0
				pOut[k, 2] = 0
				continue
			
			z = 1.0 / (d*0.0030711016 - 3.3309495161)
			x = z*(i-cx)/fx
			y = z*(j-cy)/fy

			pOut[k, 0] = x
			pOut[k, 1] = y
			pOut[k, 2] = z
