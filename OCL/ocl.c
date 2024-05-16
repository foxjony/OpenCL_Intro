// OpenCL - First GPU Test Program 
// https://www.youtube.com/watch?v=rpMNTTlTUok

#include <CL/cl.h>	// For APPLE <OpenCL/cl.h>
#include <stdio.h>
#include <stdlib.h>

#define VECTOR_SIZE 1024

// Программа выполняемая многопоточно
const char *kernelSquare =
	"__kernel void kernel_square(__global float *A, __global float *B) { \n"
	"	int index = get_global_id(0);	// Получение номера потока \n"
	"	B[index] = A[index] * A[index]; \n"
	"} \n";

int main(void) {
	int i;

	// Инициализация векторов А и В
	float *A = (float *)malloc(sizeof(float) * VECTOR_SIZE);
	float *B = (float *)malloc(sizeof(float) * VECTOR_SIZE);
	for (i = 0; i < VECTOR_SIZE; i++) {A[i] = i; B[i] = 0;}

	// Получение информации о доступных платформах
	cl_platform_id *platforms = NULL;
	cl_uint num_platforms;
	cl_int clStatus = clGetPlatformIDs(0, NULL, &num_platforms);
	platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * num_platforms);
	clStatus = clGetPlatformIDs(num_platforms, platforms, NULL);

	// Получение списка устройств и выбор устройства для исполнения кода
	cl_device_id *device_list = NULL;
	cl_uint num_devices;
	clStatus = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);
	device_list = (cl_device_id *)malloc(sizeof(cl_device_id) * num_devices);
	clStatus = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, num_devices, device_list, NULL);

	// Создание контекста для каждого устройства
	cl_context context;
	context = clCreateContext(NULL, num_devices, device_list, NULL, NULL, &clStatus);

	// Создание очереди команд (OpenCL < 2.0)
	//cl_command_queue command_queue = clCreateCommandQueue(context, device_list[0], 0, &clStatus);

	// Создание очереди команд (OpenCL >= 2.0)
	cl_command_queue command_queue = clCreateCommandQueueWithProperties(context, device_list[0], 0, &clStatus);

	// Создание буфера памяти для каждого вектора
	cl_mem A_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, VECTOR_SIZE * sizeof(float), NULL, &clStatus);
	cl_mem B_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, VECTOR_SIZE * sizeof(float), NULL, &clStatus);

	// Скопировать буфер A  на устройство
	clStatus = clEnqueueWriteBuffer(command_queue, A_clmem, CL_TRUE, 0, VECTOR_SIZE * sizeof(float), A, 0, NULL, NULL);

	// Создать программу kernelSource
	cl_program program = clCreateProgramWithSource(context, 1, (const char **)&kernelSquare, NULL, &clStatus);

	// Собрать программу
	clStatus = clBuildProgram(program, 1, device_list, NULL, NULL, NULL);

	// Создать процесс на устройстве
	cl_kernel kernel = clCreateKernel(program, "kernel_square", &clStatus);

	// Передать аргументы в программу
	clStatus = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&A_clmem);
	clStatus = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&B_clmem);

	// Выполнить программу
	size_t global_size = VECTOR_SIZE;
	size_t local_size = 64;
	clStatus = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);

	// Скопировать буфер B на хостовое устройство 
	clStatus = clEnqueueReadBuffer(command_queue, B_clmem, CL_TRUE, 0, VECTOR_SIZE * sizeof(float), B, 0 , NULL, NULL);

	// Ожидание завершения всех команд
	clStatus = clFlush(command_queue);
	clStatus = clFinish(command_queue);

	// Вивод результатов
	for (i = 0; i < VECTOR_SIZE; i++) {printf("%f * %f = %f\n", A[i], A[i], B[i]);}

	// Освобождение памяти
	clStatus = clReleaseKernel(kernel);
	clStatus = clReleaseProgram(program);
	clStatus = clReleaseMemObject(A_clmem);
	clStatus = clReleaseMemObject(B_clmem);
	clStatus = clReleaseCommandQueue(command_queue);
	clStatus = clReleaseContext(context);

	free(A);
	free(B);
	free(platforms);
	free(device_list);
	return 0;
}
