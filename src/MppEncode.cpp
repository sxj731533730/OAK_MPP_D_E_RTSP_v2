//
// Created by ubuntu on 2023/5/4.
//

#include "MppEncode.h"
#include "common.h"

extern Queue<cv::Mat> iq0;  //实例化iq
extern Queue<cv::Mat> iq1;  //实例化iq
extern Queue<MppPacket> iq2;  //实例化iq
extern std::mutex mx;  //全局锁mx
extern std::condition_variable cv_;  //条件变量cv
MppEncode::MppEncode() {

}

MppEncode::~MppEncode() {

}

int MppEncode::init_paramter( ) {
    RK_S32 ret = MPP_NOK;

    cfg_frame_width = WIDTH;
    cfg_frame_height = HEIGHT;
    int cfg_hor_stride = MPP_ALIGN(cfg_frame_width, 8);    // for YUV420P it has tobe devided to 16
    int cfg_ver_stride = MPP_ALIGN(cfg_frame_height, 8);    // for YUV420P it has tobe devided to 16


    // update resource parameter
    switch (cfg_foramt & MPP_FRAME_FMT_MASK) {
        case MPP_FMT_YUV420SP:
        case MPP_FMT_YUV420P: {
            cfg_frame_size = MPP_ALIGN(cfg_hor_stride, 8) * MPP_ALIGN(cfg_ver_stride, 8) * 3 / 2;

        }

            break;
    }

    if (MPP_FRAME_FMT_IS_FBC(cfg_foramt))
        cfg_header_size = MPP_ALIGN(MPP_ALIGN(cfg_frame_width, 8) * MPP_ALIGN(cfg_frame_height, 8) / 8, SZ_4K);
    else
        cfg_header_size = 0;

    ret = mpp_buffer_group_get_internal(&buf_grp, MPP_BUFFER_TYPE_DRM);
    if (ret) {
        mpp_err_f("failed to get mpp buffer group ret %d\n", ret);
        return -1;
    }

    ret = mpp_buffer_get(buf_grp, &frm_buf, cfg_frame_size + cfg_header_size);
    if (ret) {
        mpp_err_f("failed to get buffer for input frame ret %d\n", ret);
        return -1;
    }
    ret = mpp_buffer_get(buf_grp, &pkt_buf, cfg_frame_size);
    if (ret) {
        mpp_err_f("failed to get buffer for output packet ret %d\n", ret);
        return -1;
    }

    ret = mpp_create(&ctx, &mpi);
    if (ret) {
        mpp_err("mpp_create failed ret %d\n", ret);
        return -1;
    }

    mpp_log_q(cfg_quiet, "%p encoder test start w %d h %d type %d\n",
              ctx, cfg_frame_width, cfg_frame_height, cfg_type);

    ret = mpi->control(ctx, MPP_SET_OUTPUT_TIMEOUT, &timeout);
    if (MPP_OK != ret) {
        mpp_err("mpi control set output timeout %d ret %d\n", timeout, ret);
        return -1;
    }

    ret = mpp_init(ctx, MPP_CTX_ENC, cfg_type);
    if (ret) {
        mpp_err("mpp_init failed ret %d\n", ret);
        return -1;
    }

    ret = mpp_enc_cfg_init(&cfg);
    if (ret) {
        mpp_err_f("mpp_enc_cfg_init failed ret %d\n", ret);
        return -1;
    }

    //configuring the ctx
    mpp_enc_cfg_set_s32(cfg, "prep:width", cfg_frame_width);
    mpp_enc_cfg_set_s32(cfg, "prep:height", cfg_frame_height);
    mpp_enc_cfg_set_s32(cfg, "prep:hor_stride", cfg_hor_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:ver_stride", cfg_ver_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:format", cfg_foramt);

    mpp_enc_cfg_set_s32(cfg, "rc:mode", cfg_rc_mode);

    /*fix input / output frame rate */
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_flex", cfg_fps_in_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num", cfg_fps_in_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_denorm", cfg_fps_in_den);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_flex", cfg_fps_out_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num", cfg_fps_out_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_denorm", cfg_fps_out_den);
    mpp_enc_cfg_set_s32(cfg, "rc:gop", cfg_gop_len ? cfg_gop_len : cfg_fps_out_num * 2);

    /*drop frame or not when bitrate overflow */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_mode", MPP_ENC_RC_DROP_FRM_DISABLED);
    mpp_enc_cfg_set_u32(cfg, "rc:drop_thd", 20); /*20% of max bps */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_gap", 1); /*Do not continuous drop frame */

    /*setup bitrate for different rc_mode */
    mpp_enc_cfg_set_s32(cfg, "rc:bps_target", cfg_bps);
    switch (cfg_rc_mode) {
        case MPP_ENC_RC_MODE_FIXQP: {    /*do not setup bitrate on FIXQP mode */
        }
            break;
        case MPP_ENC_RC_MODE_CBR: {    /*CBR mode has narrow bound */
            mpp_enc_cfg_set_s32(cfg, "rc:bps_max", cfg_bps_max ? cfg_bps_max : cfg_bps * 17 / 16);
            mpp_enc_cfg_set_s32(cfg, "rc:bps_min", cfg_bps_min ? cfg_bps_min : cfg_bps * 15 / 16);
        }
            break;
        case MPP_ENC_RC_MODE_VBR:
        case MPP_ENC_RC_MODE_AVBR: {    /*VBR mode has wide bound */
            mpp_enc_cfg_set_s32(cfg, "rc:bps_max", cfg_bps_max ? cfg_bps_max : cfg_bps * 17 / 16);
            mpp_enc_cfg_set_s32(cfg, "rc:bps_min", cfg_bps_min ? cfg_bps_min : cfg_bps * 1 / 16);
        }
            break;
        default: {    /*default use CBR mode */
            mpp_enc_cfg_set_s32(cfg, "rc:bps_max", cfg_bps_max ? cfg_bps_max : cfg_bps * 17 / 16);
            mpp_enc_cfg_set_s32(cfg, "rc:bps_min", cfg_bps_min ? cfg_bps_min : cfg_bps * 15 / 16);
        }
            break;
    }

    /*setup qp for different codec and rc_mode */
    switch (cfg_type) {
        case MPP_VIDEO_CodingAVC:
        case MPP_VIDEO_CodingHEVC: {
            switch (cfg_rc_mode) {
                case MPP_ENC_RC_MODE_FIXQP: {
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_init", 20);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_max", 20);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_min", 20);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", 20);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", 20);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 2);
                }
                    break;
                case MPP_ENC_RC_MODE_CBR:
                case MPP_ENC_RC_MODE_VBR:
                case MPP_ENC_RC_MODE_AVBR: {
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_init", 26);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_max", 51);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_min", 10);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", 51);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", 10);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 2);
                }
                    break;
                default: {
                    mpp_err_f("unsupport encoder rc mode %d\n", cfg_rc_mode);
                }
                    break;
            }
        }
            break;
        case MPP_VIDEO_CodingVP8: {    /*vp8 only setup base qp range */
            mpp_enc_cfg_set_s32(cfg, "rc:qp_init", 40);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max", 127);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min", 0);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", 127);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", 0);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 6);
        }
            break;
        case MPP_VIDEO_CodingMJPEG: {    /*jpeg use special codec config to control qtable */
            mpp_enc_cfg_set_s32(cfg, "jpeg:q_factor", 80);
            mpp_enc_cfg_set_s32(cfg, "jpeg:qf_max", 99);
            mpp_enc_cfg_set_s32(cfg, "jpeg:qf_min", 1);
        }
            break;
        default: {
        }
            break;
    }
    /*setup codec  */
    mpp_enc_cfg_set_s32(cfg, "codec:type", cfg_type);
    switch (cfg_type) {
        case MPP_VIDEO_CodingAVC: {    /*
				 *H.264 profile_idc parameter
				 *66  - Baseline profile
				 *77  - Main profile
				 *100 - High profile
				 */
            mpp_enc_cfg_set_s32(cfg, "h264:profile", 100);
            /*
             *H.264 level_idc parameter
             *10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
             *20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
             *30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
             *40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
             *50 / 51 / 52         - 4K@30fps
             */
            mpp_enc_cfg_set_s32(cfg, "h264:level", 40);
            mpp_enc_cfg_set_s32(cfg, "h264:cabac_en", 1);
            mpp_enc_cfg_set_s32(cfg, "h264:cabac_idc", 0);
            mpp_enc_cfg_set_s32(cfg, "h264:trans8x8", 0);
        }
            break;
        case MPP_VIDEO_CodingHEVC:
        case MPP_VIDEO_CodingMJPEG:
        case MPP_VIDEO_CodingVP8: {
        }
            break;
        default: {
            mpp_err_f("unsupport encoder coding type %d\n", cfg_type);
        }
            break;
    }

    mpp_env_get_u32("split_mode", &cfg_split_mode, MPP_ENC_SPLIT_NONE);
    mpp_env_get_u32("split_arg", &cfg_split_arg, 0);

    ret = mpi->control(ctx, MPP_ENC_SET_CFG, cfg);
    if (ret) {
        mpp_err("mpi control enc set cfg failed ret %d\n", ret);
        return -1;
    }

    MppEncSeiMode cfg_sei_mode = MPP_ENC_SEI_MODE_ONE_FRAME;
    ret = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &cfg_sei_mode);
    if (ret) {
        mpp_err("mpi control enc set sei cfg failed ret %d\n", ret);
        return -1;
    }

    if (cfg_type == MPP_VIDEO_CodingAVC || cfg_type == MPP_VIDEO_CodingHEVC) {
        MppEncHeaderMode cfg_header_mode = MPP_ENC_HEADER_MODE_EACH_IDR;
        ret = mpi->control(ctx, MPP_ENC_SET_HEADER_MODE, &cfg_header_mode);
        if (ret) {
            mpp_err("mpi control enc set header mode failed ret %d\n", ret);
            return -1;
        }
    }

    RK_U32 gop_mode = cfg_gop_mode;

    mpp_env_get_u32("gop_mode", &gop_mode, gop_mode);
    if (gop_mode) {
        MppEncRefCfg ref;

        mpp_enc_ref_cfg_init(&ref);

        if (cfg_gop_mode < 4)
            mpi_enc_gen_ref_cfg(ref, gop_mode);
        else
            mpi_enc_gen_smart_gop_ref_cfg(ref, cfg_gop_len, cfg_vi_len);

        ret = mpi->control(ctx, MPP_ENC_SET_REF_CFG, ref);
        if (ret) {
            mpp_err("mpi control enc set ref cfg failed ret %d\n", ret);
        }
        mpp_enc_ref_cfg_deinit(&ref);
    }

    RK_U32 cfg_osd_enable, cfg_osd_mode, cfg_roi_enable, cfg_user_data_enable;
    /*setup test mode by env */
    mpp_env_get_u32("osd_enable", &cfg_osd_enable, 0);
    mpp_env_get_u32("osd_mode", &cfg_osd_mode, MPP_ENC_OSD_PLT_TYPE_DEFAULT);
    mpp_env_get_u32("roi_enable", &cfg_roi_enable, 0);
    mpp_env_get_u32("user_data_enable", &cfg_user_data_enable, 0);

    //fp_output = fopen("sample.h264", "wb");

//    DataCrc checkcrc;
//    ret = MPP_OK;
//    memset(&checkcrc, 0, sizeof(checkcrc));
//    checkcrc.sum = mpp_malloc(RK_ULONG, 512);
//
//    if (cfg_type == MPP_VIDEO_CodingAVC || cfg_type == MPP_VIDEO_CodingHEVC)
//    {
//        MppPacket packet = NULL;
//
//        /*
//         *Can use packet with normal malloc buffer as input not pkt_buf.
//         *Please refer to vpu_api_legacy.cpp for normal buffer case.
//         *Using pkt_buf buffer here is just for simplifing demo.
//         */
//        mpp_packet_init_with_buffer(&packet, pkt_buf);
//        /*NOTE: It is important to clear output packet length!! */
//        mpp_packet_set_length(packet, 0);
//
//        ret = mpi->control(ctx, MPP_ENC_GET_HDR_SYNC, packet);
//        if (ret)
//        {
//            mpp_err("mpi control enc get extra info failed\n");
//        }
//        else
//        { /*get and write sps/pps for H.264 */
//
//            void *ptr = mpp_packet_get_pos(packet);
//            size_t len = mpp_packet_get_length(packet);
//
//            //if (p->fp_output)
//            fwrite(ptr, 1, len, fp_output);
//        }
//
//        mpp_packet_deinit(&packet);
//    }



    ret = mpp_frame_init(&frame);
    if (ret) {
        printf("mpp_frame_init failed\n");
        return -1;
    }


    mpp_frame_set_width(frame, cfg_frame_width);
    mpp_frame_set_height(frame, cfg_frame_height);
    mpp_frame_set_hor_stride(frame, cfg_hor_stride);
    mpp_frame_set_ver_stride(frame, cfg_ver_stride);
    mpp_frame_set_fmt(frame, cfg_foramt);
    mpp_frame_set_eos(frame, 0);



    return 0;
}

int MppEncode::yuv_enc_h264(H264Server *h264server_item,xop::RtspServer* rtsp_server, xop::MediaSessionId session_id) {
    cv::Mat bgr_frame;

    int ret=-1;

    // ret = item_reader->read_data(frame, "EncodeH264");
    {
        std::unique_lock<std::mutex> lock(mx);
        cv_.wait(lock, []() -> bool { return !iq0.Empty(); });//lambda表达式中为真退出等待不为NULL时，退出wait

        iq0.Front(bgr_frame);
        cv_.notify_all();
    }
//毫秒级
    counter++;
    auto currentTime = steady_clock::now();
    auto elapsed = duration_cast<duration<float>>(currentTime - startTime);
    if (elapsed > seconds(1)) {
        fps = counter / elapsed.count();
        counter = 0;
        startTime = currentTime;
    }
    std::stringstream fpsStr;
    fpsStr << "NN fps: " << std::fixed << std::setprecision(2) << fps;
    cv::putText(bgr_frame, fpsStr.str(), cv::Point(2,  40), cv::FONT_HERSHEY_TRIPLEX, 1, cv::Scalar(0,255,0));

    //bgr_frame=cv::imread("dd.jpg");
    cv::resize(bgr_frame, bgr_frame, cv::Size(cfg_frame_width, cfg_frame_height), 0, 0, cv::INTER_LINEAR);
    //cv::imwrite("dd.jpg",bgr_frame);
   // std::cout << "视频宽度： " << cfg_frame_width << std::endl;
  //  std::cout << "视频高度： " << cfg_frame_height << std::endl;
    MppMeta meta = NULL;
    MppPacket packet = NULL;
    //void *buf = mpp_buffer_get_ptr(frm_buf);
    RK_S32 cam_frm_idx = -1;
    MppBuffer cam_buf = NULL;
    RK_U32 eoi = 1;
    cv::Mat yuv_frame;
    cvtColor(bgr_frame, yuv_frame, cv::COLOR_BGR2YUV_I420);

  //  printf("mpp_buffer_write\n");

     ret = mpp_buffer_write(frm_buf, 0, yuv_frame.data, cfg_frame_size);


    if (ret) {
        printf("could not write data on frame\n", chn);
       return -1;
    }
    //printf("mpp_buffer_write 4\n");
    mpp_frame_set_buffer(frame, frm_buf);
    meta = mpp_frame_get_meta(frame);
    mpp_packet_init_with_buffer(&packet, pkt_buf);
    /*NOTE: It is important to clear output packet length!! */
    mpp_packet_set_length(packet, 0);
    mpp_meta_set_packet(meta, KEY_OUTPUT_PACKET, packet);

    ret = mpi->encode_put_frame(ctx, frame);
    if (ret) {
        printf("chn %d encode put frame failed\n", chn);
        //mpp_frame_deinit(&frame);
    }
    //mpp_frame_deinit(&frame);

    do {
        ret = mpi->encode_get_packet(ctx, &packet);
        if (ret) {
            printf("chn %d encode get packet failed\n", chn);
        }

        mpp_assert(packet);
        if (packet) {
            // write packet to file here
            void *ptr = mpp_packet_get_pos(packet);
            size_t len = mpp_packet_get_length(packet);
            //fwrite(ptr, 1, len, fp_output);
            //std::cout << "write packet to file len:" << len << "\n";
            h264server_item->pushFrame(packet,rtsp_server, session_id);
            //std::cout << "write packet to file here.len:" << len << "\n";
            if (mpp_packet_is_partition(packet)) {
                eoi = mpp_packet_is_eoi(packet);
            }
        } else {
            std::cout << "there is no packet\n";
        }
        mpp_packet_deinit(&packet);
        //std::cout << "new packet \n";
    } while (!eoi);
    return 0;
}

int MppEncode::destory() {

    //if (frame!=NULL)
//    {
//      mpp_frame_deinit(&frame);
//        frame = NULL;
//    }

    if (ctx) {
        mpp_destroy(ctx);
        ctx = NULL;
    }

    if (cfg) {
        mpp_enc_cfg_deinit(cfg);
        cfg = NULL;
    }

    if (frm_buf) {
        mpp_buffer_put(frm_buf);
        frm_buf = NULL;
    }

    if (pkt_buf) {
        mpp_buffer_put(pkt_buf);
        pkt_buf = NULL;
    }

    if (buf_grp) {
        mpp_buffer_group_put(buf_grp);
        buf_grp = NULL;
    }
    fclose(fp_output);

    return 0;
}