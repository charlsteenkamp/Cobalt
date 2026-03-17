#pragma once
#include "VulkanUtils.hpp"
#include "HashUtils.hpp"

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
		DepthAttachment,
		Present
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
		RGResourceHandle_BackBufferAttachment = 0,
		RGResourceHandle_Invalid = UINT32_MAX,
		RGPassHandle_Invalid = UINT32_MAX
	};

	using RGResourceHandle = uint32_t;
	using RGPassHandle = uint32_t;
	using RGPassExecutionCallback = std::function<void(VkCommandBuffer)>;

	struct RGResourceDependency
	{
		RGResourceDependency() = default;
		RGResourceDependency(RGResourceHandle resourceHandle, RGAccessType accessType)
			: ResourceHandle(resourceHandle), AccessType(accessType)
		{
		}
		RGResourceDependency(const RGResourceDependency& other)
			: ResourceHandle(other.ResourceHandle), AccessType(other.AccessType)
		{
		}

		bool operator=(const RGResourceDependency& other)
		{
			return other.ResourceHandle == ResourceHandle && other.AccessType == AccessType;
		}

		RGResourceHandle ResourceHandle;
		RGAccessType AccessType;
	};

	struct RGPassDependency
	{
		RGPassDependency() = default;
		RGPassDependency(RGPassHandle passHandle, RGAccessType accessType)
			: PassHandle(passHandle), AccessType(accessType)
		{
		}
		RGPassDependency(const RGPassDependency& other)
			: PassHandle(other.PassHandle), AccessType(other.AccessType)
		{
		}

		bool operator=(const RGPassDependency& other)
		{
			return other.PassHandle == PassHandle && other.AccessType == AccessType;
		}

		RGPassHandle PassHandle;
		RGAccessType AccessType;
	};

	using RGResourceNameHandleMap = std::unordered_map<std::string, RGResourceHandle>;

	struct RGClearColorMapHash
	{
		size_t operator()(const std::pair<Cobalt::RGPassHandle, Cobalt::RGResourceHandle>& p) const
		{
			size_t h = 0;
			Cobalt::HashCombine(h, std::hash<uint32_t>{}(p.first));
			Cobalt::HashCombine(h, std::hash<uint32_t>{}(p.second));

			return h;
		}
	};

	using RGClearColorMap = std::unordered_map<std::pair<RGPassHandle, RGResourceHandle>, VkClearColorValue, RGClearColorMapHash>;

	class RenderGraphBuilder
	{
	public:
		RenderGraphBuilder(RGPassHandle passHandle, RGResourceNameHandleMap& resourceNameHandleMap, std::vector<RGResourceInfo>& resources, RGClearColorMap& clearColorMap);
		~RenderGraphBuilder();

	public:
		RGResourceHandle DeclareResource(const std::string& resourceName, const RGResourceInfo& resourceInfo);
		RGResourceHandle GetResource(const std::string& resourceName);

		void AddDependency(RGResourceHandle resourceHandle, RGAccessType accessType);
		void SetClearColor(RGResourceHandle resourceHandle, VkClearColorValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f });

	public:
		const std::vector<RGResourceDependency>& GetResourceDependencies() { return mResourceDependencies; }

	private:
		RGPassHandle mPassHandle;
		RGResourceNameHandleMap& mResourceNameHandleMap;
		std::vector<RGResourceInfo>& mResources;
		RGClearColorMap& mClearColorMap;
		
		std::vector<RGResourceDependency> mResourceDependencies;
	};

}
