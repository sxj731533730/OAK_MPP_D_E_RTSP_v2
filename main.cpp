#include <iostream>
#include "depthai/depthai.hpp"
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include "MppDecode.h"
#include "MppEncode.h"
/*Include ffmpeg header file*/

using namespace std;
using namespace cv;

void Decodethread();

void Encodethread();





void Decodethread() {


    int FPS = 60;
    dai::Pipeline pipeline;
    //定义
    auto cam = pipeline.create<dai::node::ColorCamera>();
    cam->setBoardSocket(dai::CameraBoardSocket::RGB);
    cam->setResolution(dai::ColorCameraProperties::SensorResolution::THE_1080_P);
    cam->setVideoSize(WIDTH, HEIGHT);
    auto Encoder = pipeline.create<dai::node::VideoEncoder>();
    Encoder->setDefaultProfilePreset(cam->getVideoSize(), cam->getFps(),
                                     dai::VideoEncoderProperties::Profile::H264_MAIN);


    cam->video.link(Encoder->input);
    cam->setFps(FPS);

    //定义输出
    auto xlinkoutpreviewOut = pipeline.create<dai::node::XLinkOut>();
    xlinkoutpreviewOut->setStreamName("out");

    Encoder->bitstream.link(xlinkoutpreviewOut->input);

    //结构推送相机
    dai::Device device(pipeline, true);
    //取帧显示


    auto outqueue = device.getOutputQueue("out", cam->getFps(), false);//maxsize 代表缓冲数据
    MPPDecode *mppDecode = new MPPDecode();
    mppDecode->init_decode();

    auto av_packet = (AVPacket *) av_malloc(sizeof(AVPacket)); // 申请空间，存放的每一帧数据 （h264、h265）
    //这边可以调整i的大小来改变文件中的视频时间,每 +1 就是一帧数据
    while (1) {

        auto h265Packet = outqueue->get<dai::ImgFrame>();
        //videoFile.write((char *) (h265Packet->getData().data()), h265Packet->getData().size());

        av_packet->data = (uint8_t *) h265Packet->getData().data();    //这里填入一个指向完整H264数据帧的指针
        av_packet->size = h265Packet->getData().size();        //这个填入H265 数据帧的大小
        //printf("mpi_dec_test start\n");
        av_packet->stream_index = 0;

        mpp_log("--------------\ndata size is: %d\n-------------", av_packet->size);
       // printf("---------------\n");
        mppDecode->decode_simple(av_packet);


    }
    delete mppDecode;
    av_free(av_packet);
}

void Encodethread() {

    std::string suffix = "live";
    std::string ip = "127.0.0.1";
    std::string port = "5554";
    std::string rtsp_url = "rtsp://" + ip + ":" + port + "/" + suffix;
    printf("%s\n", rtsp_url.c_str());
    std::shared_ptr<xop::EventLoop> event_loop(new xop::EventLoop());
    std::shared_ptr<xop::RtspServer> server = xop::RtspServer::Create(event_loop.get());

    if (!server->Start("0.0.0.0", atoi(port.c_str()))) {
        printf("RTSP Server listen on %s failed.\n", port.c_str());
        return;
    }

#ifdef AUTH_CONFIG
    server->SetAuthConfig("-_-", "admin", "12345");
#endif

    xop::MediaSession *session = xop::MediaSession::CreateNew("live");
    session->AddSource(xop::channel_0, xop::H264Source::CreateNew());
//session->StartMulticast();
    session->AddNotifyConnectedCallback([](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port) {
        printf("RTSP client connect, ip=%s, port=%hu \n", peer_ip.c_str(), peer_port);
    });

    session->AddNotifyDisconnectedCallback([](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port) {
        printf("RTSP client disconnect, ip=%s, port=%hu \n", peer_ip.c_str(), peer_port);
    });

    xop::MediaSessionId session_id = server->AddSession(session);

    H264Server *h264server_item = new H264Server();
    h264server_item->init_paramter();

    MppEncode *item_mpp_h264 = new MppEncode();
    int ret = item_mpp_h264->init_paramter();
    if (ret != 0) {
        printf("inital information fail\n");
    }
    while (1) {
        item_mpp_h264->yuv_enc_h264(h264server_item, server.get(), session_id);
    }
    item_mpp_h264->destory();
    delete item_mpp_h264;
    delete h264server_item;
}

int main() {

    const int thread_num = 2;
    std::array<thread, thread_num> threads;
    threads = {
            thread(Decodethread),
            thread(Encodethread)
    };

    for (int i = 0; i < thread_num; i++) {
        threads[i].join();
    }


    return 0;
}