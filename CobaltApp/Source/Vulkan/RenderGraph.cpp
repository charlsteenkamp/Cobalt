#include "copch.hpp"
#include "RenderGraph.hpp"

namespace Cobalt
{

	RenderGraphBuilder::RenderGraphBuilder(RGResourceNameHandleMap& resourceNameHandleMap, std::vector<RGResourceInfo>& resources)
		: mResourceNameHandleMap(resourceNameHandleMap), mResources(resources)
	{
		CO_PROFILE_FN();
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

	RenderGraph::RenderGraph()
	{

	}

	RenderGraph::~RenderGraph()
	{

	}

	Texture& RenderGraph::GetResource(RGResourceHandle handle)
	{
		Texture texture(TextureInfo(""));
		return texture;
	}

	void RenderGraph::Execute()
	{

	}

}