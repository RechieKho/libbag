#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <map>
#include <libbag.hpp>

template <typename T>
concept file_path_list_iterator_c = requires {
    requires std::is_convertible_v<std::iter_value_t<T>, std::filesystem::path>;
};

template <typename T>
concept file_path_list_container_c = requires {
    requires file_path_list_iterator_c<decltype(std::declval<T>().begin())>;
    requires file_path_list_iterator_c<decltype(std::declval<T>().end())>;
};

template <file_path_list_iterator_c Iterator>
class file_list_reader_iterator_t
{
public:
    using iterator_type = Iterator;
    using difference_type = void;
    using value_type = std::pair<libbag::unit_string_type, libbag::unit_vector_type>;
    using pointer = const value_type *;
    using reference = const value_type &;
    using iterator_category = std::output_iterator_tag;

private:
    iterator_type it;
    mutable value_type cache;

    static auto read_file(const char *p_path_c_str) -> libbag::unit_vector_type
    {
        std::basic_ifstream<libbag::unit_type> stream(p_path_c_str, std::ios::binary | std::ios::ate);
        std::streamsize size = stream.tellg();
        stream.seekg(0, std::ios::beg);

        libbag::unit_vector_type content(size);
        if (!stream.read(content.data(), size))
        {
            std::stringstream message;
            message << "Fail to read the content of the file '" << p_path_c_str << "'.";
            throw std::runtime_error(message.str());
        }

        return content;
    }

    auto update_cache() const -> void
    {
        const std::filesystem::path path = *it;
        if (cache.first == libbag::unit_string_type(path.generic_string()))
            return;

        const libbag::unit_vector_type content = read_file(path.c_str());
        cache = value_type{libbag::unit_string_type(path.generic_string()), std::move(content)};
    }

public:
    constexpr file_list_reader_iterator_t() = default;

    file_list_reader_iterator_t(iterator_type p_it)
        : it(p_it) {}

    auto operator*() const -> reference
    {
        update_cache();
        return cache;
    }

    auto operator->() const -> pointer
    {
        update_cache();
        return &cache;
    }

    auto operator++() -> file_list_reader_iterator_t &
    {
        ++it;
        return *this;
    }

    auto operator++(int) -> file_list_reader_iterator_t &
    {
        file_list_reader_iterator_t temp = *this;
        ++(*this);
        return temp;
    }

    auto operator==(const file_list_reader_iterator_t &p_reader) const -> bool
    {
        return p_reader.it == it;
    }
};

auto glob_regular_file_path(const std::vector<std::filesystem::path> &p_paths) -> std::vector<std::filesystem::path>
{
    std::vector<std::filesystem::path> result;
    std::copy_if(p_paths.begin(), p_paths.end(), std::back_inserter(result), [](const std::filesystem::path p_path)
                 { return std::filesystem::directory_entry(p_path).is_regular_file(); });

    std::vector<std::filesystem::path> directories;
    std::copy_if(p_paths.begin(), p_paths.end(), std::back_inserter(directories), [](const std::filesystem::path p_path)
                 { return std::filesystem::directory_entry(p_path).is_directory(); });

    for (const std::filesystem::path &directory_path : directories)
        for (const std::filesystem::directory_entry &p_entry : std::filesystem::recursive_directory_iterator(directory_path))
            if (p_entry.is_regular_file())
                result.push_back(p_entry.path());

    return result;
}

int main(int p_argument_count, const char *p_argument_values[])
try
{

    std::span<const char *> arguments{p_argument_values, static_cast<std::size_t>(p_argument_count)};

    if (arguments.size() <= 2)
    {
        std::cout << "usage: bag {output_path} {paths...}" << std::endl;
        return 0;
    }

    std::basic_ofstream<libbag::unit_type> stream(arguments[1], std::ios::binary);

    std::vector<std::filesystem::path> input_paths;
    std::transform(std::next(arguments.begin(), 2), arguments.end(), std::back_inserter(input_paths), [](const char *p_argument)
                   { return std::filesystem::path(p_argument); });

    std::vector<std::filesystem::path> input_regular_file_paths = glob_regular_file_path(input_paths);

    libbag::pack(
        file_list_reader_iterator_t(input_regular_file_paths.begin()),
        file_list_reader_iterator_t(input_regular_file_paths.end()),
        stream);

    return 0;
}
catch (const std::exception &e)
{
    std::cerr << "Error: " << e.what() << std::endl;
}