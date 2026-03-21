#include "copch.hpp"
#include "PipelineRegistry.hpp"

namespace Cobalt
{

	PipelineRegistry::PipelineRegistry()
	{
		CO_PROFILE_FN();
	}

	PipelineRegistry::~PipelineRegistry()
	{
		CO_PROFILE_FN();
	}

	Pipeline* PipelineRegistry::BuildPipeline(const std::string& passName, const PipelineInfo& pipelineInfo)
	{
		CO_PROFILE_FN();

		if (Pipeline* pipeline = GetPipeline(passName); pipeline)
			return pipeline;

		mPipelines.push_back(std::make_unique<Pipeline>(pipelineInfo));
		mPassNamePipelineMap[passName] = mPipelines.back().get();

		return mPipelines.back().get();
	}

	Pipeline* PipelineRegistry::GetPipeline(const std::string& passName) const
	{
		CO_PROFILE_FN();

		if (!mPassNamePipelineMap.contains(passName))
			return nullptr;

		return mPassNamePipelineMap.at(passName);
	}

}