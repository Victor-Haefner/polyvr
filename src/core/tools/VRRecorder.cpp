#include "VRRecorder.h"
#include "core/setup/windows/VRView.h"
#include "core/setup/windows/VRGtkWindow.h"
#include "core/setup/VRSetup.h"
#include "core/scene/VRSceneManager.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/VRTransform.h"
#include "core/objects/VRCamera.h"
#include "core/objects/material/VRTexture.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/system/VRSystem.h"

#include <OpenSG/OSGImage.h>

extern "C" {
    #include <libavutil/opt.h>
    #include <libavutil/imgutils.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

using namespace OSG;

namespace OSG {
class VRFrame {
    public:
        VRTexturePtr capture = 0;
        int timestamp = 0;

        int valid = 0;
        int pktSize = 0;
        char* pktData = 0;

        int width = 0;
        int height = 0;

        Vec3d f,a,u; // from at up

        VRFrame() {}
        ~VRFrame() { if (pktData) delete pktData; }

        void transcode(AVFrame *frame, AVCodecContext* codec_context, SwsContext* sws_context, int i) {
            valid = 0;
            if (!capture) return;

            AVPacket pkt;
            av_init_packet(&pkt);
            pkt.data = NULL;    // packet data will be allocated by the encoder
            pkt.size = 0;

            const unsigned char* data = capture->getImage()->getData();
            if (codec_context->pix_fmt == AV_PIX_FMT_YUV420P) {
                const int in_linesize[1] = { 3 * codec_context->width };
                sws_scale(sws_context, (const uint8_t * const *)&data, in_linesize, 0, codec_context->height, frame->data, frame->linesize);
            }

            /* encode the image */
            frame->pts = i;
            int ret = avcodec_encode_video2(codec_context, &pkt, frame, &valid);
            if (ret < 0) { fprintf(stderr, "Error encoding frame\n"); return; }

            pktSize = pkt.size;
            pktData = new char[pktSize];
            memcpy(pktData, pkt.data, pktSize);

            av_packet_unref(&pkt);
            capture = 0;
        }

        void write(FILE* f) {
            if (valid) fwrite(pktData, 1, pktSize, f);
        }
};
}

VRRecorder::VRRecorder() {
    av_register_all();
    avcodec_register_all();

    toggleCallback = VRFunction<bool>::create("recorder toggle", bind(&VRRecorder::setRecording, this, _1));
    updateCallback = VRUpdateCb::create("recorder update", bind(&VRRecorder::capture, this));

    //initCodec();
    setCodec("MPEG2VIDEO");
    setBitrate(20);
}

VRRecorder::~VRRecorder() { ; }
shared_ptr<VRRecorder> VRRecorder::create() { return shared_ptr<VRRecorder>(new VRRecorder()); }

void VRRecorder::setView(int i) {
    viewID = i;
    if (VRSetup::getCurrent() == 0) return;
    view = VRSetup::getCurrent()->getView(i);
}

void VRRecorder::setMaxFrames(int maxf) { maxFrames = maxf; }
bool VRRecorder::frameLimitReached() { return ((int)captures.size() == maxFrames); }

void VRRecorder::setTransform(VRTransformPtr t, int f) {
    if (f >= (int)captures.size() || f < 0) return;
    VRFrame* fr = captures[f];
    cout << "setTransform " << t->getName() << " " << fr->f << endl;
    t->setFrom(fr->f);
    t->setAt(fr->a);
    t->setUp(fr->u);
}

Vec3d VRRecorder::getFrom(int f) { VRFrame* fr = captures[f]; return fr->f; }
Vec3d VRRecorder::getDir(int f) { VRFrame* fr = captures[f]; Vec3d d = fr->a - fr->f; d.normalize(); return d; }
Vec3d VRRecorder::getAt(int f) { VRFrame* fr = captures[f]; return fr->a; }
Vec3d VRRecorder::getUp(int f) { VRFrame* fr = captures[f]; return fr->u; }

void VRRecorder::capture() {
    auto v = view.lock();
    if (!v) {
        v = VRSetup::getCurrent()->getView(viewID);
        view = v;
    }
    if (!v) return;
    if (frameLimitReached()) return;

    //int ts = VRGlobals::get()->CURRENT_FRAME;
    VRFrame* f = new VRFrame();
    captures.push_back(f);
    f->capture = v->grab();
    f->timestamp = getTime()*1e-3;

    VRTransformPtr t = v->getCamera();
    if (t == 0) return;
    f->f = t->getFrom();
    f->a = t->getAt();
    f->u = t->getUp();

    f->width = f->capture->getImage()->getWidth();
    f->height = f->capture->getImage()->getHeight();

    if (!frame) initFrame();

    f->transcode(frame, codec_context, sws_context, captures.size()-1);
}

bool VRRecorder::isRunning() { return running; }

vector<string> VRRecorder::getCodecList() {
    vector<string> res;
    for (auto c : codecs) res.push_back(c.first);
    return res;
}

vector<string> VRRecorder::getResList() {
    //return { "custom", "1080p", "720p", "480p", "240p" };
    return { "custom", "720p", "480p", "240p" };
}

void VRRecorder::setViewResolution(string res) {
    auto w = VRSetup::getCurrent()->getEditorWindow();
    if (res == "1080p") w->forceSize(1920, 1080);
    if (res == "720p")  w->forceSize(1280, 720);
    if (res == "480p")  w->forceSize(640, 480);
    if (res == "240p")  w->forceSize(320, 240);
}

void VRRecorder::enableVSync(bool b) {
    cout << "VRRecorder::enableVSync " << b << endl;
    auto w = VRSetup::getCurrent()->getEditorWindow();
    w->enableVSync(b);
}

void VRRecorder::clear() {
    for (auto f : captures) delete f;
    captures.clear();
}

int VRRecorder::getRecordingSize() { return captures.size(); }
float VRRecorder::getRecordingLength() {
    if (captures.size() == 0) return 0;
    int t0 = (*captures.begin())->timestamp;
    int t1 = (*captures.rbegin())->timestamp;
    return (t1-t0)*0.001; //seconds
}

void VRRecorder::setBitrate(int br) { bitrate = br; initCodec(); }
int VRRecorder::getBitrate() { return bitrate; }
void VRRecorder::setCodec(string c) { codecName = c; initCodec(); }
string VRRecorder::getCodec() { return codecName; }

void VRRecorder::initCodec() {
    if (captures.size() == 0) { fprintf(stderr, "No initial capture for CODEC init!\n"); return; }
    AVCodecID codec_id = (AVCodecID)codecs[codecName];
    codec = avcodec_find_encoder(codec_id);
    if (!codec) { fprintf(stderr, "Codec not found\n"); return; }

    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) { fprintf(stderr, "Could not allocate video codec context\n"); return; }

    codec_context->width = captures[0]->width;
    codec_context->height = captures[0]->height;
    codec_context->bit_rate = codec_context->width*codec_context->height * bitrate;
	codec_context->time_base.num = 1;
	codec_context->time_base.den = 25;/* frames per second */
    codec_context->gop_size = 10; /* emit one intra frame every ten frames */
    codec_context->max_b_frames = 1;
    codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
    //codec_context->pix_fmt = AV_PIX_FMT_RGB24;
    av_opt_set(codec_context->priv_data, "preset", "ultrafast", 0);

    AVDictionary *param = 0;
    av_dict_set(&param, "crf", "0", 0);
    if (avcodec_open2(codec_context, codec, &param) < 0) { fprintf(stderr, "Could not open codec\n"); return; } /* open codec */

    sws_context = sws_getContext(
        codec_context->width, codec_context->height, AV_PIX_FMT_RGB24,
        codec_context->width, codec_context->height, AV_PIX_FMT_YUV420P,
        SWS_FAST_BILINEAR, 0, 0, 0);
    if (!sws_context) fprintf(stderr, "Could not initialize the conversion context\n");
}

void VRRecorder::closeCodec() {
    avcodec_close(codec_context);
    av_free(codec_context);
}

void VRRecorder::initFrame() {
    initCodec();
#ifdef OLD_LIBAV
    frame = avcodec_alloc_frame(); // Allocate frame
#else
    frame = av_frame_alloc(); // Allocate frame
#endif
    if (!frame) { fprintf(stderr, "Could not allocate video frame\n"); return; }
    frame->format = codec_context->pix_fmt;
    frame->width  = codec_context->width;
    frame->height = codec_context->height;

    /* the image can be allocated by any means && av_image_alloc() is
    * just the most convenient way if av_malloc() is to be used */
    int ret = av_image_alloc(frame->data, frame->linesize, codec_context->width, codec_context->height, codec_context->pix_fmt, 32);
    if (ret < 0) { fprintf(stderr, "Could not allocate raw picture buffer\n"); return; }
}

void VRRecorder::closeFrame() {
    av_freep(&frame->data[0]);
#ifdef OLD_LIBAV
    avcodec_free_frame(&frame);
#else
    av_frame_free(&frame);
#endif
    frame = 0;
}

void VRRecorder::compile(string path) {
    if (captures.size() == 0) return;

    /*for (int i=0; i<1; i++) { // test export the first N images
        string pimg = path+"."+toString(i)+".png";
        captures[i]->capture->write(pimg.c_str());
    }*/

    FILE* f = fopen(path.c_str(), "wb");
    if (!f) { fprintf(stderr, "Could not open %s\n", path.c_str()); return; }

    for (int i=0; i<(int)captures.size(); i++) captures[i]->write(f);

    /* get the delayed frames */
    for (int got_output = 1; got_output;) {
        fflush(stdout);

        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.size = 0;
        pkt.data = NULL;
        int ret = avcodec_encode_video2(codec_context, &pkt, NULL, &got_output);
        if (ret < 0) { fprintf(stderr, "Error encoding frame\n"); return; }

        if (got_output) {
            fwrite(pkt.data, 1, pkt.size, f);
            av_packet_unref(&pkt);
        }
    }

    /* add sequence end code to have a real mpeg file */
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);

    closeCodec();
    closeFrame();
}

VRTexturePtr VRRecorder::get(int f) {
    VRFrame* fr = captures[f];
    return fr->capture;
}

void VRRecorder::setRecording(bool b) {
    if (running == b) return;
    running = b;
    if (b) VRSceneManager::get()->addUpdateFkt(updateCallback);
    else VRSceneManager::get()->dropUpdateFkt(updateCallback);
}

string VRRecorder::getPath() { return "recording_"+toString(VRGlobals::CURRENT_FRAME)+".avi"; }

weak_ptr<VRFunction<bool> > VRRecorder::getToggleCallback() { return toggleCallback; }


map<string, int> VRRecorder::codecs = {
    make_pair("MPEG1VIDEO",AV_CODEC_ID_MPEG1VIDEO),
    make_pair("MPEG2VIDEO",AV_CODEC_ID_MPEG2VIDEO), ///< preferred ID for MPEG-1/2 video decoding
    //make_pair("MPEG2VIDEO_XVMC",AV_CODEC_ID_MPEG2VIDEO_XVMC),
    make_pair("H261",AV_CODEC_ID_H261),
    make_pair("H263",AV_CODEC_ID_H263),
    make_pair("RV10",AV_CODEC_ID_RV10),
    make_pair("RV20",AV_CODEC_ID_RV20),
    make_pair("MJPEG",AV_CODEC_ID_MJPEG),
    make_pair("MJPEGB",AV_CODEC_ID_MJPEGB),
    make_pair("LJPEG",AV_CODEC_ID_LJPEG),
    make_pair("SP5X",AV_CODEC_ID_SP5X),
    make_pair("JPEGLS",AV_CODEC_ID_JPEGLS),
    make_pair("MPEG4",AV_CODEC_ID_MPEG4),
    make_pair("RAWVIDEO",AV_CODEC_ID_RAWVIDEO),
    make_pair("MSMPEG4V1",AV_CODEC_ID_MSMPEG4V1),
    make_pair("MSMPEG4V2",AV_CODEC_ID_MSMPEG4V2),
    make_pair("MSMPEG4V3",AV_CODEC_ID_MSMPEG4V3),
    make_pair("WMV1",AV_CODEC_ID_WMV1),
    make_pair("WMV2",AV_CODEC_ID_WMV2),
    make_pair("H263P",AV_CODEC_ID_H263P),
    make_pair("H263I",AV_CODEC_ID_H263I),
    make_pair("FLV1",AV_CODEC_ID_FLV1),
    make_pair("SVQ1",AV_CODEC_ID_SVQ1),
    make_pair("SVQ3",AV_CODEC_ID_SVQ3),
    make_pair("DVVIDEO",AV_CODEC_ID_DVVIDEO),
    make_pair("HUFFYUV",AV_CODEC_ID_HUFFYUV),
    make_pair("CYUV",AV_CODEC_ID_CYUV),
    make_pair("H264",AV_CODEC_ID_H264),
    make_pair("INDEO3",AV_CODEC_ID_INDEO3),
    make_pair("VP3",AV_CODEC_ID_VP3),
    make_pair("THEORA",AV_CODEC_ID_THEORA),
    make_pair("ASV1",AV_CODEC_ID_ASV1),
    make_pair("ASV2",AV_CODEC_ID_ASV2),
    make_pair("FFV1",AV_CODEC_ID_FFV1),
    make_pair("4XM",AV_CODEC_ID_4XM),
    make_pair("VCR1",AV_CODEC_ID_VCR1),
    make_pair("CLJR",AV_CODEC_ID_CLJR),
    make_pair("MDEC",AV_CODEC_ID_MDEC),
    make_pair("ROQ",AV_CODEC_ID_ROQ),
    make_pair("INTERPLAY_VIDEO",AV_CODEC_ID_INTERPLAY_VIDEO),
    make_pair("XAN_WC3",AV_CODEC_ID_XAN_WC3),
    make_pair("XAN_WC4",AV_CODEC_ID_XAN_WC4),
    make_pair("RPZA",AV_CODEC_ID_RPZA),
    make_pair("CINEPAK",AV_CODEC_ID_CINEPAK),
    make_pair("WS_VQA",AV_CODEC_ID_WS_VQA),
    make_pair("MSRLE",AV_CODEC_ID_MSRLE),
    make_pair("MSVIDEO1",AV_CODEC_ID_MSVIDEO1),
    make_pair("IDCIN",AV_CODEC_ID_IDCIN),
    make_pair("8BPS",AV_CODEC_ID_8BPS),
    make_pair("SMC",AV_CODEC_ID_SMC),
    make_pair("FLIC",AV_CODEC_ID_FLIC),
    make_pair("TRUEMOTION1",AV_CODEC_ID_TRUEMOTION1),
    make_pair("VMDVIDEO",AV_CODEC_ID_VMDVIDEO),
    make_pair("MSZH",AV_CODEC_ID_MSZH),
    make_pair("ZLIB",AV_CODEC_ID_ZLIB),
    make_pair("QTRLE",AV_CODEC_ID_QTRLE),
    make_pair("SNOW",AV_CODEC_ID_SNOW),
    make_pair("TSCC",AV_CODEC_ID_TSCC),
    make_pair("ULTI",AV_CODEC_ID_ULTI),
    make_pair("QDRAW",AV_CODEC_ID_QDRAW),
    make_pair("VIXL",AV_CODEC_ID_VIXL),
    make_pair("QPEG",AV_CODEC_ID_QPEG),
    make_pair("PNG",AV_CODEC_ID_PNG),
    make_pair("PPM",AV_CODEC_ID_PPM),
    make_pair("PBM",AV_CODEC_ID_PBM),
    make_pair("PGM",AV_CODEC_ID_PGM),
    make_pair("PGMYUV",AV_CODEC_ID_PGMYUV),
    make_pair("PAM",AV_CODEC_ID_PAM),
    make_pair("FFVHUFF",AV_CODEC_ID_FFVHUFF),
    make_pair("RV30",AV_CODEC_ID_RV30),
    make_pair("RV40",AV_CODEC_ID_RV40),
    make_pair("VC1",AV_CODEC_ID_VC1),
    make_pair("WMV3",AV_CODEC_ID_WMV3),
    make_pair("LOCO",AV_CODEC_ID_LOCO),
    make_pair("WNV1",AV_CODEC_ID_WNV1),
    make_pair("AASC",AV_CODEC_ID_AASC),
    make_pair("INDEO2",AV_CODEC_ID_INDEO2),
    make_pair("FRAPS",AV_CODEC_ID_FRAPS),
    make_pair("TRUEMOTION2",AV_CODEC_ID_TRUEMOTION2),
    make_pair("BMP",AV_CODEC_ID_BMP),
    make_pair("CSCD",AV_CODEC_ID_CSCD),
    make_pair("MMVIDEO",AV_CODEC_ID_MMVIDEO),
    make_pair("ZMBV",AV_CODEC_ID_ZMBV),
    make_pair("AVS",AV_CODEC_ID_AVS),
    make_pair("SMACKVIDEO",AV_CODEC_ID_SMACKVIDEO),
    make_pair("NUV",AV_CODEC_ID_NUV),
    make_pair("KMVC",AV_CODEC_ID_KMVC),
    make_pair("FLASHSV",AV_CODEC_ID_FLASHSV),
    make_pair("CAVS",AV_CODEC_ID_CAVS),
    make_pair("JPEG2000",AV_CODEC_ID_JPEG2000),
    make_pair("VMNC",AV_CODEC_ID_VMNC),
    make_pair("VP5",AV_CODEC_ID_VP5),
    make_pair("VP6",AV_CODEC_ID_VP6),
    make_pair("VP6F",AV_CODEC_ID_VP6F),
    make_pair("TARGA",AV_CODEC_ID_TARGA),
    make_pair("DSICINVIDEO",AV_CODEC_ID_DSICINVIDEO),
    make_pair("TIERTEXSEQVIDEO",AV_CODEC_ID_TIERTEXSEQVIDEO),
    make_pair("TIFF",AV_CODEC_ID_TIFF),
    make_pair("GIF",AV_CODEC_ID_GIF),
    make_pair("DXA",AV_CODEC_ID_DXA),
    make_pair("DNXHD",AV_CODEC_ID_DNXHD),
    make_pair("THP",AV_CODEC_ID_THP),
    make_pair("SGI",AV_CODEC_ID_SGI),
    make_pair("C93",AV_CODEC_ID_C93),
    make_pair("BETHSOFTVID",AV_CODEC_ID_BETHSOFTVID),
    make_pair("PTX",AV_CODEC_ID_PTX),
    make_pair("TXD",AV_CODEC_ID_TXD),
    make_pair("VP6A",AV_CODEC_ID_VP6A),
    make_pair("AMV",AV_CODEC_ID_AMV),
    make_pair("VB",AV_CODEC_ID_VB),
    make_pair("PCX",AV_CODEC_ID_PCX),
    make_pair("SUNRAST",AV_CODEC_ID_SUNRAST),
    make_pair("INDEO4",AV_CODEC_ID_INDEO4),
    make_pair("INDEO5",AV_CODEC_ID_INDEO5),
    make_pair("MIMIC",AV_CODEC_ID_MIMIC),
    make_pair("RL2",AV_CODEC_ID_RL2),
    make_pair("ESCAPE124",AV_CODEC_ID_ESCAPE124),
    make_pair("DIRAC",AV_CODEC_ID_DIRAC),
    make_pair("BFI",AV_CODEC_ID_BFI),
    make_pair("CMV",AV_CODEC_ID_CMV),
    make_pair("MOTIONPIXELS",AV_CODEC_ID_MOTIONPIXELS),
    make_pair("TGV",AV_CODEC_ID_TGV),
    make_pair("TGQ",AV_CODEC_ID_TGQ),
    make_pair("TQI",AV_CODEC_ID_TQI),
    make_pair("AURA",AV_CODEC_ID_AURA),
    make_pair("AURA2",AV_CODEC_ID_AURA2),
    make_pair("V210X",AV_CODEC_ID_V210X),
    make_pair("TMV",AV_CODEC_ID_TMV),
    make_pair("V210",AV_CODEC_ID_V210),
    make_pair("DPX",AV_CODEC_ID_DPX),
    make_pair("MAD",AV_CODEC_ID_MAD),
    make_pair("FRWU",AV_CODEC_ID_FRWU),
    make_pair("FLASHSV2",AV_CODEC_ID_FLASHSV2),
    make_pair("CDGRAPHICS",AV_CODEC_ID_CDGRAPHICS),
    make_pair("R210",AV_CODEC_ID_R210),
    make_pair("ANM",AV_CODEC_ID_ANM),
    make_pair("BINKVIDEO",AV_CODEC_ID_BINKVIDEO),
    make_pair("IFF_ILBM",AV_CODEC_ID_IFF_ILBM),
    make_pair("IFF_BYTERUN1",AV_CODEC_ID_IFF_BYTERUN1),
    make_pair("KGV1",AV_CODEC_ID_KGV1),
    make_pair("YOP",AV_CODEC_ID_YOP),
    make_pair("VP8",AV_CODEC_ID_VP8),
    make_pair("PICTOR",AV_CODEC_ID_PICTOR),
    make_pair("ANSI",AV_CODEC_ID_ANSI),
    make_pair("A64_MULTI",AV_CODEC_ID_A64_MULTI),
    make_pair("A64_MULTI5",AV_CODEC_ID_A64_MULTI5),
    make_pair("R10K",AV_CODEC_ID_R10K),
    make_pair("MXPEG",AV_CODEC_ID_MXPEG),
    make_pair("LAGARITH",AV_CODEC_ID_LAGARITH),
    make_pair("PRORES",AV_CODEC_ID_PRORES),
    make_pair("JV",AV_CODEC_ID_JV),
    make_pair("DFA",AV_CODEC_ID_DFA),
    make_pair("WMV3IMAGE",AV_CODEC_ID_WMV3IMAGE),
    make_pair("VC1IMAGE",AV_CODEC_ID_VC1IMAGE),
    make_pair("UTVIDEO",AV_CODEC_ID_UTVIDEO),
    make_pair("BMV_VIDEO",AV_CODEC_ID_BMV_VIDEO),
    make_pair("VBLE",AV_CODEC_ID_VBLE),
    make_pair("DXTORY",AV_CODEC_ID_DXTORY),
    make_pair("V410",AV_CODEC_ID_V410),
    make_pair("XWD",AV_CODEC_ID_XWD),
    make_pair("CDXL",AV_CODEC_ID_CDXL),
    make_pair("XBM",AV_CODEC_ID_XBM),
    make_pair("ZEROCODEC",AV_CODEC_ID_ZEROCODEC),
    make_pair("MSS1",AV_CODEC_ID_MSS1),
    make_pair("MSA1",AV_CODEC_ID_MSA1),
    make_pair("TSCC2",AV_CODEC_ID_TSCC2),
    make_pair("MTS2",AV_CODEC_ID_MTS2),
    make_pair("CLLC",AV_CODEC_ID_CLLC),
    make_pair("MSS2",AV_CODEC_ID_MSS2)
};



