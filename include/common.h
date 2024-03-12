//
// Created by ubuntu on 2022/11/15.
//

#ifndef DEPTHAI_COMMON_H
#define DEPTHAI_COMMON_H
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>

#ifdef __cplusplus
};
#endif
#include <condition_variable>


#define WIDTH 1920
#define HEIGHT 1080
template<class T>  //模板类
class Queue
{
    enum { QUSIZE = 4 };  //循环队列大小为8

    T* data;   //指针指向循环队列连续空间
    int front;  //队头
    int rear;  //队尾
    int size;  //当前队列的元素个数
    int maxsize;  //队列最大大小
public:
    Queue() :data(nullptr), front(0), rear(0), size(0), maxsize(QUSIZE)
    {
        data = new T[maxsize];
    }
    ~Queue()
    {
        free(data);
        data = nullptr;
        front = rear = -1;
        size = 0;
        maxsize = 0;
    }

    int Capt() const { return maxsize; }    //求队列最大元素个数的函数
    int Size() const { return size; }   //求现有元素个数的函数
    bool Empty() const { return Size() == 0; }  //判空函数
    bool Full() const {             //判满函数
        return Size() == maxsize;
    }
    bool Push(const T& val)     //入队函数
    {
        if (Full()) return false;
        data[rear] = val;
        rear = (rear + 1) % maxsize;  //上面说到最大值为8，也就是说存储下标为0到7
        size += 1;
        return true;
    }
    bool Front(T& val)  //出队函数
    {
        if (Empty()) return false;
        val = data[front];
        front = (front + 1) % maxsize;//上面说到最大值为8，也就是说存储下标为0到7
        size -= 1;
        return true;
    }
};





#endif //DEPTHAI_COMMON_H