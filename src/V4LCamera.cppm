#ifndef USE_V4L_CAMERA
export module V4LCamera;
#else

module;

import <atomic>;
import <cstdio>;
import <cstring>;
import <filesystem>;
import <iostream>;
import <mutex>;
import <string_view>;
import <thread>;
import <functional>;

#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

import Camera;

export module V4LCamera;

#define BUF_NUM 4

struct v4l2_ubuffer {
  void *start;
  unsigned int length;
};

extern struct v4l2_ubuffer *v4l2_ubuffers;

/* functions */
int v4l2_open(const char *device);
int v4l2_close(int fd);
int v4l2_querycap(int fd, const char *device);
int v4l2_sfmt(int fd, uint32_t pfmt);
// int v4l2_gfmt(int fd);
int v4l2_mmap(int fd);
int v4l2_munmap();
int v4l2_sfps(int fd, int fps);
int v4l2_streamon(int fd);
int v4l2_streamoff(int fd);

struct v4l2_ubuffer *v4l2_ubuffers;

int v4l2_open(const char *device) {
    struct stat st;
    std::memset(&st, 0, sizeof(st));
    if (stat(device, &st) == -1) {
	perror("stat");
	return -1;
    }
    if (!S_ISCHR(st.st_mode)) {
	fprintf(stderr, "%s is no character device\n", device);
	return -1;
    }
    return open(device, O_RDWR | O_NONBLOCK, 0);
}

int v4l2_close(int fd) { return close(fd); }

// set format
int v4l2_sfmt(int fd, uint32_t pfmt, uint32_t height, uint32_t width) {
    // set fmt
    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = pfmt;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
	return -1;
    }
    return 0;
}

// get format
// int v4l2_gfmt(int fd) {
//     // set fmt
//     struct v4l2_format fmt;
//     if (ioctl(fd, VIDIOC_G_FMT, &fmt) == -1) {
// 	fprintf(stderr, "Unable to get format\n");
// 	return -1;
//     }
//     printf("\033[33mpix.pixelformat:\t%c%c%c%c\n\033[0m",
// 	   fmt.fmt.pix.pixelformat & 0xFF, (fmt.fmt.pix.pixelformat >> 8) & 0xFF,
// 	   (fmt.fmt.pix.pixelformat >> 16) & 0xFF,
// 	   (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
//     printf("pix.height:\t\t%d\n", fmt.fmt.pix.height);
//     printf("pix.width:\t\t%d\n", fmt.fmt.pix.width);
//     printf("pix.field:\t\t%d\n", fmt.fmt.pix.field);
//     return 0;
// }

int v4l2_sfps(int fd, int fps) {
    struct v4l2_streamparm setfps;
    memset(&setfps, 0, sizeof(setfps));
    setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    setfps.parm.capture.timeperframe.numerator = 1;
    setfps.parm.capture.timeperframe.denominator = fps;
    if (ioctl(fd, VIDIOC_S_PARM, &setfps) == -1) {
	// no fatal error ,just put err msg
	fprintf(stderr, "Unable to set framerate\n");
	return -1;
    }
    return 0;
}

int v4l2_mmap(int fd) {
    // request for 4 buffers
    struct v4l2_requestbuffers req;
    req.count = BUF_NUM;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
	fprintf(stderr, "request for buffers error\n");
	return -1;
    }

    // mmap for buffers
    v4l2_ubuffers = (v4l2_ubuffer *) malloc(req.count * sizeof(struct v4l2_ubuffer));
    if (!v4l2_ubuffers) {
	fprintf(stderr, "Out of memory\n");
	return -1;
    }

    struct v4l2_buffer buf;
    unsigned int n_buffers;
    for (n_buffers = 0; n_buffers < req.count; n_buffers++) {
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = n_buffers;
	// query buffers
	if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
	    fprintf(stderr, "query buffer error\n");
	    return -1;
	}

	v4l2_ubuffers[n_buffers].length = buf.length;
	// map 4 buffers in driver space to usersapce
	v4l2_ubuffers[n_buffers].start = mmap(
	    NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
#ifdef DEBUG
	printf("buffer offset:%d\tlength:%d\n", buf.m.offset, buf.length);
#endif
	/**
	 *  output:
	 *  buffer offset:0	length:614400
	 *  buffer offset:614400	length:614400
	 *  buffer offset:1228800	length:614400
	 *  buffer offset:1843200	length:614400
	 *
	 *  explanation：saved in YUV422 format，a pixel needs 2 byte storage in
	 *  average，as our image size is 640*480. 640*480*2=614400
	 */
	if (v4l2_ubuffers[n_buffers].start == MAP_FAILED) {
	    fprintf(stderr, "buffer map error %u\n", n_buffers);
	    return -1;
	}
    }
    return 0;
}

int v4l2_munmap() {
    int i;
    for (i = 0; i < BUF_NUM; i++) {
	if (munmap(v4l2_ubuffers[i].start, v4l2_ubuffers[i].length) == -1) {
	    fprintf(stderr, "munmap failure %d\n", i);
	    return -1;
	}
    }
    return 0;
}

int v4l2_streamon(int fd) {
    // queue in the four buffers allocated by VIDIOC_REQBUFS, pretty like water
    // filling a bottle in turn
    struct v4l2_buffer buf;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    unsigned int n_buffers;
    for (n_buffers = 0; n_buffers < BUF_NUM; n_buffers++) {
	buf.index = n_buffers;
	if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
	    fprintf(stderr, "queue buffer failed\n");
	    return -1;
	}
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
	fprintf(stderr, "stream on failed\n");
	return -1;
    }
    return 0;
}

int v4l2_streamoff(int fd) {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(fd, VIDIOC_STREAMOFF, &type)) {
	fprintf(stderr, "stream off failed\n");
	return -1;
    }
    return 0;
}

export class V4LCamera : public Camera {
public:
    std::atomic_bool running;
    int fd{};
    bool streamOn{};
    bool mmapOn{};
    std::string error;
    std::thread thread;

    OnFrameCallback onFrame = [](const void*, std::size_t){};
    std::mutex onFrameMutex;

    std::string _name;

    std::size_t _width = 640;
    std::size_t _height = 480;

    std::size_t width() override {return _width;}
    std::size_t height() override {return _height;}
    const std::string& name() override {return _name;}

    V4LCamera(const std::filesystem::path& path) {
	std::cout << "On file " << path << std::endl;
	fd = v4l2_open(path.c_str());
	if (!fd) {
	    error = "Could not open " + path.string();
	    return;
	}

	querycap(path.c_str());

	if (v4l2_sfmt(fd, V4L2_PIX_FMT_YUYV, height(), width()) == -1) {
	    error = "Could not set SFMT";
	    return;
	}

	if (gfmt() == -1) {
	    error = "Could not get FMT";
	    return;
	}

	v4l2_sfps(fd, 30);

	if (v4l2_mmap(fd) == -1) {
	    error = "Could not MMAP";
	    return;
	}

	mmapOn = true;

	if (v4l2_streamon(fd) == -1) {
	    error = "Could not stream";
	    return;
	}

	streamOn = true;
	running = true;
	thread = std::thread{[&]{stream();}};
    }

    int gfmt() {
	// set fmt
	struct v4l2_format fmt;
	fmt.type = 1; // V4L2_BUF_TYPE_VIDEO_CAPTURE
	if (ioctl(fd, VIDIOC_G_FMT, &fmt) == -1) {
	    fprintf(stderr, "Unable to get format\n");
	    return -1;
	}
	_height = fmt.fmt.pix.height;
	_width = fmt.fmt.pix.width;
	printf("\033[33mpix.pixelformat:\t%c%c%c%c\n\033[0m",
	       fmt.fmt.pix.pixelformat & 0xFF, (fmt.fmt.pix.pixelformat >> 8) & 0xFF,
	       (fmt.fmt.pix.pixelformat >> 16) & 0xFF,
	       (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
	printf("pix.height:\t\t%d\n", fmt.fmt.pix.height);
	printf("pix.width:\t\t%d\n", fmt.fmt.pix.width);
	printf("pix.field:\t\t%d\n", fmt.fmt.pix.field);
	return 0;
    }

    int querycap(const char* device) {
	struct v4l2_capability cap;
	if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
	    printf("Error opening device %s: unable to query device.\n", device);
	    return -1;
	} else {
	    _name = reinterpret_cast<const char*>(cap.card);
	    // printf("driver:\t\t%s\n", cap.driver);
	    // printf("card:\t\t%s\n", cap.card);
	    // printf("bus_info:\t%s\n", cap.bus_info);
	    // printf("version:\t%d\n", cap.version);
	    // printf("capabilities:\t%x\n", cap.capabilities);
	    // if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE)
	    // 	printf("Device %s: supports capture.\n", device);
	    // if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING)
	    // 	printf("Device %s: supports streaming.\n", device);
	}

	// emu all support fmt
	// struct v4l2_fmtdesc fmtdesc;
	// fmtdesc.index = 0;
	// fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	// printf("\033[31mSupport format:\n\033[0m");
	// while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1) {
	//     printf("\033[31m\t%d.%s\n\033[0m", fmtdesc.index + 1, fmtdesc.description);
	//     fmtdesc.index++;
	// }
	return 0;
    }

    void setOnFrame(const OnFrameCallback& onFrame) override {
	std::lock_guard<std::mutex> lock{onFrameMutex};
	this->onFrame = onFrame;
    }

    void stream() {
	fd_set fds;
	struct v4l2_buffer buf;
	while (running) {
	    FD_ZERO(&fds);
	    FD_SET(fd, &fds);
	    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
	    auto ret = select(fd + 1, &fds, NULL, NULL, &tv);
	    if (ret == -1) {
		fprintf(stderr, "select error\n");
		return;
	    } else if (ret == 0) {
		fprintf(stderr, "timeout waiting for frame\n");
		continue;
	    }
	    if (FD_ISSET(fd, &fds)) {
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
		    fprintf(stderr, "VIDIOC_DQBUF failure\n");
		    return;
		}

		{
		    std::lock_guard<std::mutex> lock{onFrameMutex};
		    onFrame(v4l2_ubuffers[buf.index].start, v4l2_ubuffers[buf.index].length);
		}

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
		    fprintf(stderr, "VIDIOC_QBUF failure\n");
		    return;
		}
	    }
	}
    }

    ~V4LCamera() {
	running = false;
	if (thread.joinable())
	    thread.join();

	if (streamOn)
	    v4l2_streamoff(fd);

	if (mmapOn)
	    v4l2_munmap();

	if (fd)
	    v4l2_close(fd);
    }

    bool good() override {
	return error.empty();
    }

    const std::string& getError() override {
	return error;
    }

    class V4LReference : public Camera::Reference {
    public:
	std::filesystem::path path;

	V4LReference(const std::filesystem::path& path) : path{path} {}

	std::unique_ptr<Camera> create(const Camera::OnFrameCallback& cb) {
	    auto camera = std::make_unique<V4LCamera>(path);
	    camera->setOnFrame(cb);
	    return camera;
	}
    };

    class V4LPoller : public Camera::Poller {
	std::vector<std::unique_ptr<Camera::Reference>> poll() override {
	    std::filesystem::path dev{"/dev"};
	    std::vector<std::unique_ptr<Camera::Reference>> refs;
	    for (auto& file : std::filesystem::directory_iterator{dev}) {
		if (!file.is_character_file()) continue;
		auto filename = file.path().filename().string();
		if (std::string_view{filename}.substr(0, 5) != "video") continue;
		refs.emplace_back(std::make_unique<V4LReference>(file.path()));
	    }
	    return refs;
	}
    };
};

Camera::Poller::Register<V4LCamera::V4LPoller> _;

#endif
