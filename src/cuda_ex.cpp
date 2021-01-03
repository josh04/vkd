

//#include <cuda_runtime.h>
//#include <cuda_gl_interop.h>
//#include "helper_cuda.h"

// CUDA state
int cudaDevId = 0;
//struct cudaGraphicsResource* cuda_vbo_resource = nullptr;

//struct cudaGraphicsResource* cuda_image = nullptr;
//struct cudaGraphicsResource* cuda_image_buffer = nullptr;

/*
bool initCUDA() {
	const char* fake = "";
	cudaDevId = findCudaDevice(0, &fake);

	cudaError_t status = cudaMallocHost((void**)&host_pinned, mem_dim_);

	auto vbo_flags = cudaGraphicsMapFlagsWriteDiscard;

	checkCudaErrors(cudaGraphicsGLRegisterBuffer(&cuda_vbo_resource, gVBO, vbo_flags));
	checkCudaErrors(cudaGraphicsGLRegisterImage(&cuda_image, gTex, GL_TEXTURE_2D, vbo_flags));
	checkCudaErrors(cudaGraphicsGLRegisterBuffer(&cuda_image_buffer, imageVBO, cudaGraphicsMapFlagsNone));


	return true;
}
*/
/*
// inc from simple_kernel.cu
void run_simple_vbo_kernel(dim3 block, dim3 grid, float4* pos, unsigned int width, unsigned int height, float time);
void run_simple_image_kernel(dim3 block, dim3 grid, cudaArray_t colour, unsigned int width, unsigned int height, float time);
void run_buffer_to_image_kernel(dim3 block, dim3 grid, float4* col, cudaArray_t colour, unsigned int width, unsigned int height, float time);
*/
/*
void render_cuda(float anim_time) {

	std::vector<cudaGraphicsResource *> resources = { cuda_image, cuda_image_buffer };

	checkCudaErrors(cudaGraphicsMapResources(resources.size(), resources.data(), 0));

	float4 * float_image_ptr = nullptr;
	size_t num_bytes;
	checkCudaErrors(cudaGraphicsResourceGetMappedPointer((void**)&float_image_ptr, &num_bytes, cuda_image_buffer));

	cudaMemcpy(float_image_ptr, host_pinned, mem_dim_, cudaMemcpyHostToDevice);

	cudaArray_t cuarr;
	checkCudaErrors(cudaGraphicsSubResourceGetMappedArray(&cuarr, cuda_image, 0, 0));
	//printf("CUDA mapped VBO: May access %ld bytes\n", num_bytes);

	const unsigned int mesh_width = width_;
	const unsigned int mesh_height = height_;

	// execute the kernel
	dim3 block(8, 8, 1);
	dim3 grid(mesh_width / block.x, mesh_height / block.y, 1);

	run_simple_vbo_kernel(block, grid, float_image_ptr, mesh_width, mesh_height, anim_time);
	//run_simple_image_kernel(block, grid, cuarr, mesh_width, mesh_height, g_fAnim);
	run_buffer_to_image_kernel(block, grid, float_image_ptr, cuarr, mesh_width, mesh_height, anim_time);

	// unmap buffer object
	checkCudaErrors(cudaGraphicsUnmapResources(resources.size(), resources.data(), 0));

}
*/