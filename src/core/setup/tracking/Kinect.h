#ifndef KINECT_H_INCLUDED
#define KINECT_H_INCLUDED

#include <libfreenect.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <pthread.h>

using namespace std;

class KinectMutex {
    public:
        KinectMutex();
        void lock();
        void unlock();

    private:
        pthread_mutex_t m_mutex;
};

class Kinect : public Freenect::FreenectDevice {
    public:
        typedef vector<uint16_t> buffer;

    private:
		buffer m_buffer_depth;
		buffer m_buffer_video;
		vector<uint16_t> m_gamma;
		KinectMutex m_rgb_mutex;
		KinectMutex m_depth_mutex;
		bool m_new_rgb_frame;
		bool m_new_depth_frame;

		friend class Freenect::Freenect<Kinect>;

        Kinect(freenect_context *_ctx, int _index);

    public:
        static Kinect* get(bool verbose = false);

        ~Kinect();

		void VideoCallback(void* _rgb, uint32_t timestamp);

		void DepthCallback(void* _depth, uint32_t timestamp);

		bool getRGB(buffer &b);

		bool getDepth(buffer &b);
};

#endif // KINECT_H_INCLUDED
