#ifndef __CAMERA_H
#define __CAMERA_H
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>


//向驱动申请帧缓冲的个数
#define BUFF_COUNT 1
#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct buffer 
{
	void   *start;
	size_t length;
};

struct UsbCameraInfo
{
	char *DevName;
	int Width;
	int Height;
	int Format;
};

//打开摄像头
int UsbCameraOpen(struct UsbCameraInfo *Info);
//关闭摄像头
int UsbCameraClose(void);
//启动摄像头数据流
int UsbCameraStartCapture(void);
//停止摄像头数据流
int UsbCameraStopCapture(void);
//获取摄像头数据流
struct buffer UsbCameraGetRawData();
//停止内存映射
int UsbCameraMunmap(void);


#endif //__CAMERA_H
