#include "copch.hpp"
#include "DescriptorCache.hpp"
#include "GraphicsContext.hpp"

namespace Cobalt
{

	DescriptorCache::DescriptorCache(DescriptorBufferManager& descriptorBufferManager)
		: mDescriptorBufferManager(descriptorBufferManager)
	{
		CO_PROFILE_FN();
	}

	DescriptorCache::~DescriptorCache()
	{
		CO_PROFILE_FN();
	}

	void DescriptorCache::WriteDescriptorBindingsIfNeeded(DescriptorHandle handle, const DescriptorBindings& descriptorBindings)
	{
		CO_PROFILE_FN();

		auto signature = descriptorBindings.Hash();

		if (!mCachedDescriptorSignatures.contains(handle))
		{
			mCachedDescriptorSignatures[handle] = signature;
			WriteDescriptorBindings(handle, descriptorBindings);
		}

		if (signature != mCachedDescriptorSignatures.at(handle))
		{
			mCachedDescriptorSignatures[handle] = signature;
			WriteDescriptorBindings(handle, descriptorBindings);
		}
	}

	void DescriptorCache::WriteDescriptorBindings(DescriptorHandle handle, const DescriptorBindings& descriptorBindings)
	{
		for (const auto& binding : descriptorBindings.Bindings)
		{
			mDescriptorBufferManager.WriteDescriptor(binding, handle);
		}
	}

}