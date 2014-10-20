#include "Kinect.h"
#include <string.h>


KinectMutex::KinectMutex() { pthread_mutex_init( &m_mutex, NULL ); }
void KinectMutex::lock() { pthread_mutex_lock( &m_mutex ); }
void KinectMutex::unlock()  { pthread_mutex_unlock( &m_mutex ); }

Kinect::Kinect(freenect_context *_ctx, int _index) : Freenect::FreenectDevice(_ctx, _index) {
    m_buffer_depth = buffer(FREENECT_VIDEO_RGB_SIZE);
    m_buffer_video = buffer(FREENECT_VIDEO_RGB_SIZE);

    m_gamma = vector<uint16_t>(2048);
    for( unsigned int i = 0 ; i < 2048 ; i++) {
        float v = i/2048.0;
        v = pow(v, 3)* 6;
        m_gamma[i] = v*6*256;
    }

    m_new_rgb_frame = false;
    m_new_depth_frame = false;
}

Kinect::~Kinect() {
	//stopVideo();
	stopDepth();
	setLed(LED_OFF);
}

Kinect* Kinect::get(bool verbose) {
    static Kinect* device = 0;

    if (device == 0) {
        try {
            if (verbose) cout << "\nInit Freenect" << endl;
            static Freenect::Freenect<Kinect> freenect;
            //double freenect_angle(0);
            //freenect_video_format requested_format(FREENECT_VIDEO_RGB);

            if (verbose) cout << "\nCreate Device" << endl;
            device = &freenect.createDevice(0);
            if (verbose) cout << "\nSet Tilt" << endl;
            device->setTiltDegrees(0);
            //device->startVideo();
            if (verbose) cout << "\nStart Depth" << endl;
            device->startDepth();
            if (verbose) cout << "\nSet LED" << endl;
            device->setLed(LED_RED);
        } catch (std::runtime_error& e) {
            cout << "\nWarning! No Kinect found." << endl;
        }
    }

    return device;
}

void Kinect::VideoCallback(void* _rgb, uint32_t timestamp) { //TODO
    return;
    /*m_rgb_mutex.lock();
    uint8_t* rgb = static_cast<uint8_t*>(_rgb);
    copy(rgb, rgb+getVideoBufferSize(), m_buffer_video.begin());
    m_new_rgb_frame = true;
    m_rgb_mutex.unlock();*/
};

/*void Kinect::DepthCallback(void* _depth, uint32_t timestamp) {
    m_depth_mutex.lock();
    uint16_t* depth = static_cast<uint16_t*>(_depth);
    for( unsigned int i = 0 ; i < FREENECT_FRAME_PIX ; i++) {
        int pval = m_gamma[depth[i]];
        int lb = pval & 0xff;
        switch (pval>>8) {
            case 0:
                m_buffer_depth[3*i+0] = 255;
                m_buffer_depth[3*i+1] = 255-lb;
                m_buffer_depth[3*i+2] = 255-lb;
                break;
            case 1:
                m_buffer_depth[3*i+0] = 255;
                m_buffer_depth[3*i+1] = lb;
                m_buffer_depth[3*i+2] = 0;
                break;
            case 2:
                m_buffer_depth[3*i+0] = 255-lb;
                m_buffer_depth[3*i+1] = 255;
                m_buffer_depth[3*i+2] = 0;
                break;
            case 3:
                m_buffer_depth[3*i+0] = 0;
                m_buffer_depth[3*i+1] = 255;
                m_buffer_depth[3*i+2] = lb;
                break;
            case 4:
                m_buffer_depth[3*i+0] = 0;
                m_buffer_depth[3*i+1] = 255-lb;
                m_buffer_depth[3*i+2] = 255;
                break;
            case 5:
                m_buffer_depth[3*i+0] = 0;
                m_buffer_depth[3*i+1] = 0;
                m_buffer_depth[3*i+2] = 255-lb;
                break;
            default:
                m_buffer_depth[3*i+0] = 0;
                m_buffer_depth[3*i+1] = 0;
                m_buffer_depth[3*i+2] = 0;
                break;
        }
    }
    m_new_depth_frame = true;
    m_depth_mutex.unlock();
}*/

void Kinect::DepthCallback(void* _depth, uint32_t timestamp) {
    m_depth_mutex.lock();

    memcpy((char*)&m_buffer_depth[0], (char*)_depth, FREENECT_FRAME_PIX*2);

    m_new_depth_frame = true;
    m_depth_mutex.unlock();
}

bool Kinect::getRGB(buffer &b) {
    m_rgb_mutex.lock();
    if(m_new_rgb_frame) {
        b.swap(m_buffer_video);
        m_new_rgb_frame = false;
        m_rgb_mutex.unlock();
        return true;
    } else {
        m_rgb_mutex.unlock();
        return false;
    }
}

bool Kinect::getDepth(buffer &b) {
    m_depth_mutex.lock();
    if(m_new_depth_frame) {
        b.swap(m_buffer_depth);
        m_new_depth_frame = false;
        m_depth_mutex.unlock();
        return true;
    } else {
        m_depth_mutex.unlock();
        return false;
    }
}
