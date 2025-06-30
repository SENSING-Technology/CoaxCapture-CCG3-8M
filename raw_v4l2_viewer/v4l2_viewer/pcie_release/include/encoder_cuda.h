#ifndef __PCIE_CAMERA_H
#define __PCIE_CAMERA_H

#if 0
#include "nvEncodeAPI.h"
#include "NvEncoderCLIOptions.h"
#endif

#include "camera_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0
void ParseCommandLine(int argc, char *argv[], char *szInputFileName, int &nWidth, int &nHeight, 
NV_ENC_BUFFER_FORMAT &eFormat, char *szOutputFileName, NvEncoderInitParam &initParam, int &iGpu, 
bool &bOutputInVidMem, int32_t &cuStreamType);
void EncodeCudaOpInVidMem(int nWidth, int nHeight, NV_ENC_BUFFER_FORMAT eFormat, NvEncoderInitParam encodeCLIOptions, CUcontext cuContext, std::ifstream &fpIn, std::ofstream &fpOut, char *outFilePath, int32_t cuStreamType);
void EncodeCuda(int nWidth, int nHeight, NV_ENC_BUFFER_FORMAT eFormat, NvEncoderInitParam encodeCLIOptions, CUcontext cuContext, std::ifstream &fpIn, std::ofstream &fpOut);
void ParseCommandLine(int argc, char *argv[], char *szInputFileName, int &nWidth, int &nHeight, 
NV_ENC_BUFFER_FORMAT &eFormat, char *szOutputFileName, NvEncoderInitParam &initParam, int &iGpu, 
bool &bOutputInVidMem, int32_t &cuStreamType);
#endif

struct videoFrame{
    unsigned char *data;
    size_t length;
};


int encoder_cuda_init(int board, int channel, frameInfo *frame, char *filename);
int encoder_cuda_encode(int board, int channel, frameInfo *frame);
void encoder_cuda_destroy(int board,int channel);
void cuda_init();
#ifdef __cplusplus
}
#endif

#endif