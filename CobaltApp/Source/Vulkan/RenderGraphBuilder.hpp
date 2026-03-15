#pragma once
#include "VulkanUtils.hpp"

#include <vector>
#include <unordered_map>
#include <functional>

namespace Cobalt
{

	enum class RGPassType
	{
		Graphics,
		Compute
	};

	enum class RGAccessType
	{
		ShaderRead,
		ColorAttachmentWrite,
		ReadWrite
	};

	enum class RGResourceType
	{
		ColorAttachment,
		DepthAttachment,
	};

	enum class RGResourceSizeFlags
	{
		Absolute,
		SwapchainRelative
	};

	struct RGResourceInfo
	{
		RGResourceType ResourceType;
		RGResourceSizeFlags ResourceSizeFlags = RGResourceSizeFlags::SwapchainRelative;

		bool Transient = true;
		bool SwapchainTarget = false;

		uint32_t Width = 1;
		uint32_t Height = 1;
	};

	enum
	{
		RGResourceHandle_Invalid = UINT32_MAX,
		RGPassHandle_Invalid = UINT32_MAX
	};

	using RGResourceHandle = uint32_t;
	using RGPassHandle = uint32_t;
	using RGPassExecutionCallback = std::function<void(VkCommandBuffer)>;

	struct RGResourceDependency
	{
		RGResourceHandle ResourceHandle;
		RGAccessType AccessType;
	};

	struct RGPassDependency
	{
		RGPassHandle PassHandle;
		RGAccessType AccessType;
	};

	using RGResourceNameHandleMap = std::unordered_map<std::string, RGResourceHandle>;

	class RenderGraphBuilder
	{
	public:
		RenderGraphBuilder(RGResourceNameHandleMap& resourceNameHandleMap, std::vector<RGResourceInfo>& resources);
		~RenderGraphBuilder();

	public:
		RGResourceHandle DeclareResource(const std::string& resourceName, const RGResourceInfo& resourceInfo);
		RGResourceHandle GetResource(const std::string& resourceName);

		void AddDependency(RGResourceHandle resourceHandle, RGAccessType accessType);

	public:
		const std::vector<RGResourceDependency>& GetResourceDependencies() { return mResourceDependencies; }

	private:
		RGResourceNameHandleMap& mResourceNameHandleMap;
		std::vector<RGResourceInfo>& mResources;
		
		std::vector<RGResourceDependency> mResourceDependencies;
	};

}
