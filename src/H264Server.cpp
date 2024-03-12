//
// Created by ubuntu on 2023/5/10.
//

#include "H264Server.h"

H264Server::H264Server() {}

H264Server::~H264Server() {

}

int H264Server::init_paramter() {


    //fp_output = fopen("rtsp_sample.h264", "wb");

    return 0;

}

int H264Server::pushFrame(MppPacket packet,xop::RtspServer* rtsp_server, xop::MediaSessionId session_id) {

    //printf("start get h264\n");

    //printf("end get h264 %d\n",length);
    if (mpp_packet_get_length(packet)!= 0) {

        //printf("start get h264 %d\n",length);
        // fwrite(mpp_packet_get_pos(packet), 1, mpp_packet_get_length(packet), fp_output);
        //printf("end get h264 %d\n",length);
        xop::AVFrame videoFrame = {0};
        videoFrame.type = 0;
        videoFrame.size = mpp_packet_get_length(packet);
        videoFrame.timestamp = xop::H264Source::GetTimestamp();
        videoFrame.buffer.reset(new uint8_t[videoFrame.size]);
        memcpy(videoFrame.buffer.get(),mpp_packet_get_pos(packet), videoFrame.size);
        rtsp_server->PushFrame(session_id, xop::channel_0, videoFrame);
//        if(ret==false){
//            printf("push frame fail \n");
//        }
    }

    //xop::Timer::Sleep(40);
    return 0;
}
