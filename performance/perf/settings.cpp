#include "perf/settings.hpp"
#include "tkit/serialization/yaml/perf/settings.hpp"
#include "tkit/reflection/perf/settings.hpp"
#include <argparse/argparse.hpp>
#include <filesystem>

namespace TKit
{
static std::string cliName(std::string name)
{
    const auto pos = name.find("Settings");
    if (pos != std::string::npos)
        name.erase(pos, 8);

    std::string result{"--"};
    const char *ptr = name.c_str();
    for (const char *c = ptr; *c != '\0'; ++c)
    {
        if (std::isupper(*c) && c != ptr && !std::isupper(c[-1]))
            result.push_back('-');

        result.push_back(static_cast<char>(std::tolower(*c)));
    }

    return result;
}

#ifdef TKIT_ENABLE_INFO_LOGS
void LogSettings(const Settings &settings)
{
    TKit::Reflect<Settings>::ForEachMemberField([&settings](const auto &field1) {
        using Type1 = TKIT_REFLECT_FIELD_TYPE(field1);
        const std::string name = field1.Name;
        TKIT_LOG_INFO("{}: ", name);
        const auto &setting = field1.Get(settings);

        TKit::Reflect<Type1>::ForEachMemberField([&setting](const auto &field2) {
            using Type2 = TKIT_REFLECT_FIELD_TYPE(field2);
            const std::string name = field2.Name;
            const std::string value = std::to_string(field2.Get(setting));
            TKIT_LOG_INFO("     {}: {}", name, value);
        });
    });
}
#endif

Settings CreateSettings(int argc, char **argv)
{
    std::filesystem::create_directories(TKIT_ROOT_PATH "/performance/results");

    argparse::ArgumentParser parser{"toolkit-performance", TKIT_VERSION, argparse::default_arguments::all};
    parser.add_description("A small performance test playground for some of the toolkit utilities.");
    parser.add_epilog("For similar projects, visit my GitHub at https://github.com/ismawno");

    const std::string spath = TKIT_ROOT_PATH "/performance/perf-settings.yaml";
    parser.add_argument("-e", "--export")
        .flag()
        .help("If selected, a configuration file will be exported to: " + spath);
    parser.add_argument("-s", "--settings")
        .help("A path pointing to a .yaml file with performance settings. The file must be compliant with the "
              "program's structure to work.");

    Settings settings{};
    TKit::Reflect<Settings>::ForEachMemberField([&](const auto &field1) {
        using Type1 = TKIT_REFLECT_FIELD_TYPE(field1);
        const std::string name = field1.Name;
        const auto &setting = field1.Get(settings);

        TKit::Reflect<Type1>::ForEachMemberField([&](const auto &field2) {
            using Type2 = TKIT_REFLECT_FIELD_TYPE(field2);
            const std::string value = std::to_string(field2.Get(setting));
            argparse::Argument &arg = parser.add_argument(cliName(name + field2.Name))
                                          .template scan<'i', usize>()
                                          .help("Auto generated settings argument. Default: " + value);
        });
    });

    parser.parse_args(argc, argv);

    if (const auto path = parser.present("--settings"))
        settings = TKit::Yaml::Deserialize<Settings>(*path);

    TKit::Reflect<Settings>::ForEachMemberField([&](const auto &field1) {
        using Type1 = TKIT_REFLECT_FIELD_TYPE(field1);
        auto &setting = field1.Get(settings);
        const std::string name = field1.Name;
        TKit::Reflect<Type1>::ForEachMemberField([&](const auto &field2) {
            using Type2 = TKIT_REFLECT_FIELD_TYPE(field2);
            if (const auto value = parser.present<Type2>(cliName(name + field2.Name)))
                field2.Set(setting, *value);
        });
    });

    if (parser.get<bool>("--export"))
        TKit::Yaml::Serialize(spath, settings);

    return settings;
}
} // namespace TKit
