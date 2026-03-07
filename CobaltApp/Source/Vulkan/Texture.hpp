#pragma once
#include "VulkanUtils.hpp"
#include <vma/vk_mem_alloc.h>
#include <string>

namespace Cobalt
{

	struct TextureInfo
	{
		TextureInfo() = default;
		TextureInfo(const std::string& filePath)
			: FilePath(filePath)
		{
		}
		TextureInfo(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels = 1)
			: FilePath(""), Width(width), Height(height), Format(format), Usage(usage), MipLevels(mipLevels)
		{
		}

		bool LoadFromFile() const
		{
			return !FilePath.empty();
		}

		std::string FilePath;

		// Fields inferred from FilePath.

		uint32_t Width = 0;
		uint32_t Height = 0;
		VkFormat Format = VK_FORMAT_UNDEFINED;
		VkImageUsageFlags Usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		uint32_t MipLevels = 1;
	};

	struct Image
	{
		VkSampler Sampler;
		VkImageView ImageView;
		VkImageLayout ImageLayout;
	};

	class Texture
	{
	public:
		Texture(const TextureInfo& textureInfo);
		~Texture();

		void CopyData(const void* data);
		void Recreate(uint32_t width, uint32_t height);

	private:
		uint8_t* LoadDataFromFile(const std::string& filePath);
		void Release();

	public:
		VkImage GetImage() const { return mImage; }
		VkImageView GetImageView() const { return mImageView; }
		//VkDeviceMemory GetMemory() const { return mMemory; }
		VkSampler GetSampler() const { return mSampler; }

		VkImageLayout GetImageLayout() const { return mImageLayout; }
		void SetImageLayout(VkImageLayout layout) { mImageLayout = layout; }

		VkImageUsageFlags  GetImageUsageFlags()  const { return mUsage;       }
		VkImageAspectFlags GetImageAspectFlags() const { return mImageAspect; }

		uint32_t GetWidth() const { return mWidth; }
		uint32_t GetHeight() const { return mHeight; }

		VkFormat GetFormat() const { return mFormat; }

		uint32_t GetMipMapLevels() const { return mMipLevels; }

	private:
		VkImage mImage = VK_NULL_HANDLE;
		VkImageView mImageView = VK_NULL_HANDLE;
		VkSampler mSampler = VK_NULL_HANDLE;

		VmaAllocation mAllocation;
		VmaAllocationInfo mAllocationInfo;

		VkImageLayout mImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		uint32_t mWidth, mHeight;
		uint32_t mImageSize;

		VkFormat mFormat;
		VkImageUsageFlags mUsage;
		VkImageAspectFlags mImageAspect;
		uint32_t mMipLevels;
	};

}
