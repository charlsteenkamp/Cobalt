#pragma once
#include "VulkanUtils.hpp"
#include "DescriptorBindings.hpp"
#include "DescriptorBufferManager.hpp"

#include <unordered_map>

namespace Cobalt
{

	class DescriptorCache
	{
	public:
		DescriptorCache(DescriptorBufferManager& descriptorBufferManager);
		~DescriptorCache();

	public:
		void WriteDescriptorBindingsIfNeeded(DescriptorHandle handle, const DescriptorBindings& descriptorBindings);

	private:
		void WriteDescriptorBindings(DescriptorHandle handle, const DescriptorBindings& descriptorBindings);

	private:
		DescriptorBufferManager& mDescriptorBufferManager;
		std::unordered_map<DescriptorHandle, DescriptorSignature> mCachedDescriptorSignatures;
	};

}
