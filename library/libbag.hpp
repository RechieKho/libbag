#ifndef LIBBAG_HPP
#define LIBBAG_HPP

#include <iterator>
#include <ostream>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <filesystem>

/// libbag
/// A bundling algorithm.
///
/// Memory map:
/// | key | null byte | content | ... | indices | metadata_t |
///
/// The metadata_t validate and specify the indices.
/// The indices stores the location and size of each item.
/// An item has a key and content, separated by a null byte.
/// This also means the key with the null byte forms a C string.

namespace libbag
{
    template <typename T>
    concept has_char_trait = requires { typename std::char_traits<T>::char_type; };

    using unit_type = char;
    static_assert(sizeof(unit_type) == 1);
    static_assert(has_char_trait<unit_type>);
    using unit_string_view_type = std::basic_string_view<unit_type>;
    using unit_span_type = std::span<const unit_type>;
    using unit_string_type = std::basic_string<unit_type>;
    using unit_vector_type = std::vector<unit_type>;
    using unit_stringstream_type = std::basic_stringstream<unit_type>;

    using size_type = uint64_t;
    using key_type = unit_string_view_type;
    using content_type = unit_span_type;
    using bag_type = unit_span_type;

    constexpr const unit_type null_unit = '\0';
    constexpr const size_type identifier_mark = 0xBABAFAFA;

    template <typename T>
    concept unique_object_representations_c = std::has_unique_object_representations_v<T>;

    struct slice_t
    {
        size_type byte_offset;
        size_type byte_count;

        constexpr slice_t(size_type p_byte_offset, size_type p_byte_count)
            : byte_offset(p_byte_offset), byte_count(p_byte_count) {}
    };
    static_assert(unique_object_representations_c<slice_t>);
    using slice_view_type = std::span<const slice_t>;

    struct metadata_t
    {
        size_type mark;
        size_type true_byte_count;
        slice_t index_page;

        constexpr metadata_t(size_type p_mark, size_type p_true_byte_count, slice_t p_index_page)
            : mark(p_mark), true_byte_count(p_true_byte_count), index_page(p_index_page) {}
    };
    static_assert(unique_object_representations_c<metadata_t>);

    template <typename T>
    class memory_view_iterator_t
    {
    public:
        using difference_type = int64_t;
        using value_type = T;
        using pointer = const T *;
        using reference = const T &;
        using iterator_category = std::contiguous_iterator_tag;

    private:
        pointer value_pointer;

    public:
        constexpr memory_view_iterator_t() = default;

        constexpr memory_view_iterator_t(pointer p_value_pointer)
            : value_pointer(p_value_pointer) {}

        auto operator*() const -> reference { return *value_pointer; }

        auto operator->() const -> pointer { return value_pointer; }

        auto operator[](difference_type p_index) const -> reference { return value_pointer[p_index]; }

        auto operator+(difference_type p_difference) const -> memory_view_iterator_t { return memory_view_iterator_t(value_pointer + p_difference); }

        auto operator+=(difference_type p_difference) -> memory_view_iterator_t & { return *this = *this + p_difference; }

        auto operator-(memory_view_iterator_t p_memory) const -> difference_type { return value_pointer - p_memory.value_pointer; }

        auto operator-(difference_type p_difference) const -> memory_view_iterator_t { return memory_view_iterator_t(value_pointer - p_difference); }

        auto operator-=(difference_type p_difference) -> memory_view_iterator_t & { return *this = *this - p_difference; }

        auto operator++() -> memory_view_iterator_t &
        {
            ++value_pointer;
            return *this;
        }

        auto operator++(int) -> memory_view_iterator_t
        {
            memory_view_iterator_t temp = *this;
            ++(*this);
            return temp;
        }

        auto operator--() -> memory_view_iterator_t &
        {
            --value_pointer;
            return *this;
        }

        auto operator--(int) -> memory_view_iterator_t
        {
            memory_view_iterator_t temp = *this;
            --(*this);
            return temp;
        }

        auto operator<=>(const memory_view_iterator_t &) const = default;

        friend auto operator+(typename memory_view_iterator_t::difference_type p_difference, const memory_view_iterator_t &p_memory) -> memory_view_iterator_t;
        friend auto operator-(typename memory_view_iterator_t::difference_type p_difference, const memory_view_iterator_t &p_memory) -> memory_view_iterator_t;
    };

    template <typename T>
    auto operator+(typename memory_view_iterator_t<T>::difference_type p_difference, const memory_view_iterator_t<T> &p_memory) -> memory_view_iterator_t<T> { return p_memory + p_difference; }

    template <typename T>
    auto operator-(typename memory_view_iterator_t<T>::difference_type p_difference, const memory_view_iterator_t<T> &p_memory) -> memory_view_iterator_t<T> { return p_memory - p_difference; }

    template <typename = void>
    auto operator<<(std::basic_ostream<unit_type> &p_stream, const unit_span_type &p_units) -> std::basic_ostream<unit_type> &
    {
        for (const auto &e : p_units)
            p_stream << e;

        return p_stream;
    }

    template <unique_object_representations_c T>
    auto serialize(std::basic_ostream<unit_type> &p_stream, const T &p_object) -> std::basic_ostream<unit_type> &
    {
        const auto object_unit_pointer = reinterpret_cast<const unit_type *>(&p_object);
        const auto object_end_unit_pointer = object_unit_pointer + sizeof(p_object);

        for (auto it = object_unit_pointer; it != object_end_unit_pointer; ++it)
            p_stream << *it;

        return p_stream;
    }

    template <typename = void>
    auto operator<<(std::basic_ostream<unit_type> &p_stream, const metadata_t &p_metadata_t) -> std::basic_ostream<unit_type> &
    {
        return serialize(p_stream, p_metadata_t);
    }

    template <typename = void>
    auto operator<<(std::basic_ostream<unit_type> &p_stream, const slice_t &p_slice_t) -> std::basic_ostream<unit_type> &
    {
        return serialize(p_stream, p_slice_t);
    }

    template <typename T>
    concept slice_iterator_c = std::same_as<std::iter_value_t<T>, slice_t>;

    template <typename T>
    concept slice_container_c = requires {
        requires slice_iterator_c<decltype(std::declval<T>().begin())>;
        requires slice_iterator_c<decltype(std::declval<T>().end())>;
    };

    template <slice_container_c C>
    auto operator<<(std::basic_ostream<unit_type> &p_stream, const C &p_slices) -> std::basic_ostream<unit_type> &
    {
        for (const auto &slice : p_slices)
            p_stream << slice;

        return p_stream;
    }

    template <typename T>
    concept packing_iterator_c = requires {
        requires std::is_convertible_v<decltype(std::get<0>(std::declval<std::iter_value_t<T>>())), key_type>;
        requires std::is_convertible_v<decltype(std::get<1>(std::declval<std::iter_value_t<T>>())), content_type>;
    };

    template <packing_iterator_c Iterator>
    auto pack(Iterator p_begin, Iterator p_end, std::basic_ostream<unit_type> &p_output) -> void
    {
        std::vector<slice_t> indices;
        size_type current_byte_offset = 0;

        for (auto it = p_begin; it != p_end; ++it)
        {
            const auto &[raw_key, raw_content] = *it;
            const auto key = key_type(raw_key);
            const auto content = content_type(raw_content);

            p_output << key << null_unit << content;

            const size_type byte_count = (key.size()) * sizeof(typename decltype(key)::value_type) + sizeof(null_unit) + content.size() * sizeof(typename decltype(content)::value_type);
            const auto index = slice_t(current_byte_offset, byte_count);
            indices.push_back(index);
            current_byte_offset += byte_count;
        }

        p_output << indices;

        const size_type indices_byte_count = indices.size() * sizeof(typename decltype(indices)::value_type);
        const metadata_t data = metadata_t(identifier_mark, current_byte_offset + indices_byte_count + sizeof(metadata_t), slice_t(current_byte_offset, indices_byte_count));
        p_output << data;
    }

    template <typename T>
    concept packing_container_c = requires {
        requires packing_iterator_c<decltype(std::declval<T>().begin())>;
        requires packing_iterator_c<decltype(std::declval<T>().end())>;
    };

    auto pack(const packing_container_c auto &p_container, std::basic_ostream<unit_type> &p_output) -> void
    {
        pack(p_container.begin(), p_container.end(), p_output);
    }

    using attribute_type = std::pair<key_type, slice_t>;

    template <typename T>
    concept attribute_container_c = requires {
        std::declval<std::insert_iterator<T>>() = std::declval<attribute_type>();
    };

    template <attribute_container_c C>
    auto get_attributes(const bag_type &p_bag, std::insert_iterator<C> p_output) -> const unit_type *
    {
        const auto bag_unit_pointer = static_cast<const unit_type *>(p_bag.data());
        const auto bag_byte_count = p_bag.size_bytes();
        const auto bag_end_unit_pointer = bag_unit_pointer + bag_byte_count;

        // Get metadata_t.
        const metadata_t *data = reinterpret_cast<const metadata_t *>(bag_end_unit_pointer - sizeof(metadata_t));
        if (data->mark != identifier_mark)
            throw std::runtime_error("Invalid marking.");

        const auto origin = bag_end_unit_pointer - data->true_byte_count;

        // Get indices.
        const auto indices = slice_view_type(
            memory_view_iterator_t(reinterpret_cast<const slice_t *>(origin + data->index_page.byte_offset)),
            memory_view_iterator_t(reinterpret_cast<const slice_t *>(origin + data->index_page.byte_offset + data->index_page.byte_count)));

        // Get attributes.
        for (const auto &index : indices)
        {
            if ((index.byte_offset) >= data->true_byte_count)
                throw std::runtime_error("Invalid byte offset.");
            if ((index.byte_offset + index.byte_count) >= data->true_byte_count)
                throw std::runtime_error("Invalid byte count.");

            const auto key_unit_pointer = origin + index.byte_offset;
            p_output = attribute_type(key_type(key_unit_pointer), index);
        }

        return origin;
    }

    using unpack_result_type = std::pair<key_type, content_type>;

    template <typename T>
    concept unpack_result_container_c = requires {
        std::declval<std::insert_iterator<T>>() = std::declval<unpack_result_type>();
    };

    template <typename F>
    concept unpack_filter_predicate = std::predicate<F, attribute_type>;

    template <unpack_result_container_c C, unpack_filter_predicate F>
    auto unpack(const bag_type &p_bag, F p_filter_predicate, std::insert_iterator<C> p_output) -> void
    {
        std::vector<attribute_type> attributes;
        const auto origin = get_attributes(p_bag, std::inserter(attributes, attributes.end()));

        const auto bag_unit_pointer = static_cast<const unit_type *>(p_bag.data());
        const auto bag_end_unit_pointer = bag_unit_pointer + p_bag.size_bytes();
        for (const auto &attribute : attributes)
        {
            if (!p_filter_predicate(attribute))
                continue;

            const auto &[key, index] = attribute;
            const auto item_unit_pointer = origin + index.byte_offset;
            const auto key_unit_pointer = item_unit_pointer;

            if (key != key_type(key_unit_pointer))
                throw std::logic_error("Contrasting key.");

            const auto content_unit_pointer = std::find(item_unit_pointer, bag_end_unit_pointer, null_unit) + sizeof(null_unit);
            const auto content_end_unit_pointer = content_unit_pointer + index.byte_count - (key.size()) * sizeof(typename decltype(key)::value_type) - sizeof(null_unit);
            p_output = unpack_result_type(key, content_type(
                                                   memory_view_iterator_t(content_unit_pointer),
                                                   memory_view_iterator_t(content_end_unit_pointer)));
        }
    }

    template <unpack_result_container_c C>
    auto unpack_all(const bag_type &p_bag, std::insert_iterator<C> p_output) -> void
    {
        unpack(p_bag, [](const attribute_type &p_attribute)
               { return true; }, p_output);
    }

} // namespace libbag

#endif // LIBBAG_HPP