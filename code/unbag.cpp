#include <iostream>
#include <libbag.hpp>
#include <filesystem>
#include <iterator>
#include <fstream>
#include <sstream>
#include <map>
#include <ranges>

using unpack_result_container_type = std::map<libbag::key_type, libbag::content_type>;

int main(int p_argument_count, const char *p_argument_values[])
try
{
    std::span<const char *> arguments{p_argument_values, static_cast<std::size_t>(p_argument_count)};

    if (arguments.size() <= 1)
    {
        std::cout << "usage: unbag {bags...}" << std::endl;
        return 0;
    }

    for (const auto bag_path_c_str : arguments | std::views::drop(1))
    {
        std::filesystem::path path(bag_path_c_str);
        if (!std::filesystem::exists(path))
            continue;

        path.make_preferred();

        std::basic_ifstream<libbag::unit_type> input_file(path.c_str(), std::ios::binary | std::ios::ate);
        std::streamsize size = input_file.tellg();
        input_file.seekg(0, std::ios::beg);

        libbag::unit_vector_type input_content(size);
        if (!input_file.read(input_content.data(), size))
        {
            std::stringstream message;
            message << "Fail to read the content of the file '" << bag_path_c_str << "'.";
            throw std::runtime_error(message.str());
        }

        unpack_result_container_type unpacked;
        libbag::unpack_all(libbag::bag_type(input_content.begin(), input_content.end()), std::inserter(unpacked, unpacked.end()));

        for (const auto &[key, content] : unpacked)
        {
            std::basic_ofstream<libbag::unit_type> output_file(key, std::ios::binary);
            output_file.write(content.data(), content.size_bytes());
        }
    }

    return 0;
}
catch (const std::exception &e)
{
    std::cerr << "Error: " << e.what() << std::endl;
}