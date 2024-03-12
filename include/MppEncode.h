//
// Created by ubuntu on 2023/5/4.
//

#ifndef MPP_TEST_MPPENCODE_H
#define MPP_TEST_MPPENCODE_H
//#define MODULE_TAG "mpi_enc_test"
#include <string.h>
#include "rk_mpi.h"
#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_time.h"
#include "mpp_common.h"
#include "utils.h"
#include "mpi_enc_utils.h"
#include "camera_source.h"
#include "mpp_enc_roi_utils.h"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "iostream"
#include "H264Server.h"
#include <chrono>
#include <iostream>
#include "iomanip"
using namespace std;
using namespace chrono;

class MppEncode {
public:
    MppEncode();
    ~MppEncode();
    int init_paramter();
    int yuv_enc_h264(H264Server *h264server_item,xop::RtspServer* rtsp_server, xop::MediaSessionId session_id);
    int destory();
private:
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    int counter = 0;
    float fps = 0;
    FILE * fp_output;
    int cfg_frame_width ;
    int cfg_frame_height ;
    MppBufferGroup buf_grp;
    MppBuffer frm_buf;
    MppBuffer pkt_buf;
    MppPollType timeout = MPP_POLL_BLOCK;
    MppEncCfg cfg;
    MppFrame frame = NULL;
    int cfg_bps = 600000;
    int cfg_bps_min = 400000;
    int cfg_bps_max = 800000;
    int cfg_rc_mode = 0;	//0:vbr 1:cbr 2:avbr 3:cvbr 4:fixqp;
    int cfg_frame_num = 0;	//max encoding frame number
    std::mutex m_mtx;

    int cfg_gop_mode = 0;
    int cfg_gop_len = 0;
    int cfg_vi_len = 0;
    int cfg_fps_in_flex = 0;
    int cfg_fps_in_den = 1;
    int cfg_fps_in_num = 30;	// input fps
    int cfg_fps_out_flex = 0;
    int cfg_fps_out_den = 1;
    int cfg_fps_out_num = 30;
    int cfg_frame_size = 0;
    int cfg_header_size = 0;
    int cfg_quiet = 0;
    MppApi * mpi=NULL;
    MppCtx ctx=NULL;
    MppFrameFormat cfg_foramt = MPP_FMT_YUV420P;	// format of input frame YUV420P MppFrameFormat inside mpp_frame.h
    MppCodingType cfg_type = MPP_VIDEO_CodingAVC;
    RK_S32 chn = 1;
    RK_U32 cap_num = 0;
    RK_U32 cfg_split_mode = 0;
    RK_U32 cfg_split_arg = 0;

};


#endif //MPP_TEST_MPPENCODE_H
