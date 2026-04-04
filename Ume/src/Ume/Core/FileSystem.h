#pragma once

#include "Ume/Core/Buffer.h"

namespace Ume {

	class FileSystem
	{
	public:
		// TODO: move to FileSystem class
		static Buffer ReadFileBinary(const std::filesystem::path& filepath);
	};

}
