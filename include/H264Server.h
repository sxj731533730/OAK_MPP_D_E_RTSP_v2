//
// Created by ubuntu on 2023/5/10.
//

#ifndef OAK_MPP_D_E_RTSP_H264SERVER_H
#define OAK_MPP_D_E_RTSP_H264SERVER_H

#include <mpp/inc/mpp_packet.h>
#include "xop/RtspServer.h"
#include "net/Timer.h"
 class H264Server {
public:
    H264Server();
    ~H264Server();
    int init_paramter();
    int pushFrame(MppPacket packet,xop::RtspServer* rtsp_server, xop::MediaSessionId session_id);
private:
     FILE * fp_output;


};


#endif //OAK_MPP_D_E_RTSP_H264SERVER_H
