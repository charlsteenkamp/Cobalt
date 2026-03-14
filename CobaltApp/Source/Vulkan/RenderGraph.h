#pragma once
#include "VulkanUtils.hpp"
#include "Texture.hpp"

#include <vector>
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

	using RGResourceHandle = uint32_t;
	using RGPassHandle = uint32_t;
	using RGPassExecutionCallback = std::function<void(VkCommandBuffer)>;

	struct RGPassDependency
	{
		RGPassHandle PassHandle;
		RGAccessType AccessType;
	};

	class RenderGraphBuilder
	{
	public:
		RenderGraphBuilder();
		~RenderGraphBuilder();

	public:
		RGResourceHandle AddResource(const TextureInfo& textureInfo);

		RGPassHandle AddPass(const std::string& name, RGPassType passType);
		void AddDependency(RGPassHandle passHandle, RGResourceHandle resourceHandle, RGAccessType accessType);
		void SetExecutionCallback(RGPassHandle passHandle, RGPassExecutionCallback callback);

		void Compile();

	private:
		std::vector<RGPassDependency> mDependencies;
	};

	class RenderGraph
	{
	public:
		RenderGraph();
		~RenderGraph();

	public:
		Texture& GetResource(RGResourceHandle handle);

	public:
		void Execute();

	};

}
