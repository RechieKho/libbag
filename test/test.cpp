#include <catch2/catch_test_macros.hpp>
#include <libbag.hpp>
#include <map>
#include <sstream>
#include <iostream>

using collection_type = std::map<libbag::unit_string_type, libbag::unit_string_type>;
using unpack_result_container_type = std::map<libbag::key_type, libbag::content_type>;
static_assert(libbag::packing_container_c<collection_type>);

auto operator<<(std::ostream &p_stream, collection_type p_collection) -> std::ostream &
{
    p_stream << '{' << std::endl;
    for (const auto &[key, value] : p_collection)
        p_stream << "\t{ '" << key << "': '" << value << "' }," << std::endl;
    p_stream << '}' << std::endl;

    return p_stream;
}

TEST_CASE("Pack and unpack", "[libbag]")
{
    collection_type input{
        {"file_1", "abc."},
        {"file_2", "def."},
        {"file_3", "ghi"}};
    std::cout << "Input: " << input << std::endl;

    libbag::unit_string_type packed;

    {
        libbag::unit_stringstream_type stream;
        stream << "Prepended data should not be affected.";
        libbag::pack(input, stream);
        packed = stream.str();
    }

    unpack_result_container_type unpacked;
    libbag::unpack_all(libbag::bag_type(packed.begin(), packed.end()), std::inserter(unpacked, unpacked.end()));

    collection_type output;
    std::transform(unpacked.begin(), unpacked.end(), std::inserter(output, output.end()), [](const libbag::unpack_result_type &p_item)
                   { return std::pair(libbag::unit_string_type(p_item.first), libbag::unit_string_type(p_item.second.begin(), p_item.second.end())); });
    std::cout << "Output: " << output << std::endl;

    REQUIRE(input == output);
}