#include "copch.hpp"
#include "Texture.hpp"
#include "GraphicsContext.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanCommands.hpp"

#include <stb_image.h>

namespace Cobalt
{

	static void UnpackRGBToRGBA(uint8_t* rgba, const uint8_t* rgb, const int32_t count)
	{
		for (int32_t i = count; i--; rgba += 4, rgb += 3)
		{
			*(uint32_t*)(void*)rgba = *(const uint32_t*)(const void*)rgb;
		}

		for (int32_t i = 0; i < 3; i++)
		{
			rgba[i] = rgb[i];
		}
	}

	Texture::Texture(const TextureInfo& textureInfo)
		: mWidth(textureInfo.Width), mHeight(textureInfo.Height), mFormat(textureInfo.Format), mUsage(textureInfo.Usage), mMipLevels(textureInfo.MipLevels)
	{
		CO_PROFILE_FN();

		if (textureInfo.LoadFromFile())
		{
			uint8_t* data = LoadDataFromFile(textureInfo.FilePath);
			Recreate(mWidth, mHeight);
			CopyData(data);
		}
		else
		{
			Recreate(mWidth, mHeight);
		}
	}

	Texture::~Texture()
	{
		CO_PROFILE_FN();

		Release();
	}

	void Texture::CopyData(const void* data)
	{
		CO_PROFILE_FN();

		std::unique_ptr<VulkanBuffer> stagingBuffer = VulkanBuffer::CreateMappedBuffer(mAllocationInfo.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		stagingBuffer->CopyData(data);

		GraphicsContext::Get().SubmitSingleTimeCommands(GraphicsContext::Get().GetQueue(), [&](VkCommandBuffer commandBuffer)
		{
			VulkanCommands::TransitionImageLayout(commandBuffer, *this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			VulkanCommands::CopyBufferToImage(commandBuffer, *stagingBuffer, *this);
			VulkanCommands::TransitionImageLayout(commandBuffer, *this, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		});
	}

	void Texture::Recreate(uint32_t width, uint32_t height)
	{
		CO_PROFILE_FN();

		mWidth = width;
		mHeight = height;

		Release();

		// Create image

		VkImageCreateInfo imageCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.flags = 0,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = mFormat,
			.extent = { mWidth, mHeight, 1 },
			.mipLevels = mMipLevels,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = mUsage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};

		if (mUsage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			//imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			//mImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		VmaAllocationCreateInfo allocCreateInfo = {
			.usage = VMA_MEMORY_USAGE_AUTO,
		};

		//VK_CALL(vkCreateImage(GraphicsContext::Get().GetDevice(), &imageCreateInfo, nullptr, &mImage));
		VK_CALL(vmaCreateImage(GraphicsContext::Get().GetAllocator(), &imageCreateInfo, &allocCreateInfo, &mImage, &mAllocation, &mAllocationInfo));

		// Create image view

		if (mFormat == VK_FORMAT_D16_UNORM_S8_UINT || mFormat == VK_FORMAT_D16_UNORM || mFormat == VK_FORMAT_D24_UNORM_S8_UINT || mFormat == VK_FORMAT_D32_SFLOAT)
			mImageAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
		else
			mImageAspect = VK_IMAGE_ASPECT_COLOR_BIT;

		VkImageViewCreateInfo imageViewCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.flags = 0,
			.image = mImage,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = mFormat,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange = {
				.aspectMask = mImageAspect,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		VK_CALL(vkCreateImageView(GraphicsContext::Get().GetDevice(), &imageViewCreateInfo, nullptr, &mImageView));

		// Create sampler if needed

		if (mUsage & VK_IMAGE_USAGE_SAMPLED_BIT)
		{
			VkPhysicalDeviceProperties physicalDeviceProperties;
			vkGetPhysicalDeviceProperties(GraphicsContext::Get().GetPhysicalDevice(), &physicalDeviceProperties);

			VkSamplerCreateInfo samplerCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				.flags = 0,
				.magFilter = VK_FILTER_NEAREST,
				.minFilter = VK_FILTER_NEAREST,
				.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
				.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				.mipLodBias = 0.0f,
				.anisotropyEnable = VK_TRUE,
				.maxAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy,
				.compareEnable = VK_FALSE,
				.compareOp = {},
				.minLod = 0.0f,
				.maxLod = VK_LOD_CLAMP_NONE,
				.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
				.unnormalizedCoordinates = VK_FALSE,
			};

			VK_CALL(vkCreateSampler(GraphicsContext::Get().GetDevice(), &samplerCreateInfo, nullptr, &mSampler));
		}
	}

	uint8_t* Texture::LoadDataFromFile(const std::string& filePath)
	{
		CO_PROFILE_FN();

		int32_t width, height, channels;
		stbi_uc* data = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

		if (!data || (channels != 4 && channels != 3))
		{
			std::cerr << "Failed to load texture" << filePath << std::endl;
			return nullptr;
		}

		mWidth = width;
		mHeight = height;
		mImageSize = mWidth * mHeight * channels;
		mFormat = VK_FORMAT_R8G8B8A8_SRGB;
		mUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		// TODO: mipmaps

		//mMipLevels = (uint32_t)std::floor(std::log2(std::max(mWidth, mHeight))) + 1;
		mMipLevels = 1;

		//if (channels == 3)
		if (0)
		{
			uint8_t* data_rgba = (uint8_t*)malloc(mImageSize);
			memset(data_rgba, 0, mImageSize);

			UnpackRGBToRGBA(data_rgba, data, mWidth * mHeight);

			free(data);

			return data_rgba;
		}

		return data;
	}

	void Texture::Release()
	{
		CO_PROFILE_FN();

		if (mImage)
			//vkDestroyImage(GraphicsContext::Get().GetDevice(), mImage, nullptr);
			vmaDestroyImage(GraphicsContext::Get().GetAllocator(), mImage, mAllocation);

		if (mImageView)
			vkDestroyImageView(GraphicsContext::Get().GetDevice(), mImageView, nullptr);

		if (mSampler)
			vkDestroySampler(GraphicsContext::Get().GetDevice(), mSampler, nullptr);

		//if (mMemory)
			//vkFreeMemory(GraphicsContext::Get().GetDevice(), mMemory, nullptr);
	}

	Cubemap::Cubemap(const CubemapInfo& cubemapInfo)
	{
		CO_PROFILE_FN();

		auto paths = cubemapInfo.FacePaths.GetPaths();

		std::vector<uint8_t*> facesData(6);

		for (uint32_t i = 0; i < facesData.size(); i++)
			facesData[i] = LoadDataFromFile(paths[i]);

		Create();
		CopyData(facesData);

		for (uint8_t* face : facesData)
			free(face);
	}

	Cubemap::~Cubemap()
	{
		CO_PROFILE_FN();
	}

	uint8_t* Cubemap::LoadDataFromFile(const std::filesystem::path& filePath)
	{
		CO_PROFILE_FN();

		int32_t channelCount = 0;

		uint8_t* data = stbi_load(filePath.string().c_str(), (int32_t*)&mWidth, (int32_t*)&mHeight, &channelCount, STBI_rgb_alpha);

		mFormat = VK_FORMAT_R8G8B8A8_SRGB;
		return data;
	}

	void Cubemap::Create()
	{
		// Create image

		VkImageCreateInfo imageCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = mFormat,
			.extent = { mWidth, mHeight, 1 },
			.mipLevels = mMipLevels,
			.arrayLayers = 6,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};

		VmaAllocationCreateInfo allocCreateInfo = {
			.usage = VMA_MEMORY_USAGE_AUTO,
		};

		VK_CALL(vmaCreateImage(GraphicsContext::Get().GetAllocator(), &imageCreateInfo, &allocCreateInfo, &mImage, &mAllocation, &mAllocationInfo));

		// Create image view

		VkImageViewCreateInfo imageViewCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.flags = 0,
			.image = mImage,
			.viewType = VK_IMAGE_VIEW_TYPE_CUBE,
			.format = mFormat,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = mMipLevels,
				.baseArrayLayer = 0,
				.layerCount = 6
			}
		};

		VK_CALL(vkCreateImageView(GraphicsContext::Get().GetDevice(), &imageViewCreateInfo, nullptr, &mImageView));

		// Create sampler if needed

		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(GraphicsContext::Get().GetPhysicalDevice(), &physicalDeviceProperties);

		VkSamplerCreateInfo samplerCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.flags = 0,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.mipLodBias = 0.0f,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy,
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_NEVER,
			.minLod = 0.0f,
			.maxLod = (float)mMipLevels,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE,
			.unnormalizedCoordinates = VK_FALSE,
		};

		VK_CALL(vkCreateSampler(GraphicsContext::Get().GetDevice(), &samplerCreateInfo, nullptr, &mSampler));
	}

	void Cubemap::CopyData(const std::vector<uint8_t*>& facesData)
	{
		CO_PROFILE_FN();

		VkDeviceSize imageSize = mWidth * mHeight * 4 * 6;
		VkDeviceSize layerSize = mWidth * mHeight * 4;

		std::unique_ptr<VulkanBuffer> stagingBuffer = VulkanBuffer::CreateMappedBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

		for (uint32_t i = 0; i < 6; i++)
			stagingBuffer->CopyData(facesData[i], layerSize, i * layerSize);

		std::vector<VkBufferImageCopy> bufferImageCopies;

		for (uint32_t i = 0; i < facesData.size(); i++)
		{
			bufferImageCopies.push_back(VkBufferImageCopy {
				.bufferOffset = i * layerSize,
				.imageSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = 0,
					.baseArrayLayer = i,
					.layerCount = 1,
				},
				.imageExtent = { mWidth, mHeight, 1 },
			});
		}

		GraphicsContext::Get().SubmitSingleTimeCommands(GraphicsContext::Get().GetQueue(), [&](VkCommandBuffer commandBuffer)
		{
			VulkanCommands::TransitionImageLayout(commandBuffer, mImage, VK_IMAGE_ASPECT_COLOR_BIT, mMipLevels, 6, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			vkCmdCopyBufferToImage(commandBuffer, stagingBuffer->GetBuffer(), mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)bufferImageCopies.size(), bufferImageCopies.data());
			VulkanCommands::TransitionImageLayout(commandBuffer, mImage, VK_IMAGE_ASPECT_COLOR_BIT, mMipLevels, 6, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		});

		mImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

}