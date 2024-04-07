#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <sys/ipc.h>
#include <sys/types.h>

#include <m3api/xiApi.h>
#include "fastvideo_sdk.h"
#include "helper_jpeg.hpp"
#include "helper_jpeg_store.cpp"

#define CAMERANUM 4
#define BUF 1024

int main() {
	int i = 0;
	int j = 0;
	int imageCounts = 0;

	int maxWidth[CAMERANUM];
	int maxHeight[CAMERANUM];
	HANDLE hCamera[CAMERANUM];
	unsigned int uCameraCounts;
	int iCameraCounts;
	
	std::string camera = "/camera_";
	std::string image = "_image_";
	std::string dot = "./";
	std::string folder = "folder";
	std::string jpg = ".jpg";
        std::stringstream str;

	fastStatus_t fast_status = FAST_OK;
	XI_RETURN xi_status = XI_OK;
	XI_IMG xi_image[CAMERANUM];

	unsigned affinity = 1;
	bool openGlMode = false;
	fast_status = fastInit(affinity, openGlMode);
	std::cout << "FastvideoSDK was initialized. Status: " << fast_status << std::endl;

	xi_status = xiGetNumberDevices(&uCameraCounts);
	iCameraCounts = (int) uCameraCounts;
	std::cout << "GetNumberDevices status: " << xi_status << std::endl;

	std::cout << "Number of cameras: " << iCameraCounts << std::endl;

	if ((iCameraCounts == 0) || (xi_status != XI_OK)){
		return -1;
	}
		
	for (i = 0; i < iCameraCounts; i++) {	
		xi_status = xiOpenDevice(i, &hCamera[i]);
		std::cout << "Initialization of " << i << " camera. Status = " << xi_status << std::endl;
				
		if(xi_status != XI_OK) {
			return -1;
		}
	}

    	for (i = 0; i < iCameraCounts; i++) {
		xiGetParamInt(hCamera[i], XI_PRM_WIDTH, &maxWidth[i]);
		xiGetParamInt(hCamera[i], XI_PRM_HEIGHT, &maxHeight[i]);
		std::cout << "Get resolution of camera: " << maxWidth[i] << " " << maxHeight[i] << std::endl;
	}

	int buffer_size = 32;
	
	for (i = 0; i < iCameraCounts; i++) {
		xi_status = xiGetParamInt(hCamera[i], XI_PRM_ACQ_TRANSPORT_BUFFER_SIZE XI_PRM_INFO_MAX, &buffer_size);
		std::cout << "Getting max buffer size for USB. Status: " << xi_status << std::endl;
		xi_status = xiSetParamInt(hCamera[i], XI_PRM_ACQ_TRANSPORT_BUFFER_SIZE, buffer_size);
		std::cout << "Setting max buffer size for USB. Status: " << xi_status << std::endl;
	}

        unsigned channelCount = 1;

        for (i = 0; i < iCameraCounts; i++) {
                int imageDataFormat = 0;
                xi_status = xiSetParamInt(hCamera[i], XI_PRM_IMAGE_DATA_FORMAT, XI_RAW8);
                std::cout << "Setting image data format. Status: " << xi_status << std::endl;
                xi_status = xiGetParamInt(hCamera[i], XI_PRM_IMAGE_DATA_FORMAT, &imageDataFormat);
                std::cout << "Getting image data format. Status: " << xi_status << ". Image data format: " << imageDataFormat << std::endl;

                if (imageDataFormat == XI_RAW8) {
                        channelCount = 1;
                        std::cout << "Camera " << i << " image data format RAW8." << std::endl;
                } else {
                        std::cout << "Camera " << i << " image data format not RAW8." << std::endl;
                        return -1;
                }
        }

	fastImportFromHostHandle_t handle[CAMERANUM];
	fastDeviceSurfaceBufferHandle_t dstBuffer[CAMERANUM];
        fastSurfaceFormat_t surfaceFmt = FAST_I8;

	for (i = 0; i < iCameraCounts; i++) {
		fast_status = fastImportFromHostCreate(&handle[i], surfaceFmt, maxWidth[i], maxHeight[i], &dstBuffer[i]);
		std::cout << "Adapter for import from host on camera_" << i << " was created. Status: " << fast_status << std::endl;
	}

	size_t requestedGpuSizeInBytes;
	
	for (i = 0; i < iCameraCounts; i++) {
		fast_status = fastImportFromHostGetAllocatedGpuMemorySize(handle[i], &requestedGpuSizeInBytes);
		std::cout << "fastImportFromHostGetAllocatedGpuMemorySize on camera_" << i << " was performed. Memory size : " 
			<< requestedGpuSizeInBytes << ". Status: " << fast_status << std::endl;
	}

        fastJpegEncoderHandle_t jpegEncoderHandle[CAMERANUM];
        fastDeviceSurfaceBufferHandle_t srcBuffer[CAMERANUM];
	
 	for (i = 0; i < iCameraCounts; i++) {	
		srcBuffer[i] = dstBuffer[i];
        	fast_status = fastJpegEncoderCreate (&jpegEncoderHandle[i], maxWidth[i], maxHeight[i], srcBuffer[i]);
		std::cout << "Encoder for JPEG on camera_" << i << " was created. Status: " << fast_status << std::endl;
	}

	for (i = 0; i < iCameraCounts; i++) {
		memset(&xi_image[i], 0, sizeof(xi_image[i]));
        	xi_image[i].size = sizeof(XI_IMG);
	}

	int exposure_time = 0;
	std::cout << "Select exposure time: ";
        int scanf_status = scanf("%d", &exposure_time);
	std::cout << "Selected exposure time " << exposure_time << std::endl;

        if (scanf_status == 0 || exposure_time < 100) {
		std::cout << "Invalid exposure time." << std::endl;
                return -1;
        }

        for (i = 0; i < iCameraCounts; i++) {
                xi_status = xiSetParamInt(hCamera[i], XI_PRM_EXPOSURE, exposure_time);
		std::cout << "Setting exposure time. Status: " << xi_status << std::endl;
        }

        int framerate = 0;
	std::cout << "Select framerate: ";
        scanf_status = scanf("%d", &framerate);
	std::cout << "Selected framerate " << framerate << std::endl;

        if (scanf_status == 0 || framerate < 1 || framerate > 200) {
		std::cout << "Invalid framerate." << std::endl;
                return -1;
        }

        for (i = 0; i < iCameraCounts; i++) {
                xi_status = xiSetParamInt(hCamera[i], XI_PRM_ACQ_TIMING_MODE, XI_ACQ_TIMING_MODE_FRAME_RATE_LIMIT);
		std::cout << "Setting timing mode framerate. Status: " << xi_status << std::endl;
                xi_status = xiSetParamInt(hCamera[i], XI_PRM_FRAMERATE, framerate);
		std::cout << "Setting framerate. Status: " << xi_status << std::endl;
        }

	std::cout << "Select number of images: ";
        scanf_status = scanf("%d", &imageCounts);
	std::cout << "Selected number of images " << imageCounts;

        if (scanf_status == 0 || imageCounts < 1 || imageCounts > 999000) {
		std::cout << "Invalid number of images." << std::endl;
                return -1;
        }

        fastJfifInfo_t jfifInfo;
	memset(&jfifInfo, 0, sizeof(jfifInfo));
        jfifInfo.bytestreamSize = channelCount * maxHeight[0] * maxWidth[0];
        jfifInfo.exifSectionsCount = 0;
        jfifInfo.exifSections = NULL;
        fast_status = fastMalloc((void **)&jfifInfo.h_Bytestream, jfifInfo.bytestreamSize);
        std::cout << "fastMalloc on camera_0 was performed. Status: " << fast_status << std::endl;

        xiSetParamInt(hCamera[0], XI_PRM_GPO_SELECTOR, 1);
        xiSetParamInt(hCamera[0], XI_PRM_GPO_MODE,  XI_GPO_EXPOSURE_ACTIVE);

        if (iCameraCounts > 1) {
                for (i = 1; i < iCameraCounts; i++) {
                        xiSetParamInt(hCamera[i], XI_PRM_GPI_SELECTOR, 1);
                        xiSetParamInt(hCamera[i], XI_PRM_GPI_MODE,  XI_GPI_TRIGGER);
                        xiSetParamInt(hCamera[i], XI_PRM_TRG_SOURCE, XI_TRG_EDGE_RISING);
                }
        }

	for (i = 0; i < iCameraCounts; i++) {
		std::cout << "Starting acquisition. Camera " << i <<std::endl;
	        xi_status = xiStartAcquisition(hCamera[i]);
		std::cout << "Starting acquisition. Camera " << i << ". Status " << xi_status << std::endl;
        }

	unsigned quality = 90;

        auto start = std::chrono::system_clock::now();

        for (i = 0; i < imageCounts; i++) {
		for (j = 0; j < iCameraCounts; j++) {
                	std::string num;
                	str << std::setfill('0') <<std::setw(6) << i;
                	str >> num;
                	str.clear();
                	std::string full_name;
                	full_name = full_name + dot + folder + camera + std::to_string(j) + image + num + jpg;
                	char* filename = const_cast<char*>(full_name.c_str());
			std::cout << filename << std::endl;

                	xi_status = xiGetImage(hCamera[j], 1000, &xi_image[j]);
			unsigned iWidth = (unsigned) xi_image[j].width;
			unsigned pitch = iWidth;
			unsigned iHeight = (unsigned) xi_image[j].height;

			std::cout << "Getting image. Status: " << xi_status << std::endl;

			if (xi_status == XI_OK) {
        			fast_status = fastImportFromHostCopy (handle[j], (void *)xi_image[j].bp, iWidth, pitch, iHeight);
				std::cout << "fastImportFromHostCopy was performed. Status: " << fast_status << std::endl;

				fast_status = fastJpegEncode(jpegEncoderHandle[j], quality, &jfifInfo);
				std::cout << "fastJpegEncode was performed. Status: " << fast_status << std::endl;

				fast_status = fastJfifStoreToFile(filename, &jfifInfo);
				std::cout << "Storing to file  was performed. Status: " << fast_status << std::endl;
               		} else {
				std::cout << "Camera status: " << xi_status << std::endl;
                	}
		}
	}

        auto end = std::chrono::system_clock::now();

        printf("Time: %.2lf seconds\n", (double)std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0);

	for (i = 0; i < iCameraCounts; i++) {
        	xiStopAcquisition(hCamera[i]);
        }

	for (i = 0; i < iCameraCounts; i++) {
     		xiCloseDevice(hCamera[i]);
        }

	for (i = 0; i < iCameraCounts; i++) {
		fast_status = fastJpegEncoderDestroy(jpegEncoderHandle[i]);
		std::cout << "Export adapter for jpeg encoder on camera_" << i << " was destroyed. Status: " << fast_status << std::endl;
	}

	for (i = 0; i < iCameraCounts; i++) {
		fast_status = fastImportFromHostDestroy(handle[i]);
		std::cout << "Import adapter from host on camera_" << i << " was destroyed. Status: " << fast_status << std::endl;
	}

	if (jfifInfo.h_Bytestream != NULL) {
		fast_status = fastFree(jfifInfo.h_Bytestream);
		std::cout << "fastFree was performed. Status: " << fast_status << std::endl;
	}

	return 0;
}
