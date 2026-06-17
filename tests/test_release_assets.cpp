#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace
{
std::string readText(const std::filesystem::path& path)
{
    std::ifstream in(path);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}
}

TEST_CASE("QML resource manifest matches the shipped QML files")
{
#ifndef GHOST_REPLAY_TEST_SOURCE_DIR
    SKIP("Source directory is not available");
#else
    const auto source_dir = std::filesystem::path(GHOST_REPLAY_TEST_SOURCE_DIR);
    const auto qml_dir = source_dir / "resources" / "qml";
    const auto manifest_path = source_dir / "resources" / "qml.qrc";

    REQUIRE(std::filesystem::is_directory(qml_dir));
    REQUIRE(std::filesystem::is_regular_file(manifest_path));

    const std::string manifest = readText(manifest_path);
    std::set<std::string> manifest_files;

    const std::regex file_pattern(R"(<file(?:\s+alias="[^"]+")?>qml/([^<]+)</file>)");
    for (std::sregex_iterator it(manifest.begin(), manifest.end(), file_pattern), end; it != end; ++it)
    {
        manifest_files.insert((*it)[1].str());
        REQUIRE(std::filesystem::is_regular_file(qml_dir / (*it)[1].str()));
    }

    std::set<std::string> qml_files;
    for (const auto& entry : std::filesystem::directory_iterator(qml_dir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".qml")
            qml_files.insert(entry.path().filename().string());
    }

    REQUIRE(qml_files == manifest_files);
#endif
}

TEST_CASE("bundled UI fonts are present and attributed")
{
#ifndef GHOST_REPLAY_TEST_SOURCE_DIR
    SKIP("Source directory is not available");
#else
    const auto source_dir = std::filesystem::path(GHOST_REPLAY_TEST_SOURCE_DIR);
    const auto font_dir = source_dir / "resources" / "fonts";
    const auto resource_manifest = readText(source_dir / "resources" / "resources.qrc");
    const auto third_party = readText(source_dir / "docs" / "THIRD_PARTY_NOTICES.md");

    const std::vector<std::string> font_files = {
        "Inter-opsz-wght.ttf",
        "SpaceGrotesk-wght.ttf",
    };

    for (const auto& file : font_files)
    {
        INFO(file);
        REQUIRE(std::filesystem::is_regular_file(font_dir / file));
        REQUIRE(resource_manifest.find("fonts/" + file) != std::string::npos);
    }

    REQUIRE(std::filesystem::is_regular_file(font_dir / "OFL-Inter.txt"));
    REQUIRE(std::filesystem::is_regular_file(font_dir / "OFL-SpaceGrotesk.txt"));
    REQUIRE(third_party.find("Inter") != std::string::npos);
    REQUIRE(third_party.find("Space Grotesk") != std::string::npos);
#endif
}
