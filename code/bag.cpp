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

    libbag::pack(
        file_list_reader_iterator_t(std::next(arguments.begin(), 2)),
        file_list_reader_iterator_t(arguments.end()),
        stream);

    return 0;
}
catch (std::exception e)
{
    std::cerr << "Error: " << e.what() << std::endl;
}