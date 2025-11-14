#include "copch.hpp"
#include "Model.hpp"
#include "Vulkan/Renderer.hpp"
#include "AssetManager.hpp"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

namespace Cobalt
{

	static glm::vec4 FastGLTFVec4ToGLMVec4(const fastgltf::math::nvec4& vec4)
	{
		const float* data = vec4.data();
		return { data[0], data[1], data[2], data[3] };
	}

	static glm::vec3 FastGLTFVec3ToGLMVec3(const fastgltf::math::nvec3& vec3)
	{
		const float* data = vec3.data();
		return { data[0], data[1], data[2] };
	}

	static glm::vec2 FastGLTFVec2ToGLMVec2(const fastgltf::math::nvec2& vec2)
	{
		const float* data = vec2.data();
		return { data[0], data[1] };
	}


	Model::Model(const std::filesystem::path& filePath)
		: mFilePath(filePath)
	{
		CO_PROFILE_FN();

		Load();
	}

	Model::~Model()
	{
		CO_PROFILE_FN();
	}         

	void Model::Load()
	{
		fastgltf::Expected<fastgltf::GltfDataBuffer> gltfData = fastgltf::GltfDataBuffer::FromPath(mFilePath);

		if (gltfData.error() != fastgltf::Error::None)
		{
			return;
		}

		fastgltf::Parser parser;
		fastgltf::Options loadOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

		fastgltf::Expected<fastgltf::Asset> asset = parser.loadGltf(gltfData.get(), mFilePath.parent_path(), loadOptions);

		if (asset.error() != fastgltf::Error::None)
		{
			return;
		}
		
		const fastgltf::Asset& gltfAsset = asset.get();

		LoadTextures(gltfAsset);
		LoadMaterials(gltfAsset);
		LoadMeshes(gltfAsset);
	}

	void Model::LoadTextures(const fastgltf::Asset& gltf)
	{
		CO_PROFILE_FN();

		mTextureAssetHandles.resize(gltf.images.size());

		for (uint32_t i = 0; i < gltf.images.size(); i++)
		{
			const fastgltf::Image& image = gltf.images[i];
			// TODO

			TextureInfo textureInfo;

			AssetHandle textureAssetHandle = AssetManager::RegisterTexture(textureInfo);
			mTextureAssetHandles[i] = textureAssetHandle;
		}
	}

	void Model::LoadMaterials(const fastgltf::Asset& gltf)
	{
		CO_PROFILE_FN();

		mMaterialAssetHandles.resize(gltf.materials.size());

		for (uint32_t i = 0; i < gltf.materials.size(); i++)
		{
			const fastgltf::Material& material = gltf.materials[i];
			MaterialData materialData;

			// Base color factor & texture

			materialData.BaseColorFactor = FastGLTFVec4ToGLMVec4(material.pbrData.baseColorFactor);

			if (material.pbrData.baseColorTexture.has_value())
			{
				const fastgltf::TextureInfo& texture = material.pbrData.baseColorTexture.value();

				materialData.BaseColorMapHandle = mTextureAssetHandles[texture.textureIndex];
			}

			// Normal scale & texture

			if (material.normalTexture.has_value())
			{
				const fastgltf::NormalTextureInfo& normalTexture = material.normalTexture.value();

				materialData.NormalScale     = normalTexture.scale;
				materialData.NormalMapHandle = mTextureAssetHandles[normalTexture.textureIndex];
			}

			// Occlusion, Roughness & Metallic
			
			// Occlusion strength, Metallic & Roughness factors

			if (material.occlusionTexture.has_value())
			{
				const fastgltf::OcclusionTextureInfo& texture = material.occlusionTexture.value();

				materialData.OcclusionStrength = texture.strength;
			}

			materialData.RoughnessFactor = material.pbrData.roughnessFactor;
			materialData.MetallicFactor = material.pbrData.metallicFactor;

			// Check if a packed occlusion roughness metallic texture exists and use that,

			if (material.packedOcclusionRoughnessMetallicTextures)
			{
				if (material.packedOcclusionRoughnessMetallicTextures->occlusionRoughnessMetallicTexture.has_value())
				{
					const fastgltf::TextureInfo& texture = material.packedOcclusionRoughnessMetallicTextures->occlusionRoughnessMetallicTexture.value();

					materialData.OcclusionRoughnessMetallicMapHandle = mTextureAssetHandles[texture.textureIndex];
				}
			}

			// otherwise, just use the metallic roughness texture

			else if (material.pbrData.metallicRoughnessTexture.has_value())
			{
				const fastgltf::TextureInfo& texture = material.pbrData.metallicRoughnessTexture.value();
				materialData.OcclusionRoughnessMetallicMapHandle = mTextureAssetHandles[texture.textureIndex];
			}

			// Emmissive factor & texture

			materialData.EmissiveFactor = FastGLTFVec3ToGLMVec3(material.emissiveFactor);

			if (material.emissiveTexture.has_value())
			{
				const fastgltf::TextureInfo& texture = material.emissiveTexture.value();
				materialData.EmissiveMapHandle = mTextureAssetHandles[texture.textureIndex];
			}

			MaterialInfo materialInfo = {
				.MaterialData = materialData,
				.Pipeline = Renderer::GetPBRPipeline()
			};

			AssetHandle materialAssetHandle = AssetManager::RegisterMaterial(materialInfo);
			mMaterialAssetHandles[i] = materialAssetHandle;
		}
	}

	void Model::LoadMeshes(const fastgltf::Asset& gltf)
	{
		CO_PROFILE_FN();

		mMeshAssetHandles.resize(gltf.meshes.size());

		std::vector<MeshVertex> vertices;
		std::vector<uint32_t> indices;

		vertices.reserve(1024);
		indices.reserve(1024);

		for (uint32_t i = 0; i < gltf.meshes.size(); i++)
		{
			const fastgltf::Mesh& mesh = gltf.meshes[i];

			vertices.clear();
			indices.clear();

			//std::vector<MeshSurface> surfaces;
			//surfaces.reserve(mesh.primitives.size());

			AssetHandle materialAssetHandle = CO_DEFAULT_MATERIAL_ASSET;

			for (const fastgltf::Primitive& primitive : mesh.primitives)
			{
				uint32_t firstVertexIndex  = vertices.size();
				uint32_t firstIndicesIndex = indices.size();

				//MeshSurface surface;
				//surface.FirstIndex = firstIndicesIndex;
				//surface.IndexCount = (uint32_t)gltf.accessors[primitive.indicesAccessor.value()].count;
				//surface.MaterialAssetHandle = CO_DEFAULT_MATERIAL_ASSET;

				// Set surface material

				if (primitive.materialIndex.has_value())
					materialAssetHandle = mMaterialAssetHandles[primitive.materialIndex.value()];

				//if (primitive.materialIndex.has_value())
					//surface.MaterialAssetHandle = mMaterialAssetHandles[primitive.materialIndex.value()];

				//surfaces.push_back(surface);

				//MaterialHandle materialHandle = Renderer::GetMaterialHandleFromAssetHandle(surface.MaterialAssetHandle);

				// Load indices

				const fastgltf::Accessor& indicesAccessor = gltf.accessors[primitive.indicesAccessor.value()];

				fastgltf::iterateAccessor<uint32_t>(gltf, indicesAccessor,
					[&indices, firstVertexIndex](uint32_t index)
					{
						indices.push_back(firstVertexIndex + index);
					}
				);

				// Load vertices

				const fastgltf::Accessor& verticesAccessor = gltf.accessors[primitive.findAttribute("POSITION")->accessorIndex];
				vertices.resize(vertices.size() + verticesAccessor.count);

				fastgltf::iterateAccessorWithIndex<fastgltf::math::nvec3>(gltf, verticesAccessor,
					[&vertices, firstVertexIndex](fastgltf::math::nvec3 position, size_t index)
					{
						vertices[firstVertexIndex + index].Position       = FastGLTFVec3ToGLMVec3(position);
						//vertices[firstVertexIndex + index].MaterialHandle = materialHandle;
					}
				);

				// Load normals

				if (auto normals = primitive.findAttribute("NORMAL"); normals != primitive.attributes.end())
				{
					fastgltf::iterateAccessorWithIndex<fastgltf::math::nvec3>(gltf, gltf.accessors[normals->accessorIndex],
						[&vertices, firstVertexIndex](fastgltf::math::nvec3 normal, size_t index)
						{
							vertices[firstVertexIndex + index].Normal = FastGLTFVec3ToGLMVec3(normal);
						}
					);
				}

				// Load UV coords

				if (auto uv = primitive.findAttribute("TEXCOORD_0"); uv != primitive.attributes.end())
				{
					fastgltf::iterateAccessorWithIndex<fastgltf::math::nvec2>(gltf, gltf.accessors[uv->accessorIndex],
						[&vertices, firstVertexIndex](fastgltf::math::nvec2 v, size_t index)
						{
							vertices[firstVertexIndex + index].TexCoordU = v.x();
							vertices[firstVertexIndex + index].TexCoordV = v.y();
						}
					);
				}
			}

			MeshInfo meshInfo = {
				.Vertices = vertices,
				.Indices  = indices,
				.MaterialRef = AssetManager::GetMaterial(materialAssetHandle)
			};

			AssetHandle meshAssetHandle = AssetManager::RegisterMesh(meshInfo);
			mMeshAssetHandles[i] = meshAssetHandle;
		}
	}

}