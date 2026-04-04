#pragma once

#include "Ume/Core/Base.h"
#include "Ume/Renderer/Texture.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace Ume {

	// VPak binary layout (v1, all values little-endian):
	//   [0]  4 bytes  magic: 'V','P','A','K'
	//   [4]  4 bytes  uint32  version (currently 1)
	//   [8]  4 bytes  uint32  entryCount
	//   [12] per entry:
	//     4    uint32   pathLength
	//     N    char[]   UTF-8 path (no null terminator, length == pathLength)
	//     4    uint32   width
	//     4    uint32   height
	//     4    uint32   channels
	//     4    uint32   originalBytes
	//     8    uint64   dataOffset (absolute byte offset inside the file)
	//     8    uint64   storedBytes
	//     8    uint64   hash64
	//   <data blocks follow, addressed by dataOffset>

	struct VPakEntry
	{
		uint32_t Width       = 0;
		uint32_t Height      = 0;
		uint32_t Channels    = 4;
		uint64_t DataOffset  = 0;
		uint64_t StoredBytes = 0;
		uint64_t Hash64      = 0;
	};

	// Lightweight in-memory index over a single .vpak file.
	// Does NOT load pixel data until CreateTexture() is called.
	class VPakReader
	{
	public:
		static Ref<VPakReader> Open(const std::filesystem::path& vpakPath);

		// Returns true if the file was read and the header is valid.
		bool IsValid() const { return m_Valid; }

		bool HasEntry(const std::string& relativePath) const;

		// Allocates and uploads an OpenGL texture for the named entry.
		// Returns nullptr if the entry does not exist or the read fails.
		Ref<Texture2D> CreateTexture(const std::string& relativePath) const;

		// Enumerate all packed paths (for diagnostics / editor use).
		std::vector<std::string> GetAllPaths() const;

		const std::filesystem::path& GetSourcePath() const { return m_SourcePath; }

	private:
		VPakReader() = default;

		bool Load(const std::filesystem::path& path);

		std::filesystem::path m_SourcePath;
		std::unordered_map<std::string, VPakEntry> m_Index;
		bool m_Valid = false;
	};

	// Global registry that lets engine/editor code mount a VPak by name
	// and resolve texture paths transparently.
	class VPakRegistry
	{
	public:
		// Mount a vpak under a logical name.  Re-mounts silently if already open.
		static void Mount(const std::string& name, const std::filesystem::path& vpakPath);

		// Unmount a previously mounted vpak.
		static void Unmount(const std::string& name);

		static void UnmountAll();

		// Resolve a texture by searching all mounted vpaks.
		// Returns nullptr if not found in any pack.
		static Ref<Texture2D> FindTexture(const std::string& relativePath);

		// Find in a specific mounted pack.
		static Ref<Texture2D> FindTextureInPack(const std::string& packName, const std::string& relativePath);

		// Whether any mounted pack contains the given path.
		static bool Contains(const std::string& relativePath);

		static const std::unordered_map<std::string, Ref<VPakReader>>& GetMounted();

	private:
		static std::unordered_map<std::string, Ref<VPakReader>> s_Mounted;
	};

}
