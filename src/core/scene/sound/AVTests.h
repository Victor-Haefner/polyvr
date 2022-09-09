
#include <iostream>
#include <vector>

extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

using namespace std;

struct VRSoundBuffer {
	ALbyte* data = 0; // pointer to buffer
	int size = 0; // size of buffer
	int sample_rate = 0;
	ALenum format = 0x1101; // AL_FORMAT_MONO16; // number of channels
	bool owned = false;
};

struct OutputStream {
    int64_t next_pts = 0;
    AVStream *st = 0;
    AVCodecContext *enc = 0;
    AVFrame *frame = 0;
    AVFrame *tmp_frame = 0;
    SwrContext *avr = 0;
};

vector<VRSoundBuffer*> ownedBuffer;

AVFrame* test_alloc_audio_frame(enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples) {
    AVFrame* frame = av_frame_alloc();
    if (!frame) { fprintf(stderr, "Error allocating an audio frame\n"); return 0; }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        int ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) { fprintf(stderr, "Error allocating an audio buffer\n"); return 0; }
    }

    return frame;
}

bool test_open_audio(AVFormatContext *oc, OutputStream *ost) {
    int nb_samples, ret;

    AVCodecContext* c = ost->enc;
    cout << "Open audio codec: " << c << endl;
    if (avcodec_open2(c, NULL, NULL) < 0) { fprintf(stderr, "could not open codec\n"); return false; }

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) nb_samples = 10000;
    else nb_samples = c->frame_size;
	
	if (nb_samples == 0) {
		cout << "Warning, no samples set, set to 10000" << endl;
		nb_samples = 10000;
	}

    cout << "Allocate audio frames: " << nb_samples << endl;

    ost->frame     = test_alloc_audio_frame(c->sample_fmt, c->channel_layout, c->sample_rate, nb_samples);
    ost->tmp_frame = test_alloc_audio_frame(AV_SAMPLE_FMT_S16, AV_CH_LAYOUT_MONO, 22050, nb_samples);

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) { fprintf(stderr, "Could not copy the stream parameters\n"); return false; }
	
	return true;
}

void test_add_audio_stream(OutputStream *ost, AVFormatContext *oc, enum AVCodecID codec_id) {
    AVCodec* codec = avcodec_find_encoder(codec_id);
    if (!codec) { fprintf(stderr, "codec not found\n"); return; }

    ost->st = avformat_new_stream(oc, NULL);
    if (!ost->st) { fprintf(stderr, "Could not alloc stream\n"); return; }

	
    AVCodecContext* c = avcodec_alloc_context3(codec);
    if (!c) { fprintf(stderr, "Could not alloc an encoding context\n"); return; }
    ost->enc = c;

    /* put sample parameters */
    c->sample_fmt     = codec->sample_fmts           ? codec->sample_fmts[0]           : AV_SAMPLE_FMT_S16;
    c->sample_rate    = codec->supported_samplerates ? codec->supported_samplerates[0] : 44100;
    c->channel_layout = codec->channel_layouts       ? codec->channel_layouts[0]       : AV_CH_LAYOUT_STEREO;
    c->channels       = av_get_channel_layout_nb_channels(c->channel_layout);
    c->bit_rate       = 64000;

	ost->st->time_base.num = 1;
	ost->st->time_base.den = c->sample_rate;

    // some formats want stream headers to be separate
    if (oc->oformat->flags & AVFMT_GLOBALHEADER) c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    /* initialize sample format conversion;
     * to simplify the code, we always pass the data through lavr, even
     * if the encoder supports the generated format directly -- the price is
     * some extra data copying;
     */
    ost->avr = swr_alloc();
    if (!ost->avr) { fprintf(stderr, "Error allocating the resampling context\n"); return; }

    av_opt_set_channel_layout(ost->avr, "in_channel_layout",  AV_CH_LAYOUT_MONO, 0);
    av_opt_set_sample_fmt    (ost->avr, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int           (ost->avr, "in_sample_rate",     22050,             0);
    av_opt_set_channel_layout(ost->avr, "out_channel_layout", c->channel_layout, 0);
    av_opt_set_sample_fmt    (ost->avr, "out_sample_fmt",     c->sample_fmt,     0);
    av_opt_set_int           (ost->avr, "out_sample_rate",    c->sample_rate,    0);

    int ret = swr_init(ost->avr);
    if (ret < 0) { fprintf(stderr, "Error opening the resampling context\n"); return; }
}

AVFrame* test_get_audio_frame(OutputStream *ost, VRSoundBuffer* buffer) {
    AVFrame* frame = ost->tmp_frame;
    if (!frame || !buffer) return 0;
    int16_t* src = (int16_t*)buffer->data;
    int16_t* dst = (int16_t*)frame->data[0];

    frame->nb_samples = buffer->size*0.5;
    for (int j = 0; j < frame->nb_samples; j++) {
        int v = src[j]; // audio data
        for (int i = 0; i < ost->enc->channels; i++) *dst++ = v;
    }

    return frame;
}

int test_encode_audio_frame(AVFormatContext *oc, OutputStream *ost, AVFrame *frame) {
    AVPacket pkt = { 0 }; // data and size must be 0;
    int got_packet = 0;

    //cout << "   init packet" << endl;
    av_init_packet(&pkt);
    //cout << "   encode audio frame" << endl;
	
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 48, 0) 
    avcodec_encode_audio2(ost->enc, &pkt, frame, &got_packet);
#else 
    //cout << "    send frame" << endl;
    auto error = avcodec_send_frame(ost->enc, frame);
    if ( error != AVERROR_EOF && error != AVERROR(EAGAIN) && error != 0){
        fprintf(stderr, "Could not send frame\n");
        return 0;
    }   
    //if ( error == AVERROR_EOF) cout << "     EOF" << endl; 
    //if ( error == AVERROR(EAGAIN)) cout << "     EAGAIN" << endl;

    //cout << "    receive packet " << error << endl;
    error = avcodec_receive_packet(ost->enc, &pkt);
	if (error == 0) got_packet = 1;
    //if ( error == AVERROR_EOF) cout << "     EOF" << endl; 
    //if ( error == AVERROR(EAGAIN)) cout << "     EAGAIN" << endl; 
    if ( error != AVERROR_EOF && error != AVERROR(EAGAIN) && error != 0) {
        fprintf(stderr, "Could not receive packet\n");
        return 0;
    }  
    //cout << "    write frame " << got_packet << ", " << error << endl; 
#endif

    if (got_packet) {
        pkt.stream_index = ost->st->index;

        //cout << "   rescale packet" << endl;
        av_packet_rescale_ts(&pkt, ost->enc->time_base, ost->st->time_base);

        /* Write the compressed frame to the media file. */
        //cout << "   write frame" << endl;
        if (av_interleaved_write_frame(oc, &pkt) != 0) {
            fprintf(stderr, "Error while writing audio frame\n");
            return 0;
        }
        //cout << "   write frame done" << endl;
    }

    return (frame || got_packet) ? 0 : 1;
}

void test_write_buffer(AVFormatContext *oc, OutputStream *ost, VRSoundBuffer* buffer) {
    cout << "  get audio frame " << buffer << endl;
    AVFrame* frame = test_get_audio_frame(ost, buffer);
    if (!frame) return;
    //cout << "  resample convert " << frame->linesize[0] << " " << frame->nb_samples << endl;
    int ret = swr_convert(ost->avr, NULL, 0, (const uint8_t **)frame->extended_data, frame->nb_samples);
    if (ret < 0) { fprintf(stderr, "Error feeding audio data to the resampler\n"); return; }

    cout << "   write buffer " << size_t(buffer->data) << ", " << buffer->size << ", " << buffer->sample_rate << endl;
    while ((frame) || (!frame && swr_get_out_samples(ost->avr, 0))) {
        // when we pass a frame to the encoder, it may keep a reference to it internally; make sure we do not overwrite it here
        ret = av_frame_make_writable(ost->frame);
        if (ret < 0) {
			cout << " Warning! av_frame_make_writable return " << ret << endl;
			break;
		}

        /* the difference between the two avresample calls here is that the
         * first one just reads the already converted data that is buffered in
         * the lavr output buffer, while the second one also flushes the
         * resampler */
        ret = swr_convert(ost->avr, ost->frame->extended_data, ost->frame->nb_samples, NULL, 0);

        if (ret < 0) { fprintf(stderr, "Error while resampling\n"); break; }

        if (frame && ret != ost->frame->nb_samples) {
            //fprintf(stderr, "Too few samples returned from lavr\n");
            if (ret == 0) {
                //cout << " VRSound::write_buffer: Too few samples returned from lavr! expected: " << ost->frame->nb_samples << ", got: " << ret << endl;
                break;
            }
        }

        //cout << "   encode frame " << ret << ", " << ost->next_pts << ", " << ost->frame->nb_samples << endl;

        ost->frame->nb_samples = ret;
        ost->frame->pts        = ost->next_pts;
        ost->next_pts         += ost->frame->nb_samples;
        auto lastEncodingFlag = test_encode_audio_frame(oc, ost, ret ? ost->frame : NULL);
        //cout << "  encode frame done" << endl;
    }
    cout << "   write buffer done" << endl;
}

void test_close_stream(AVFormatContext *oc, OutputStream *ost) {
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    //av_frame_free(&ost->tmp_frame); // crash under windows
    swr_free(&ost->avr);
}
	
double lastSimTime = 0;
double simPhase = 0;
double Pi = 3.14;
	
VRSoundBuffer* test_genPacket(double dt) {
    float frequency = 440;
    float period1 = 0.2;
    float period2 = 0.8;
	
    float Ac = 32760;
    float wc = frequency;
    int sample_rate = 22050;

    size_t buf_size = size_t(dt * sample_rate);
    buf_size += buf_size%2;
	
	
	auto frame = new VRSoundBuffer();
    frame->size = buf_size*sizeof(short);
    frame->sample_rate = sample_rate;
    frame->format = AL_FORMAT_MONO16;
    frame->data = new ALbyte[frame->size];
    frame->owned = true;
	
	

    double H = dt/(buf_size-1);

    double st = 0;
    for(size_t i=0; i<buf_size; i++) {
        double k = lastSimTime + double(i)*H;
        double Ak = abs(sin(k/period1));

        st = i*2*Pi/sample_rate + simPhase;
        short v = short(Ak * Ac * sin( wc*st ));
        ((short*)frame->data)[i] = v;
    }
    simPhase = st;
    return frame;
}

void createBuffers() {
	for (int i=0; i<10; i++) {
		ownedBuffer.push_back( test_genPacket(0.3) );
	}
}

void testMP3Write() {
	cout << "start av mp3 write test" << endl;
	cout << " AV versions: " << endl;
	cout << " codec:  " << AV_VERSION_MAJOR(LIBAVCODEC_VERSION_INT) << "." << AV_VERSION_MINOR(LIBAVCODEC_VERSION_INT) << endl;
	cout << " format: " << AV_VERSION_MAJOR(LIBAVFORMAT_VERSION_INT) << "." << AV_VERSION_MINOR(LIBAVFORMAT_VERSION_INT) << endl;
	
	createBuffers();
	
	string path = "test.mp3";
	
	
    OutputStream audio_st = { 0 };
    const char *filename = path.c_str();
    av_register_all();
	
	cout << "sound exportToFile: " << filename << endl;

    AVOutputFormat* fmt = av_guess_format(NULL, filename, NULL);
    if (!fmt) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        fmt = av_guess_format("mpeg", NULL, NULL);
    }
    if (!fmt) { fprintf(stderr, "Could not find suitable output format\n"); return; }

    AVFormatContext* oc = avformat_alloc_context();
    if (!oc) { fprintf(stderr, "Memory error\n"); return; }

    oc->oformat = fmt;
	cout << "pass filename to av context: " << sizeof(oc->filename) << endl;
    snprintf(oc->filename, sizeof(oc->filename), "%s", filename);

    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
		test_add_audio_stream(&audio_st, oc, fmt->audio_codec);
		cout << "added audio stream" << endl;
	}

    if (!test_open_audio(oc, &audio_st)) return;
    av_dump_format(oc, 0, filename, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
		cout << "open output file" << endl;
        if (avio_open(&oc->pb, filename, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "Could not open '%s'\n", filename);
            return;
        }
    }

    /* Write the stream header, if any. */
	cout << " start writing buffers" << endl;
    avformat_write_header(oc, NULL);
    for (auto buffer : ownedBuffer) test_write_buffer(oc, &audio_st, buffer);
	cout << " done writing buffers" << endl;

    int ret = 0;
    do {
        ret = test_encode_audio_frame(oc, &audio_st, NULL); // flush last frames
    } while ( ret == 0 );

	cout << " write trailer" << endl;
    av_write_trailer(oc);

    // cleanup
	cout << " cleanup" << endl;
    test_close_stream(oc, &audio_st);
    if (!(fmt->flags & AVFMT_NOFILE)) avio_close(oc->pb);
    avformat_free_context(oc);
	
	
	cout << "av mp3 write test success!" << endl;
}
