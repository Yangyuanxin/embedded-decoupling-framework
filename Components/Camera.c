#include "Camera.h"

static int fd = -1;
struct buffer *buffers;
static unsigned int n_buffers;

static void errno_exit(const char *s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}

static int init_mmap(const char *dev_name)
{
	struct v4l2_requestbuffers req;

	CLEAR(req);
	req.count = BUFF_COUNT;
	req.type = V4L2_CAP_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == ioctl(fd, VIDIOC_REQBUFS, &req)) 
	{
		fprintf(stderr, "%s does not support "
			        "memory mapping\n", dev_name);
		return -1;
	}

	buffers = calloc(req.count, sizeof(*buffers));
	if (!buffers) 
	{
		fprintf(stderr, "Out of memory\n");
		return -2;
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type        = V4L2_CAP_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = n_buffers;

		if (-1 == ioctl(fd, VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start =
		        mmap(NULL /* start anywhere */,
		             buf.length,
		             PROT_READ | PROT_WRITE /* required */,
		             MAP_SHARED /* recommended */,
		             fd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
		{
			fprintf(stderr, "mmap failed!\n");
			return -3;
		}
	}
	
	return 0;
}

static int init_device(const char *dev_name, int format, int width, int height)
{
	int ret = -1;
	//unsigned int min;
	struct v4l2_capability cap;
	struct v4l2_format fmt;

	if (-1 == ioctl(fd, VIDIOC_QUERYCAP, &cap)) 
	{
		fprintf(stderr, "%s is no V4L2 device\n",
			        dev_name);
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		fprintf(stderr, "%s is no video capture device\n",
		        dev_name);
		return -2;
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) 
	{
		fprintf(stderr, "%s does not support streaming i/o\n",
			    dev_name);
		return -3;
	}
	
	CLEAR(fmt);
	fmt.type = V4L2_CAP_VIDEO_CAPTURE;
	if (-1 == ioctl(fd, VIDIOC_G_FMT, &fmt))
	{
		fprintf(stderr, "%s does VIDIOC_G_FMT failed!\n",
			    dev_name);
		return -5;
	}

	printf("fmt.fmt.pix.bytesperline = %d\n", fmt.fmt.pix.bytesperline);
	printf("fmt.fmt.pix.sizeimage = %d\n", fmt.fmt.pix.sizeimage);
	#if 0
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;
	#endif

	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_CAP_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = format;
	fmt.fmt.pix.field  = V4L2_FIELD_INTERLACED; //视频帧传输的方式为交错式
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;

    // 设置分辨率
    ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
    if (ret < 0) {
        perror("ioctl VIDIOC_S_FMT");
        return -6;
    }

	ret = init_mmap(dev_name);
	if(ret < 0)
	{
		fprintf(stderr, "%s init_mmap failed!\n",
			    dev_name);
		return -7;
	}
	
	return 0;
}

int UsbCameraOpen(struct UsbCameraInfo *Info)
{
	int ret;
	struct stat st;

	if (stat(Info->DevName, &st) < 0)
	{
		fprintf(stderr, "Cannot identify '%s': %d, %s\n",
		        Info->DevName, errno, strerror(errno));
		return -1;
	}

	if (!S_ISCHR(st.st_mode))
	{
		fprintf(stderr, "%s is no device\n", Info->DevName);
		return -2;
	}

	fd = open(Info->DevName, O_RDWR | O_NONBLOCK, 0);
	if (fd < 0) 
	{
		fprintf(stderr, "Cannot open '%s': %d, %s\n",
		        Info->DevName, errno, strerror(errno));
		return -3;
	}
	
	ret = init_device(Info->DevName, Info->Format, Info->Width, Info->Height);
	if(ret < 0)
	{
		fprintf(stderr, "init_device failed!\n");
		return -4;
	}

	return fd;
}

int UsbCameraStopCapture(void)
{
	enum v4l2_buf_type type;
	type = V4L2_CAP_VIDEO_CAPTURE;
	if (-1 == ioctl(fd, VIDIOC_STREAMOFF, &type))
	{
		fprintf(stderr, "VIDIOC_STREAMOFF failed: %d, %s\n",
		        errno, strerror(errno));
		return -1;
	}
	
	return 0;
}

int UsbCameraMunmap(void)
{
	unsigned int i;
	for (i = 0; i < n_buffers; ++i)
	{
		if (-1 == munmap(buffers[i].start, buffers[i].length))
		{
			fprintf(stderr, "munmap failed: %d, %s\n",
		        errno, strerror(errno));
			return -1;
		}
	}
	return 0;
}

int UsbCameraClose(void)
{
	if(close(fd) < 0)
	{
		fprintf(stderr, "close fd failed: %d, %s\n",
		        errno, strerror(errno));
		return -1;
	}
	return 0;
}

int UsbCameraStartCapture(void)
{
	unsigned int i;
	struct v4l2_buffer buf;
	enum v4l2_buf_type type;

	for (i = 0; i < n_buffers; ++i) 
	{
		CLEAR(buf);
		buf.type = V4L2_CAP_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (-1 == ioctl(fd, VIDIOC_QBUF, &buf))
		{
			fprintf(stderr, "VIDIOC_QBUF failed: %d, %s\n",
		        errno, strerror(errno));
			return -1;
		}
	}
	
	type = V4L2_CAP_VIDEO_CAPTURE;
	if (-1 == ioctl(fd, VIDIOC_STREAMON, &type))
	{
		fprintf(stderr, "VIDIOC_STREAMON failed: %d, %s\n",
		        errno, strerror(errno));
		return -2;
	}
	
	return 0;
}

struct buffer UsbCameraGetRawData()
{
	struct v4l2_buffer buf;
	bzero(&buf, sizeof(buf));
	buf.type = V4L2_CAP_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if (-1 == ioctl(fd, VIDIOC_DQBUF, &buf)) 
		perror("Retrieving Frame");

	assert(buf.index < n_buffers);
	struct buffer Buf = {
		.start = buffers[buf.index].start,
		//.length = buffers[buf.index].length
		.length = buf.bytesused
	};
	
	if (-1 == ioctl(fd, VIDIOC_QBUF, &buf))
	{
		perror("Queue buffer");
	}

	return Buf;
}
