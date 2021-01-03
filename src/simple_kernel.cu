///////////////////////////////////////////////////////////////////////////////
//! Simple kernel to modify vertex positions in sine wave pattern
//! @param data  data in global memory
///////////////////////////////////////////////////////////////////////////////
__global__ void simple_vbo_kernel(float4* pos, unsigned int width, unsigned int height, float time)
{
    unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;

    // calculate uv coordinates
    float u = x / (float)width;
    float v = y / (float)height;
    u = u * 2.0f - 1.0f;
    v = v * 2.0f - 1.0f;

    // calculate simple sine wave pattern
    float freq = 4.0f;
    float w = sinf(u * freq + time) * cosf(v * freq + time) * 0.5f;

    // write output vertex
    if (y < height / 2) {
        pos[y * width + x] = make_float4(u, w, v, 1.0f);
    }
}

void run_simple_vbo_kernel(dim3 block, dim3 grid, float4* pos, unsigned int width, unsigned int height, float time) {
    simple_vbo_kernel<<<grid, block>>>(pos, width, height, time);
}

__global__ void simple_image_kernel(cudaTextureObject_t colour, unsigned int width, unsigned int height, float time)
{
    unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;

    // calculate uv coordinates
    float u = x / (float)width;
    float v = y / (float)height;
    u = u * 2.0f - 1.0f;
    v = v * 2.0f - 1.0f;

    // calculate simple sine wave pattern
    float freq = 4.0f;
    float w = sinf(u * freq + time) * cosf(v * freq + time) * 0.5f;

    // write output vertex
    float4 c = make_float4(u, w, v, 1.0f);
    surf2Dwrite(c, colour, x * sizeof(float4), y);
}

#include "GL/gl3w.h"
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include "helper_cuda.h"

cudaTextureObject_t inTexObject;

void run_simple_image_kernel(dim3 block, dim3 grid, cudaArray_t colour, unsigned int width, unsigned int height, float time) {

    struct cudaChannelFormatDesc desc;
    checkCudaErrors(cudaGetChannelDesc(&desc, colour));

    cudaResourceDesc            texRes;
    memset(&texRes, 0, sizeof(cudaResourceDesc));

    texRes.resType = cudaResourceTypeArray;
    texRes.res.array.array = colour;

    cudaTextureDesc             texDescr;
    memset(&texDescr, 0, sizeof(cudaTextureDesc));

    texDescr.normalizedCoords = false;
    texDescr.filterMode = cudaFilterModePoint;
    texDescr.addressMode[0] = cudaAddressModeWrap;
    texDescr.readMode = cudaReadModeElementType;

    checkCudaErrors(cudaCreateTextureObject(&inTexObject, &texRes, &texDescr, NULL));

    simple_image_kernel<<<grid, block>>>(inTexObject, width, height, time);
}

__global__ void buffer_to_image_kernel(float4* col, cudaTextureObject_t colour, unsigned int width, unsigned int height, float time)
{
    unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
    /*
    // calculate uv coordinates
    float u = x / (float)width;
    float v = y / (float)height;
    u = u * 2.0f - 1.0f;
    v = v * 2.0f - 1.0f;

    // calculate simple sine wave pattern
    float freq = 4.0f;
    float w = sinf(u * freq + time) * cosf(v * freq + time) * 0.5f;

    // write output vertex
    float4 c = make_float4(u, w, v, 1.0f);
*/

    surf2Dwrite(col[y * width + x], colour, x * sizeof(float4), y);
}
void run_buffer_to_image_kernel(dim3 block, dim3 grid, float4* col, cudaArray_t colour, unsigned int width, unsigned int height, float time) {

    struct cudaChannelFormatDesc desc;
    checkCudaErrors(cudaGetChannelDesc(&desc, colour));

    cudaResourceDesc            texRes;
    memset(&texRes, 0, sizeof(cudaResourceDesc));

    texRes.resType = cudaResourceTypeArray;
    texRes.res.array.array = colour;

    cudaTextureDesc             texDescr;
    memset(&texDescr, 0, sizeof(cudaTextureDesc));

    texDescr.normalizedCoords = false;
    texDescr.filterMode = cudaFilterModePoint;
    texDescr.addressMode[0] = cudaAddressModeWrap;
    texDescr.readMode = cudaReadModeElementType;

    checkCudaErrors(cudaCreateTextureObject(&inTexObject, &texRes, &texDescr, NULL));

    buffer_to_image_kernel<<<grid, block>>>(col, inTexObject, width, height, time);
}