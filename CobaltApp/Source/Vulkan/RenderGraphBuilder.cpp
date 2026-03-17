#include "copch.hpp"
#include "RenderGraphBuilder.hpp"
#include "HashUtils.hpp"

namespace Cobalt
{

	RenderGraphBuilder::RenderGraphBuilder(RGPassHandle passHandle, RGResourceNameHandleMap& resourceNameHandleMap, std::vector<RGResourceInfo>& resources, RGClearColorMap& clearColorMap)
		: mPassHandle(passHandle), mResourceNameHandleMap(resourceNameHandleMap), mResources(resources), mClearColorMap(clearColorMap)
	{
		CO_PROFILE_FN();

		mResourceNameHandleMap["BackBuffer Attachment"] = 0;
		mResources.emplace_back(RGResourceInfo {
			.ResourceType = RGResourceType::ColorAttachment,
			.Transient = false,
			.SwapchainTarget = true
		});
	}

	RenderGraphBuilder::~RenderGraphBuilder()
	{
		CO_PROFILE_FN();
	}

	RGResourceHandle RenderGraphBuilder::DeclareResource(const std::string& resourceName, const RGResourceInfo& resourceInfo)
	{
		CO_PROFILE_FN();

		if (!mResourceNameHandleMap.contains(resourceName))
		{
			mResources.push_back(resourceInfo);
			mResourceNameHandleMap[resourceName] = mResources.size() - 1;
		}

		return mResourceNameHandleMap.at(resourceName);
	}

	RGResourceHandle RenderGraphBuilder::GetResource(const std::string& resourceName)
	{
		CO_PROFILE_FN();

		if (mResourceNameHandleMap.contains(resourceName))
			return mResourceNameHandleMap.at(resourceName);

		return RGResourceHandle_Invalid;
	}

	void RenderGraphBuilder::AddDependency(RGResourceHandle resourceHandle, RGAccessType accessType)
	{
		CO_PROFILE_FN();

		mResourceDependencies.emplace_back(resourceHandle, accessType);
	}

	void RenderGraphBuilder::SetClearColor(RGResourceHandle resourceHandle, VkClearColorValue clearColor)
	{
		CO_PROFILE_FN();

		mClearColorMap[{ mPassHandle, resourceHandle }] = clearColor;
	}

}