#pragma once
#include "Material.hpp"

namespace Cobalt
{

	class MaterialRegistry
	{
	public:
		static void RegisterMaterial(Material* material);

	private:
		inline static std::vector<Material*> sRegisteredMaterials;
	};

}
