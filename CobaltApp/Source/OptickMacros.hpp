#pragma once
#include <optick.h>

#define CO_ENABLE_PROFILING

#ifdef CO_ENABLE_PROFILING
	#define CO_PROFILE_FRAME(name) OPTICK_FRAME(name)
	#define CO_PROFILE_FN(...) OPTICK_EVENT(__VA_ARGS__)
	#define CO_PROFILE_SCOPE(name) OPTICK_EVENT(name)
	#define CO_PROFILE_CATEGORY(name, category) OPTICK_CATEGORY(name, category)
	#define CO_PROFILE_COMMAND_BUFFER(commandBuffer) OPTICK_GPU_CONTEXT(commandBuffer)
	#define CO_PROFILE_GPU_EVENT(name) OPTICK_GPU_EVENT(name)
	#define CO_PROFILE_SET_SWAPCHAIN(swapchain) OPTICK_GPU_FLIP(swapchain)
	#define CO_PROFILE_START_CAPTURE(...) OPTICK_START_CAPTURE(__VA_ARGS__)
	#define CO_PROFILE_STOP_CAPTURE(...) OPTICK_STOP_CAPTURE(__VA_ARGS__)
	#define CO_PROFILE_SAVE_CAPTURE(...) OPTICK_SAVE_CAPTURE(__VA_ARGS__)
#else
	#define CO_PROFILE_FRAME(name)
	#define CO_PROFILE_FN(...)
	#define CO_PROFILE_SCOPE(name)
	#define CO_PROFILE_CATEGORY(name, category)
	#define CO_PROFILE_COMMAND_BUFFER(commandBuffer)
	#define CO_PROFILE_GPU_EVENT(name)
	#define CO_PROFILE_SET_SWAPCHAIN(swapchain)
	#define CO_PROFILE_START_CAPTURE(...)
	#define CO_PROFILE_STOP_CAPTURE(...) 
	#define CO_PROFILE_SAVE_CAPTURE(...) 
#endif
