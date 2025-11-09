#include "perf/settings.hpp"
#include "tkit/serialization/yaml/perf/settings.hpp"
#include "tkit/reflection/perf/settings.hpp"
#include <argparse/argparse.hpp>
#include <filesystem>

namespace TKit::Perf
{
static std::string cliName(std::string p_Name)
{
    const auto pos = p_Name.find("Settings");
    if (pos != std::string::npos)
        p_Name.erase(pos, 8);

    std::string result{"--"};
    const char *ptr = p_Name.c_str();
    for (const char *c = ptr; *c != '\0'; ++c)
    {
        if (std::isupper(*c) && c != ptr && !std::isupper(c[-1]))
            result.push_back('-');

        result.push_back(static_cast<char>(std::tolower(*c)));
    }

    return result;
}

void LogSettings(const Settings &p_Settings)
{
    TKit::Reflect<Settings>::ForEachMemberField([&p_Settings](const auto &p_Field1) {
        using Type1 = TKIT_REFLECT_FIELD_TYPE(p_Field1);
        const std::string name = p_Field1.Name;
        Info("{}: ", name);
        const auto &setting = p_Field1.Get(p_Settings);

        TKit::Reflect<Type1>::ForEachMemberField([&setting](const auto &p_Field2) {
            using Type2 = TKIT_REFLECT_FIELD_TYPE(p_Field2);
            const std::string name = p_Field2.Name;
            const std::string value = std::to_string(p_Field2.Get(setting));
            Info("     {}: {}", name, value);
        });
    });
}

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
    TKit::Reflect<Settings>::ForEachMemberField([&](const auto &p_Field1) {
        using Type1 = TKIT_REFLECT_FIELD_TYPE(p_Field1);
        const std::string name = p_Field1.Name;
        const auto &setting = p_Field1.Get(settings);

        TKit::Reflect<Type1>::ForEachMemberField([&](const auto &p_Field2) {
            using Type2 = TKIT_REFLECT_FIELD_TYPE(p_Field2);
            const std::string value = std::to_string(p_Field2.Get(setting));
            argparse::Argument &arg = parser.add_argument(cliName(name + p_Field2.Name))
                                          .template scan<'i', usize>()
                                          .help("Auto generated settings argument. Default: " + value);
        });
    });

    parser.parse_args(argc, argv);

    if (const auto path = parser.present("--settings"))
        settings = TKit::Yaml::Deserialize<Settings>(*path);

    TKit::Reflect<Settings>::ForEachMemberField([&](const auto &p_Field1) {
        using Type1 = TKIT_REFLECT_FIELD_TYPE(p_Field1);
        auto &setting = p_Field1.Get(settings);
        const std::string name = p_Field1.Name;
        TKit::Reflect<Type1>::ForEachMemberField([&](const auto &p_Field2) {
            using Type2 = TKIT_REFLECT_FIELD_TYPE(p_Field2);
            if (const auto value = parser.present<Type2>(cliName(name + p_Field2.Name)))
                p_Field2.Set(setting, *value);
        });
    });

    if (parser.get<bool>("--export"))
        TKit::Yaml::Serialize(spath, settings);

    return settings;
}
} // namespace TKit::Perf
