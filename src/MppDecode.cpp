//
// Created by LX on 2020/4/25.
//

#include "MppDecode.h"

Queue<cv::Mat> iq0;  //实例化iq
Queue<cv::Mat> iq1;  //实例化iq
Queue<MppPacket> iq2;  //实例化iq
std::mutex mx;  //全局锁mx
std::condition_variable cv_;  //条件变量cvt

MPPDecode::MPPDecode(){

}
MPPDecode::~MPPDecode(){

}
void MPPDecode::dump_mpp_frame_to_file(MppFrame frame, FILE *fp)
{
    RK_U32 width    = 0;
    RK_U32 height   = 0;
    RK_U32 h_stride = 0;
    RK_U32 v_stride = 0;

    MppBuffer buffer    = NULL;
    RK_U8 *base = NULL;

    width    = mpp_frame_get_width(frame);
    height   = mpp_frame_get_height(frame);
    h_stride = mpp_frame_get_hor_stride(frame);
    v_stride = mpp_frame_get_ver_stride(frame);
    buffer   = mpp_frame_get_buffer(frame);

    base = (RK_U8 *)mpp_buffer_get_ptr(buffer);
    RK_U32 buf_size = mpp_frame_get_buf_size(frame);
    size_t base_length = mpp_buffer_get_size(buffer);
    mpp_log("base_length = %d\n",base_length);

    RK_U32 i;
    RK_U8 *base_y = base;
    RK_U8 *base_c = base + h_stride * v_stride;

    //保存为YUV420sp格式
    /*for (i = 0; i < height; i++, base_y += h_stride)
    {
        fwrite(base_y, 1, width, fp);
    }
    for (i = 0; i < height / 2; i++, base_c += h_stride)
    {
        fwrite(base_c, 1, width, fp);
    }*/

    //保存为YUV420p格式
    for(i = 0; i < height; i++, base_y += h_stride)
    {
        fwrite(base_y, 1, width, fp);
    }
    for(i = 0; i < height * width / 2; i+=2)
    {
        fwrite((base_c + i), 1, 1, fp);
    }
    for(i = 1; i < height * width / 2; i+=2)
    {
        fwrite((base_c + i), 1, 1, fp);
    }
}

size_t MPPDecode::mpp_buffer_group_usage(MppBufferGroup group)
{
    if (NULL == group)
    {
        mpp_err_f("input invalid group %p\n", group);
        return MPP_BUFFER_MODE_BUTT;
    }

    MppBufferGroupImpl *p = (MppBufferGroupImpl *)group;
    return p->usage;
}
void MPPDecode::deInit(MppPacket *packet, MppFrame *frame, MppCtx ctx, char *buf, MpiDecLoopData data) {
    if (packet) {
        mpp_packet_deinit(packet);
        packet = NULL;
    }

    if (frame) {
        mpp_frame_deinit(frame);
        frame = NULL;
    }

    if (ctx) {
        mpp_destroy(ctx);
        ctx = NULL;
    }


    if (buf) {
        mpp_free(buf);
        buf = NULL;
    }


    if (data.pkt_grp) {
        mpp_buffer_group_put(data.pkt_grp);
        data.pkt_grp = NULL;
    }

    if (data.frm_grp) {
        mpp_buffer_group_put(data.frm_grp);
        data.frm_grp = NULL;
    }

    if (data.fp_output) {
        fclose(data.fp_output);
        data.fp_output = NULL;
    }

    if (data.fp_input) {
        fclose(data.fp_input);
        data.fp_input = NULL;
    }
}

int MPPDecode::init_decode(){

    //// 初始化
    MPP_RET ret = MPP_OK;
    size_t file_size = 0;

    // base flow context
    MppCtx ctx = NULL;
    MppApi *mpi = NULL;

    // input / output
    MppPacket packet = NULL;
    MppFrame frame = NULL;

    MpiCmd mpi_cmd = MPP_CMD_BASE;
    MppParam param = NULL;
    RK_U32 need_split = 1;
//    MppPollType timeout = 5;

    // paramter for resource malloc
    //RK_U32 width        = 1920;
    //RK_U32 height       = 1080;
    MppCodingType type = MPP_VIDEO_CodingAVC;

    // resources
    char *buf = NULL;
    size_t packet_size = WIDTH * HEIGHT;
    MppBuffer pkt_buf = NULL;
    MppBuffer frm_buf = NULL;



    mpp_log("mpi_dec_test start\n");
    memset(&data, 0, sizeof(data));

//    data.fp_output = fopen("./tenoutput.yuv", "w+b");
//    if (NULL == data.fp_output) {
//        mpp_err("failed to open output file %s\n", "tenoutput.yuv");
//        deInit(&packet, &frame, ctx, buf, data);
//    }

    mpp_log("mpi_dec_test decoder test start w %d h %d type %d\n", WIDTH, HEIGHT, type);

    // decoder demo
    ret = mpp_create(&ctx, &mpi);

    if (MPP_OK != ret) {
        mpp_err("mpp_create failed\n");
        deInit(&packet, &frame, ctx, buf, data);
    }

    // NOTE: decoder split mode need to be set before init
    mpi_cmd = MPP_DEC_SET_PARSER_SPLIT_MODE;
    param = &need_split;
    ret = mpi->control(ctx, mpi_cmd, param);
    if (MPP_OK != ret) {
        mpp_err("mpi->control failed\n");
        deInit(&packet, &frame, ctx, buf, data);
    }

//    mpi_cmd = MPP_SET_INPUT_BLOCK;
//    param = &need_split;
//    ret = mpi->control(ctx, mpi_cmd, param);
//    if (MPP_OK != ret) {
//        mpp_err("mpi->control failed\n");
//        deInit(&packet, &frame, ctx, buf, data);
//    }

    ret = mpp_init(ctx, MPP_CTX_DEC, type);
    if (MPP_OK != ret) {
        mpp_err("mpp_init failed\n");
        deInit(&packet, &frame, ctx, buf, data);
    }

    data.ctx = ctx;
    data.mpi = mpi;

    data.frame = frame;

    return 0;
}
int MPPDecode::decode_simple(AVPacket *av_packet )
{
    data.packet_size = av_packet->size;

    data.eos = 0;
    data.frame_count = 0;
    RK_U32 pkt_done = 0;
    RK_U32 pkt_eos  = 0;
    RK_U32 err_info = 0;
    MPP_RET ret = MPP_OK;
    MppCtx ctx  = data.ctx;
    MppApi *mpi = data.mpi;
    // char   *buf = data->buf;
    MppPacket packet = NULL;
    MppFrame  frame  = NULL;
    size_t read_size = 0;
    size_t packet_size = data.packet_size;


    ret = mpp_packet_init(&packet, av_packet->data, av_packet->size);
    mpp_packet_set_pts(packet, av_packet->pts);


    do {
        RK_S32 times = 5;
        // send the packet first if packet is not done
        if (!pkt_done) {
            ret = mpi->decode_put_packet(ctx, packet);
            if (MPP_OK == ret)
                pkt_done = 1;
        }

        // then get all available frame and release
        do {
         
            RK_S32 get_frm = 0;
            RK_U32 frm_eos = 0;

            try_again:
            ret = mpi->decode_get_frame(ctx, &frame);
            if (MPP_ERR_TIMEOUT == ret) {
                if (times > 0) {
                    times--;
                    msleep(2);
                    goto try_again;
                }
                mpp_err("decode_get_frame failed too much time\n");
            }
            if (MPP_OK != ret) {
                mpp_err("decode_get_frame failed ret %d\n", ret);
                break;
            }

            if (frame) {
           
                if (mpp_frame_get_info_change(frame)) {
                    RK_U32 width = mpp_frame_get_width(frame);
                    RK_U32 height = mpp_frame_get_height(frame);
                    RK_U32 hor_stride = mpp_frame_get_hor_stride(frame);
                    RK_U32 ver_stride = mpp_frame_get_ver_stride(frame);
                    RK_U32 buf_size = mpp_frame_get_buf_size(frame);

                    mpp_log("decode_get_frame get info changed found\n");
                    mpp_log("decoder require buffer w:h [%d:%d] stride [%d:%d] buf_size %d",
                            width, height, hor_stride, ver_stride, buf_size);

                    ret = mpp_buffer_group_get_internal(&data.frm_grp, MPP_BUFFER_TYPE_ION);
                    if (ret) {
                        mpp_err("get mpp buffer group  failed ret %d\n", ret);
                        break;
                    }
                    mpi->control(ctx, MPP_DEC_SET_EXT_BUF_GROUP, data.frm_grp);

                    mpi->control(ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
                } else {
                    err_info = mpp_frame_get_errinfo(frame) | mpp_frame_get_discard(frame);
                    if (err_info) {
                        mpp_log("decoder_get_frame get err info:%d discard:%d.\n",
                                mpp_frame_get_errinfo(frame), mpp_frame_get_discard(frame));
                        printf("frame parse error\n");
                    }
                    data.frame_count++;
                    mpp_log("decode_get_frame get frame %d\n", data.frame_count);
                   // printf("decode_get_frame get frame %d\n", data.frame_count);
                    //                   if(data.fp_output&&!err_info){
//                       dump_mpp_frame_to_file(frame, data.fp_output);
//                   }
                   if (!err_info){
                       cv::Mat rgbImg;
                       YUV420SP2Mat(frame, rgbImg);
                       //printf("normal frame\n");
                       //rgbImg=cv::imread("../test/7.jpg");
                       std::unique_lock<std::mutex> lock(mx);  //类似于智能指针的智能锁
                       cv_.wait(lock, []()->bool {return !iq0.Full(); });  //lambda表达式
                       iq0.Push(rgbImg);  //上述lambda表达式为真退出，所以就不为full时为退出

                       cv_.notify_all();
                      // cv::imwrite("./"+std::to_string(count++)+".jpg", rgbImg);
                     // cv::imshow(".jpg", rgbImg);
                    // cv::waitKey(1);
                    //    dump_mpp_frame_to_file(frame, data->fp_output);
                   }

                }
                frm_eos = mpp_frame_get_eos(frame);
                mpp_frame_deinit(&frame);

                frame = NULL;
                get_frm = 1;
            }

            // try get runtime frame memory usage
            if (data.frm_grp) {
                size_t usage = mpp_buffer_group_usage(data.frm_grp);
                if (usage > data.max_usage)
                    data.max_usage = usage;
            }

            // if last packet is send but last frame is not found continue
            if (pkt_eos && pkt_done && !frm_eos) {
                msleep(10);
                continue;
            }

            if (frm_eos) {
                mpp_log("found last frame\n");
                break;
            }

            if (data.frame_num > 0 && data.frame_count >= data.frame_num) {
                data.eos = 1;
                break;
            }

            if (get_frm)
                continue;
            break;
        } while (1);

        if (data.frame_num > 0 && data.frame_count >= data.frame_num) {
            data.eos = 1;
            mpp_log("reach max frame number %d\n", data.frame_count);
            break;
        }

        if (pkt_done)
            break;

        /*
         * why sleep here:
         * mpi->decode_put_packet will failed when packet in internal queue is
         * full,waiting the package is consumed .Usually hardware decode one
         * frame which resolution is 1080p needs 2 ms,so here we sleep 3ms
         * * is enough.
         */
        msleep(3);
    } while (1);
    mpp_packet_deinit(&packet);

    return ret;
}

void MPPDecode::YUV420SP2Mat(MppFrame  frame, cv::Mat &rgbImg ) {
	RK_U32 width = 0;
	RK_U32 height = 0;
	RK_U32 h_stride = 0;
	RK_U32 v_stride = 0;

	MppBuffer buffer = NULL;
	RK_U8 *base = NULL;

	width = mpp_frame_get_width(frame);
	height = mpp_frame_get_height(frame);
	h_stride = mpp_frame_get_hor_stride(frame);
	v_stride = mpp_frame_get_ver_stride(frame);
	buffer = mpp_frame_get_buffer(frame);

	base = (RK_U8 *)mpp_buffer_get_ptr(buffer);
	RK_U32 buf_size = mpp_frame_get_buf_size(frame);
	size_t base_length = mpp_buffer_get_size(buffer);
	// mpp_log("base_length = %d\n",base_length);

	RK_U32 i;
	RK_U8 *base_y = base;
	RK_U8 *base_c = base + h_stride * v_stride;

	cv::Mat yuvImg;
	yuvImg.create(height * 3 / 2, width, CV_8UC1);

	//转为YUV420p格式
	int idx = 0;
	for (i = 0; i < height; i++, base_y += h_stride) {
		//        fwrite(base_y, 1, width, fp);
		memcpy(yuvImg.data + idx, base_y, width);
		idx += width;
	}
	for (i = 0; i < height / 2; i++, base_c += h_stride) {
		//        fwrite(base_c, 1, width, fp);
		memcpy(yuvImg.data + idx, base_c, width);
		idx += width;
	}
	//这里的转码需要转为RGB 3通道， RGBA四通道则不能检测成功
	cv::cvtColor(yuvImg, rgbImg, cv::COLOR_YUV420sp2RGB);
    putText(rgbImg, "sxj731533730", cv::Point(300, 300), cv::FONT_HERSHEY_COMPLEX, 1.0,cv::Scalar(12, 23, 200), 3, 8);

}
