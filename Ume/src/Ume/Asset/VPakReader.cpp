#include "umepch.h"
#include "VPakReader.h"

#include "Ume/Renderer/Texture.h"

#include <fstream>
#include <cstring>

namespace Ume {

	namespace {

		constexpr uint32_t kExpectedVersion = 1;

		bool ReadU32(std::ifstream& f, uint32_t& out)
		{
			return static_cast<bool>(f.read(reinterpret_cast<char*>(&out), sizeof(uint32_t)));
		}

		bool ReadU64(std::ifstream& f, uint64_t& out)
		{
			return static_cast<bool>(f.read(reinterpret_cast<char*>(&out), sizeof(uint64_t)));
		}
	}

	// ---------------------------------------------------------------------------
	// VPakReader
	// ---------------------------------------------------------------------------

	Ref<VPakReader> VPakReader::Open(const std::filesystem::path& vpakPath)
	{
		// CreateRef cannot be used here since the constructor is private.
		// Use a shared_ptr via make_shared with a helper struct instead.
		struct Passkey {};
		class OpenableReader : public VPakReader
		{
		public:
			explicit OpenableReader(Passkey) {}
		};

		auto reader = CreateRef<OpenableReader>(Passkey{});
		if (!static_cast<VPakReader*>(reader.get())->Load(vpakPath))
			return nullptr;
		return reader;
	}

	bool VPakReader::Load(const std::filesystem::path& path)
	{
		m_SourcePath = path;

		std::ifstream f(path, std::ios::binary);
		if (!f.is_open())
		{
			UME_CORE_WARN("VPakReader: could not open '{}'", path.string());
			return false;
		}

		// Magic
		char magic[4] = {};
		if (!f.read(magic, 4) || std::memcmp(magic, "VPAK", 4) != 0)
		{
			UME_CORE_WARN("VPakReader: '{}' has invalid magic", path.string());
			return false;
		}

		uint32_t version = 0;
		if (!ReadU32(f, version) || version != kExpectedVersion)
		{
			UME_CORE_WARN("VPakReader: '{}' has unsupported version {}", path.string(), version);
			return false;
		}

		uint32_t entryCount = 0;
		if (!ReadU32(f, entryCount))
			return false;

		for (uint32_t i = 0; i < entryCount; i++)
		{
			uint32_t pathLen = 0;
			if (!ReadU32(f, pathLen))
				return false;

			std::string relativePath(pathLen, '\0');
			if (!f.read(relativePath.data(), pathLen))
				return false;

			VPakEntry entry;
			uint32_t originalBytes = 0;
			if (!ReadU32(f, entry.Width))    return false;
			if (!ReadU32(f, entry.Height))   return false;
			if (!ReadU32(f, entry.Channels)) return false;
			if (!ReadU32(f, originalBytes))  return false;
			if (!ReadU64(f, entry.DataOffset))  return false;
			if (!ReadU64(f, entry.StoredBytes)) return false;
			if (!ReadU64(f, entry.Hash64))      return false;

			m_Index.emplace(std::move(relativePath), entry);
		}

		m_Valid = true;
		return true;
	}

	bool VPakReader::HasEntry(const std::string& relativePath) const
	{
		return m_Index.count(relativePath) > 0;
	}

	std::vector<std::string> VPakReader::GetAllPaths() const
	{
		std::vector<std::string> paths;
		paths.reserve(m_Index.size());
		for (const auto& [k, v] : m_Index)
			paths.emplace_back(k);
		return paths;
	}

	Ref<Texture2D> VPakReader::CreateTexture(const std::string& relativePath) const
	{
		auto it = m_Index.find(relativePath);
		if (it == m_Index.end())
		{
			UME_CORE_WARN("VPakReader: entry '{}' not found in '{}'", relativePath, m_SourcePath.string());
			return nullptr;
		}

		const VPakEntry& entry = it->second;
		if (entry.StoredBytes == 0)
		{
			UME_CORE_WARN("VPakReader: entry '{}' has no data", relativePath);
			return nullptr;
		}

		std::ifstream f(m_SourcePath, std::ios::binary);
		if (!f.is_open())
		{
			UME_CORE_WARN("VPakReader: could not re-open '{}' for texture data", m_SourcePath.string());
			return nullptr;
		}

		f.seekg(static_cast<std::streamoff>(entry.DataOffset));
		if (!f)
		{
			UME_CORE_WARN("VPakReader: seek failed in '{}' for entry '{}'", m_SourcePath.string(), relativePath);
			return nullptr;
		}

		std::vector<uint8_t> pixels(static_cast<size_t>(entry.StoredBytes));
		if (!f.read(reinterpret_cast<char*>(pixels.data()), static_cast<std::streamsize>(entry.StoredBytes)))
		{
			UME_CORE_WARN("VPakReader: read failed for '{}' in '{}'", relativePath, m_SourcePath.string());
			return nullptr;
		}

		TextureSpecification spec;
		spec.Width  = entry.Width;
		spec.Height = entry.Height;
		// Packer always stores RGBA8; honour that regardless of original channels.
		spec.Format = ImageFormat::RGBA8;
		spec.GenerateMips = false;

		Ref<Texture2D> tex = Texture2D::Create(spec);
		if (!tex)
			return nullptr;

		tex->SetData(pixels.data(), static_cast<uint32_t>(pixels.size()));
		return tex;
	}

	// ---------------------------------------------------------------------------
	// VPakRegistry
	// ---------------------------------------------------------------------------

	std::unordered_map<std::string, Ref<VPakReader>> VPakRegistry::s_Mounted;

	void VPakRegistry::Mount(const std::string& name, const std::filesystem::path& vpakPath)
	{
		auto reader = VPakReader::Open(vpakPath);
		if (!reader)
		{
			UME_CORE_WARN("VPakRegistry: failed to mount '{}' from '{}' — images will load from disk", name, vpakPath.string());
			return;
		}
		s_Mounted[name] = reader;
		UME_CORE_INFO("VPakRegistry: mounted '{}' ({} textures) from '{}'", name, reader->GetAllPaths().size(), vpakPath.string());
	}

	void VPakRegistry::Unmount(const std::string& name)
	{
		s_Mounted.erase(name);
	}

	void VPakRegistry::UnmountAll()
	{
		s_Mounted.clear();
	}

	Ref<Texture2D> VPakRegistry::FindTexture(const std::string& relativePath)
	{
		for (const auto& [name, reader] : s_Mounted)
		{
			if (reader && reader->HasEntry(relativePath))
				return reader->CreateTexture(relativePath);
		}
		return nullptr;
	}

	Ref<Texture2D> VPakRegistry::FindTextureInPack(const std::string& packName, const std::string& relativePath)
	{
		auto it = s_Mounted.find(packName);
		if (it == s_Mounted.end() || !it->second)
			return nullptr;
		return it->second->CreateTexture(relativePath);
	}

	bool VPakRegistry::Contains(const std::string& relativePath)
	{
		for (const auto& [name, reader] : s_Mounted)
		{
			if (reader && reader->HasEntry(relativePath))
				return true;
		}
		return false;
	}

	const std::unordered_map<std::string, Ref<VPakReader>>& VPakRegistry::GetMounted()
	{
		return s_Mounted;
	}

}
