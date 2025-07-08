#include "perf/settings.hpp"
#include "tkit/serialization/yaml/perf/settings.hpp"
#include "tkit/reflection/perf/settings.hpp"
#include <argparse/argparse.hpp>

namespace TKit::Perf
{
static std::string cliName(std::string p_Name) noexcept
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

Settings CreateSettings(int argc, char **argv)
{
    argparse::ArgumentParser parser{"toolkit-performance", TKIT_VERSION, argparse::default_arguments::all};
    parser.add_description("A small performance test playground for some of the toolkit utilities.");
    parser.add_epilog("For similar projects, visit my GitHub at https://github.com/ismawno");

    parser.add_argument("--settings")
        .help("A path pointing to a .yaml file with performance settings. The file must be compliant with the "
              "program's structure to work.");

    TKit::Reflect<Settings>::ForEachField([&parser](const auto &p_Field1) {
        using Type1 = TKIT_REFLECT_FIELD_TYPE(p_Field1);
        const std::string name = p_Field1.Name;
        TKit::Reflect<Type1>::ForEachField([&parser, &name](const auto &p_Field2) {
            using Type2 = TKIT_REFLECT_FIELD_TYPE(p_Field2);
            argparse::Argument &arg = parser.add_argument(cliName(name + p_Field2.Name))
                                          .template scan<'i', usize>()
                                          .help("Auto generated settings argument.");
        });
    });

    try
    {
        parser.parse_args(argc, argv);
    }
    catch (const std::exception &err)
    {
        std::cerr << err.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    Settings settings{};

    if (const auto path = parser.present("--settings"))
        settings = TKit::Yaml::Deserialize<Settings>(*path);

    TKit::Reflect<Settings>::ForEachField([&parser, &settings](const auto &p_Field1) {
        using Type1 = TKIT_REFLECT_FIELD_TYPE(p_Field1);
        auto &setting = p_Field1.Get(settings);
        TKit::Reflect<Type1>::ForEachField([&parser, &setting](const auto &p_Field2) {
            using Type2 = TKIT_REFLECT_FIELD_TYPE(p_Field2);
            if (const auto value = parser.present<Type2>(cliName(p_Field2.Name)))
                p_Field2.Set(setting, *value);
        });
    });

    return settings;
}
} // namespace TKit::Perf
