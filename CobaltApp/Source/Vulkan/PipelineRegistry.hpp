#pragma once
#include "VulkanUtils.hpp"
#include "Pipeline.hpp"

namespace Cobalt
{

	class PipelineRegistry
	{
	public:
		PipelineRegistry();
		~PipelineRegistry();

	public:
		Pipeline* BuildPipeline(const std::string& passName, const PipelineInfo& pipelineInfo);
		Pipeline* GetPipeline(const std::string& passName) const;

	private:
		std::vector<std::unique_ptr<Pipeline>> mPipelines;
		std::unordered_map<std::string, Pipeline*> mPassNamePipelineMap;
	};

}
