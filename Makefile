CPP = g++
CPPFLAGS = -Wall #-fsanitize=address -g
LDFLAGS = -L/home/nvidia/Gantsev_07_2023/OtherLibsLinux/XimeaAPI/api/Xarm64 -L/home/nvidia/Gantsev_07_2023/OtherLibsLinux/FastvideoSDK/fastvideo_sdk/lib/
INCLUDES = -I/home/nvidia/Gantsev_07_2023/OtherLibsLinux/XimeaAPI/include -I/home/nvidia/Gantsev_07_2023/OtherLibsLinux/FastvideoSDK/fastvideo_sdk/inc/ -I/home/nvidia/Gantsev_07_2023/OtherLibsLinux/FastvideoSDK/common/helper_jpeg/ -I/home/nvidia/Gantsev_07_2023/OtherLibsLinux/FastvideoSDK/common/
LDLIBS = -std=c++11 -lm3api -lpthread -lfastvideo_sdk

SRCS = camera.cpp
OBJS = $(SRCS:.cpp = .o)

TARGET = camera

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CPP) $(CPPFLAGS) $(LDFLAGS) $(INCLUDES) $^ -o $@ $(LDLIBS)

%.o: %.cpp
	$(CPP) $(CPPFLAGS) $^ -o $@

clean:
	rm -f $(TARGET)
