#include "copch.hpp"
#include "Model.hpp"
#include "Vulkan/Renderer.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>

namespace Cobalt
{

	Model::Model(const std::string& modelpath)
	{
		CO_PROFILE_FN();

		LoadModel(modelpath);
	}

	Model::~Model()
	{
		CO_PROFILE_FN();
	}

	void Model::LoadModel(const std::string& modelPath)
	{
		CO_PROFILE_FN();

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(modelPath, aiProcess_Triangulate);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cerr << "Assimp Error: Failed to load module " << modelPath << std::endl;
		}

		ProcessNode(scene->mRootNode, scene);
	}

	void Model::ProcessNode(aiNode* node, const aiScene* scene)
	{
		CO_PROFILE_FN();

		// Store meshes

		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			mMeshes.push_back(ProcessMesh(mesh, scene));
		}

		// Recurse for each child

		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene);
		}
	}

	std::unique_ptr<Mesh> Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
	{
		CO_PROFILE_FN();

		std::vector<MeshVertex> vertices;
		std::vector<uint32_t> indices;

		for (uint32_t i = 0; i < mesh->mNumVertices; i++)
		{
			aiVector3D position = mesh->mVertices[i];
			aiVector3D normal = mesh->mNormals[i];
			aiVector3D texCoords = { 0.0f, 0.0f, 0.0f };

			if (mesh->HasTextureCoords(0))
				texCoords = mesh->mTextureCoords[0][i];

			vertices.push_back(MeshVertex {
				.Position = { position.x, position.y, position.z },
				.TexCoordU = texCoords.x,
				.Normal = { normal.x, normal.y, normal.z },
				.TexCoordV = texCoords.y
			});
		}

		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];

			std::vector<uint32_t> faceIndices(face.mIndices, face.mIndices + face.mNumIndices);
			indices.insert(indices.end(), faceIndices.begin(), faceIndices.end());
		}

		MaterialData materialData;

		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

			materialData.AlbedoMapHandle = LoadMaterialTexture(material, aiTextureType_DIFFUSE);
			//materialData.MetallicMapHandle = LoadMaterialTexture(material, );

			/*
			materialData.DiffuseMapHandle  = LoadMaterialTexture(material, aiTextureType_AMBIENT);
			materialData.SpecularMapHandle = LoadMaterialTexture(material, aiTextureType_SPECULAR);

			aiGetMaterialFloat(material, AI_MATKEY_SHININESS, &materialData.Shininess);
			if (materialData.Shininess == 0.0f)
				materialData.Shininess = 1.0f;
			*/
		}

		return std::make_unique<Mesh>(vertices, indices, materialData);
	}

	TextureHandle Model::LoadMaterialTexture(aiMaterial* material, aiTextureType type)
	{
		CO_PROFILE_FN();

		static std::unordered_map<std::string, TextureHandle> loadedTexturePaths;

		if (material->GetTextureCount(type) == 0)
			return 0;

		aiString path;
		material->GetTexture(type, 0, &path);

		std::string pathStr = path.C_Str();

		TextureHandle textureHandle = 0;

		if (loadedTexturePaths.find(pathStr) != loadedTexturePaths.end())
		{
			textureHandle = loadedTexturePaths.at(pathStr);
		}
		else
		{
			textureHandle = Renderer::CreateTexture(TextureInfo("CobaltApp/Assets/Sponza/" + std::string(pathStr)));
			loadedTexturePaths[pathStr] = textureHandle;
		}

		return textureHandle;
	}

}