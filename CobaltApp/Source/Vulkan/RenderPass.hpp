#pragma once
#include "RenderGraphBuilder.hpp"
#include "Renderer.hpp"

#include <filesystem>

namespace Cobalt
{

	using RenderPassFlags = uint32_t;

	enum class RenderPassFlagBits : RenderPassFlags
	{
		None = 0,
		SideAffect = 1,
		MeshPass = 2
	};

	class RenderPass
	{
	public:
		RenderPass(const std::string& name, const std::filesystem::path& shaderPath, RenderPassFlags flags)
			: mName(name), mShaderPath(shaderPath), mFlags(flags)
		{
		}

		virtual ~RenderPass() = default;

	public:
		virtual void Setup(RenderGraphBuilder& builder) = 0;
		virtual void Execute(VkCommandBuffer commandBuffer, const RenderContext& renderContext) = 0;

	public:
		const std::string& GetName() const { return mName; }
		const std::filesystem::path& GetShaderPath() const { return mShaderPath; }
		bool HasFlag(RenderPassFlagBits flag) const { return mFlags & (RenderPassFlags)flag; }

	protected:
		std::string mName;
		std::filesystem::path mShaderPath;
		RenderPassFlags mFlags;
	};

}
