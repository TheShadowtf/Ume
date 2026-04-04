#include <yaml-cpp/yaml.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
    struct TextureBuildEntry
    {
        std::string RelativePath;
        uint32_t Width = 0;
        uint32_t Height = 0;
        uint32_t Channels = 4;
        uint32_t OriginalBytes = 0;
        std::vector<uint8_t> CompiledPixels;
        uint64_t Hash = 0;
    };

    struct CliOptions
    {
        fs::path ProjectFile;          // --project mode
        fs::path SourceDirectory;      // --source-dir mode (alternative to --project)
        std::string PackName;          // --pack-name  (used with --source-dir, default = dir name)
        fs::path OutputDirectory;
        std::vector<fs::path> CopyTargets;
        bool Verbose = false;
    };

    constexpr uint32_t kVPakVersion = 1;

    uint64_t Fnv1a64(const uint8_t* data, size_t size)
    {
        uint64_t hash = 14695981039346656037ull;
        for (size_t i = 0; i < size; i++)
        {
            hash ^= static_cast<uint64_t>(data[i]);
            hash *= 1099511628211ull;
        }
        return hash;
    }

    std::string ToLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    bool IsTexturePath(const fs::path& path)
    {
        const std::string ext = ToLower(path.extension().string());
        return ext == ".png"
            || ext == ".jpg"
            || ext == ".jpeg"
            || ext == ".tga"
            || ext == ".bmp"
            || ext == ".psd"
            || ext == ".gif"
            || ext == ".hdr"
            || ext == ".pic"
            || ext == ".pnm";
    }

    std::string EscapeJson(const std::string& text)
    {
        std::ostringstream out;
        for (char c : text)
        {
            switch (c)
            {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << c; break;
            }
        }
        return out.str();
    }

    bool ParseArguments(int argc, char** argv, CliOptions& outOptions)
    {
        for (int i = 1; i < argc; i++)
        {
            const std::string arg = argv[i];
            if (arg == "--project" && i + 1 < argc)
            {
                outOptions.ProjectFile = fs::path(argv[++i]);
            }
            else if (arg == "--output" && i + 1 < argc)
            {
                outOptions.OutputDirectory = fs::path(argv[++i]);
            }
            else if (arg == "--copy-to" && i + 1 < argc)
            {
                outOptions.CopyTargets.emplace_back(argv[++i]);
            }
            else if (arg == "--source-dir" && i + 1 < argc)
            {
                outOptions.SourceDirectory = fs::path(argv[++i]);
            }
            else if (arg == "--pack-name" && i + 1 < argc)
            {
                outOptions.PackName = argv[++i];
            }
            else if (arg == "--verbose")
            {
                outOptions.Verbose = true;
            }
            else if (arg == "--help" || arg == "-h")
            {
                return false;
            }
            else
            {
                std::cerr << "Unknown argument: " << arg << "\n";
                return false;
            }
        }

        if (outOptions.ProjectFile.empty() && outOptions.SourceDirectory.empty())
        {
            std::cerr << "Missing required argument: --project <path> or --source-dir <dir>\n";
            return false;
        }

        if (!outOptions.SourceDirectory.empty() && outOptions.PackName.empty())
            outOptions.PackName = outOptions.SourceDirectory.filename().string();

        return true;
    }

    std::string NormalizeRelativePath(const fs::path& base, const fs::path& absolutePath)
    {
        std::error_code ec;
        fs::path relative = fs::relative(absolutePath, base, ec);
        if (ec)
            relative = absolutePath.filename();

        std::string normalized = relative.generic_string();
        if (!normalized.empty() && normalized[0] == '/')
            normalized.erase(normalized.begin());
        return normalized;
    }

    bool CompileTexture(const fs::path& sourcePath, TextureBuildEntry& outEntry)
    {
        int width = 0;
        int height = 0;
        int channels = 0;

        stbi_set_flip_vertically_on_load(0);
        stbi_uc* pixels = stbi_load(sourcePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels)
            return false;

        const size_t bytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4ull;
        outEntry.Width = static_cast<uint32_t>(width);
        outEntry.Height = static_cast<uint32_t>(height);
        outEntry.Channels = 4;
        outEntry.OriginalBytes = static_cast<uint32_t>(bytes);
        outEntry.CompiledPixels.assign(pixels, pixels + bytes);
        outEntry.Hash = Fnv1a64(outEntry.CompiledPixels.data(), outEntry.CompiledPixels.size());

        stbi_image_free(pixels);
        return true;
    }

    bool LoadProjectData(const fs::path& projectFile, std::string& outProjectName, fs::path& outAssetDirectory)
    {
        YAML::Node data;
        try
        {
            data = YAML::LoadFile(projectFile.string());
        }
        catch (const YAML::ParserException& e)
        {
            std::cerr << "Failed to parse project file: " << e.what() << "\n";
            return false;
        }

        YAML::Node project = data["Project"];
        if (!project)
        {
            std::cerr << "Project file is missing 'Project' root node\n";
            return false;
        }

        outProjectName = project["Name"] ? project["Name"].as<std::string>() : projectFile.stem().string();
        const std::string assetDirectory = project["AssetDirectory"] ? project["AssetDirectory"].as<std::string>() : "Assets";
        outAssetDirectory = projectFile.parent_path() / fs::path(assetDirectory);
        return true;
    }

    bool BuildTextureEntries(const fs::path& assetDirectory, bool verbose, std::vector<TextureBuildEntry>& outEntries)
    {
        if (!fs::exists(assetDirectory) || !fs::is_directory(assetDirectory))
        {
            std::cerr << "Asset directory not found: " << assetDirectory.string() << "\n";
            return false;
        }

        for (const auto& entry : fs::recursive_directory_iterator(assetDirectory))
        {
            if (!entry.is_regular_file())
                continue;

            const fs::path sourcePath = entry.path();
            if (!IsTexturePath(sourcePath))
                continue;

            TextureBuildEntry textureEntry;
            textureEntry.RelativePath = NormalizeRelativePath(assetDirectory, sourcePath);

            if (!CompileTexture(sourcePath, textureEntry))
            {
                std::cerr << "Skipping unreadable texture: " << sourcePath.string() << "\n";
                continue;
            }

            if (verbose)
            {
                std::cout << "Compiled texture: " << textureEntry.RelativePath
                    << " (" << textureEntry.Width << "x" << textureEntry.Height << ")\n";
            }

            outEntries.emplace_back(std::move(textureEntry));
        }

        std::sort(outEntries.begin(), outEntries.end(), [](const TextureBuildEntry& a, const TextureBuildEntry& b)
        {
            return a.RelativePath < b.RelativePath;
        });

        return true;
    }

    bool WriteVPak(const fs::path& vpakPath, const std::vector<TextureBuildEntry>& entries)
    {
        struct TocRecord
        {
            uint32_t PathLength = 0;
            const std::string* Path = nullptr;
            uint32_t Width = 0;
            uint32_t Height = 0;
            uint32_t Channels = 0;
            uint32_t OriginalBytes = 0;
            uint64_t DataOffset = 0;
            uint64_t StoredBytes = 0;
            uint64_t Hash = 0;
        };

        const uint64_t headerSize = 4ull + sizeof(uint32_t) + sizeof(uint32_t);

        std::vector<TocRecord> toc;
        toc.reserve(entries.size());

        uint64_t tocSize = 0;
        for (const auto& entry : entries)
        {
            TocRecord record;
            record.PathLength = static_cast<uint32_t>(entry.RelativePath.size());
            record.Path = &entry.RelativePath;
            record.Width = entry.Width;
            record.Height = entry.Height;
            record.Channels = entry.Channels;
            record.OriginalBytes = entry.OriginalBytes;
            record.StoredBytes = static_cast<uint64_t>(entry.CompiledPixels.size());
            record.Hash = entry.Hash;
            toc.emplace_back(record);

            tocSize += sizeof(uint32_t);
            tocSize += record.PathLength;
            tocSize += sizeof(uint32_t) * 4ull;
            tocSize += sizeof(uint64_t) * 3ull;
        }

        uint64_t cursor = headerSize + tocSize;
        for (auto& record : toc)
        {
            record.DataOffset = cursor;
            cursor += record.StoredBytes;
        }

        std::ofstream out(vpakPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open())
        {
            std::cerr << "Failed to open output file: " << vpakPath.string() << "\n";
            return false;
        }

        const uint32_t entryCount = static_cast<uint32_t>(entries.size());

        out.write("VPAK", 4);
        out.write(reinterpret_cast<const char*>(&kVPakVersion), sizeof(uint32_t));
        out.write(reinterpret_cast<const char*>(&entryCount), sizeof(uint32_t));

        for (const auto& record : toc)
        {
            out.write(reinterpret_cast<const char*>(&record.PathLength), sizeof(uint32_t));
            out.write(record.Path->data(), record.PathLength);
            out.write(reinterpret_cast<const char*>(&record.Width), sizeof(uint32_t));
            out.write(reinterpret_cast<const char*>(&record.Height), sizeof(uint32_t));
            out.write(reinterpret_cast<const char*>(&record.Channels), sizeof(uint32_t));
            out.write(reinterpret_cast<const char*>(&record.OriginalBytes), sizeof(uint32_t));
            out.write(reinterpret_cast<const char*>(&record.DataOffset), sizeof(uint64_t));
            out.write(reinterpret_cast<const char*>(&record.StoredBytes), sizeof(uint64_t));
            out.write(reinterpret_cast<const char*>(&record.Hash), sizeof(uint64_t));
        }

        for (size_t i = 0; i < entries.size(); i++)
        {
            const auto& payload = entries[i].CompiledPixels;
            if (!payload.empty())
                out.write(reinterpret_cast<const char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
        }

        out.flush();
        return out.good();
    }

    bool WriteManifest(const fs::path& manifestPath, const std::string& projectName, const std::vector<TextureBuildEntry>& entries)
    {
        std::ofstream manifest(manifestPath, std::ios::out | std::ios::trunc);
        if (!manifest.is_open())
        {
            std::cerr << "Failed to write manifest: " << manifestPath.string() << "\n";
            return false;
        }

        manifest << "{\n";
        manifest << "  \"format\": \"UmeVPackManifest\",\n";
        manifest << "  \"version\": " << kVPakVersion << ",\n";
        manifest << "  \"project\": \"" << EscapeJson(projectName) << "\",\n";
        manifest << "  \"assetClass\": \"Textures\",\n";
        manifest << "  \"entryCount\": " << entries.size() << ",\n";
        manifest << "  \"entries\": [\n";

        for (size_t i = 0; i < entries.size(); i++)
        {
            const auto& entry = entries[i];
            manifest << "    {\n";
            manifest << "      \"path\": \"" << EscapeJson(entry.RelativePath) << "\",\n";
            manifest << "      \"width\": " << entry.Width << ",\n";
            manifest << "      \"height\": " << entry.Height << ",\n";
            manifest << "      \"channels\": " << entry.Channels << ",\n";
            manifest << "      \"compiledBytes\": " << entry.CompiledPixels.size() << ",\n";
            manifest << "      \"hash64\": \"0x" << std::hex << std::setw(16) << std::setfill('0') << entry.Hash << std::dec << "\"\n";
            manifest << "    }" << (i + 1 < entries.size() ? "," : "") << "\n";
        }

        manifest << "  ]\n";
        manifest << "}\n";

        return true;
    }

    bool CopyOutputs(const fs::path& sourceVpak, const fs::path& sourceManifest, const std::vector<fs::path>& targets)
    {
        std::error_code ec;
        for (const auto& target : targets)
        {
            fs::create_directories(target, ec);
            if (ec)
            {
                std::cerr << "Failed to create copy target directory: " << target.string() << " - " << ec.message() << "\n";
                return false;
            }

            const fs::path targetVpak = target / sourceVpak.filename();
            const fs::path targetManifest = target / sourceManifest.filename();

            fs::copy_file(sourceVpak, targetVpak, fs::copy_options::overwrite_existing, ec);
            if (ec)
            {
                std::cerr << "Failed to copy vpak file to " << targetVpak.string() << " - " << ec.message() << "\n";
                return false;
            }

            fs::copy_file(sourceManifest, targetManifest, fs::copy_options::overwrite_existing, ec);
            if (ec)
            {
                std::cerr << "Failed to copy manifest file to " << targetManifest.string() << " - " << ec.message() << "\n";
                return false;
            }
        }

        return true;
    }

    void PrintUsage()
    {
        std::cout
            << "UmeVPackTool\n"
            << "Usage (project mode):\n"
            << "  UmeVPackTool --project <path/to/project.uproj> [--output <dir>] [--copy-to <dir>]... [--verbose]\n"
            << "Usage (source-dir mode):\n"
            << "  UmeVPackTool --source-dir <dir> --pack-name <name> [--output <dir>] [--copy-to <dir>]... [--verbose]\n"
            << "\n"
            << "Behavior:\n"
            << "  - Compiles all texture assets found recursively into a single <packname>.vpak\n"
            << "  - Emits a <packname>.vpak.manifest.json alongside the package\n"
            << "  - Optionally copies both files into one or more runtime/export directories\n";
    }
}

int main(int argc, char** argv)
{
    CliOptions options;
    if (!ParseArguments(argc, argv, options))
    {
        PrintUsage();
        return 1;
    }

    std::string projectName;
    fs::path assetDirectory;

    if (!options.SourceDirectory.empty())
    {
        // --source-dir mode: pack any directory directly without a .uproj
        if (!fs::exists(options.SourceDirectory) || !fs::is_directory(options.SourceDirectory))
        {
            std::cerr << "Source directory does not exist: " << options.SourceDirectory.string() << "\n";
            return 1;
        }
        projectName = options.PackName;
        assetDirectory = options.SourceDirectory;
    }
    else
    {
        // --project mode
        if (!fs::exists(options.ProjectFile))
        {
            std::cerr << "Project file does not exist: " << options.ProjectFile.string() << "\n";
            return 1;
        }
        if (!LoadProjectData(options.ProjectFile, projectName, assetDirectory))
            return 1;
        if (options.PackName.empty())
            options.PackName = "Textures";
    }

    std::vector<TextureBuildEntry> entries;
    if (!BuildTextureEntries(assetDirectory, options.Verbose, entries))
        return 1;

    if (entries.empty())
        std::cout << "No textures found to package in: " << assetDirectory.string() << "\n";

    fs::path outputDirectory = options.OutputDirectory;
    if (outputDirectory.empty())
    {
        outputDirectory = options.ProjectFile.empty()
            ? options.SourceDirectory.parent_path()
            : options.ProjectFile.parent_path() / "Build" / "Paks";
    }

    std::error_code ec;
    fs::create_directories(outputDirectory, ec);
    if (ec)
    {
        std::cerr << "Failed to create output directory: " << outputDirectory.string() << " - " << ec.message() << "\n";
        return 1;
    }

    const fs::path vpakPath = outputDirectory / (options.PackName + ".vpak");
    const fs::path manifestPath = outputDirectory / (options.PackName + ".vpak.manifest.json");

    if (!WriteVPak(vpakPath, entries))
        return 1;

    if (!WriteManifest(manifestPath, projectName, entries))
        return 1;

    if (!options.CopyTargets.empty())
    {
        if (!CopyOutputs(vpakPath, manifestPath, options.CopyTargets))
            return 1;
    }

    std::cout << "VPAK build complete\n";
    std::cout << "Pack: " << options.PackName << "\n";
    std::cout << "Asset Source: " << assetDirectory.string() << "\n";
    std::cout << "Output: " << vpakPath.string() << "\n";
    std::cout << "Textures Compiled: " << entries.size() << "\n";

    return 0;
}
