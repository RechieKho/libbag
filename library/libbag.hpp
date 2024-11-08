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
/// | key | null byte | content | ... | indices | metadata |
///
/// The metadata validate and specify the indices.
/// The indices stores the location and size of each item.
/// An item has a key and content, separated by a null byte.
/// This also means the key with the null byte forms a C string.

namespace libbag
{
    using unit_type = char;
    static_assert(sizeof(unit_type) == 1);

    using size_type = uint64_t;
    using key_type = std::basic_string_view<unit_type>;
    using unit_view_type = std::span<const unit_type>;
    using content_type = unit_view_type;
    using bag_type = unit_view_type;

    constexpr const unit_type null_unit = '\0';
    constexpr const size_type identifier_mark = 0xBABAFAFA;

    template <typename T>
    concept unique_object_representations = std::has_unique_object_representations_v<T>;

    struct slice
    {
        size_type byte_offset;
        size_type byte_count;

        constexpr slice(size_type p_byte_offset, size_type p_byte_count)
            : byte_offset(p_byte_offset), byte_count(p_byte_count) {}
    };
    static_assert(unique_object_representations<slice>);
    using slice_view_type = std::span<const slice>;

    struct metadata
    {
        size_type mark;
        slice index_page;

        constexpr metadata(size_type p_mark, slice p_index_page)
            : mark(p_mark), index_page(p_index_page) {}
    };
    static_assert(unique_object_representations<metadata>);

    template <typename T>
    class memory_view_iterator
    {
    public:
        using difference_type = int64_t;
        using value_type = T;
        using pointer = const T *;
        using reference = const T &;
        using iterator_category = std::contiguous_iterator_tag;

    private:
        pointer iterator;

    public:
        constexpr memory_view_iterator() = default;

        constexpr memory_view_iterator(pointer p_iterator)
            : iterator(p_iterator)
        {
        }

        auto operator*() const -> reference
        {
            if (iterator == nullptr)
                throw std::runtime_error("Fail to dereference a null memory view iterator");
            return *iterator;
        }

        auto operator->() const -> pointer
        {
            if (iterator == nullptr)
                throw std::runtime_error("Fail to dereference a null memory view iterator");
            return iterator;
        }

        auto operator[](difference_type p_index) const -> reference
        {
            if (iterator == nullptr)
                throw std::runtime_error("Fail to dereference a null memory view iterator");
            return iterator[p_index];
        }

        auto operator+(difference_type p_difference) const -> memory_view_iterator
        {
            if (iterator == nullptr)
                throw std::runtime_error("Fail to perform addition to a null memory view iterator");
            return memory_view_iterator(iterator + p_difference);
        }

        auto operator+=(difference_type p_difference) -> memory_view_iterator &
        {
            return *this = *this + p_difference;
        }

        auto operator-(memory_view_iterator p_memory) const -> difference_type
        {
            if (iterator == nullptr || p_memory.iterator == nullptr)
                throw std::runtime_error("Fail to compute difference with a null memory view iterator");
            return iterator - p_memory.iterator;
        }

        auto operator-(difference_type p_difference) const -> memory_view_iterator
        {
            if (iterator == nullptr)
                throw std::runtime_error("Fail to compute difference with a null memory view iterator");
            return memory_view_iterator(iterator - p_difference);
        }

        auto operator-=(difference_type p_difference) -> memory_view_iterator &
        {
            return *this = *this - p_difference;
        }

        auto operator++() -> memory_view_iterator &
        {
            if (iterator == nullptr)
                throw std::runtime_error("Fail to increment a null memory view iterator");
            ++iterator;
            return *this;
        }

        auto operator++(int) -> memory_view_iterator
        {
            memory_view_iterator temp = *this;
            ++(*this);
            return temp;
        }

        auto operator--() -> memory_view_iterator &
        {
            if (iterator == nullptr)
                throw std::runtime_error("Fail to decrement a null memory view iterator");
            --iterator;
            return *this;
        }

        auto operator--(int) -> memory_view_iterator
        {
            memory_view_iterator temp = *this;
            --(*this);
            return temp;
        }

        auto operator<=>(const memory_view_iterator &) const = default;

        friend auto operator+(typename memory_view_iterator::difference_type p_difference, const memory_view_iterator &p_memory) -> memory_view_iterator;
        friend auto operator-(typename memory_view_iterator::difference_type p_difference, const memory_view_iterator &p_memory) -> memory_view_iterator;
    };

    template <typename T>
    auto operator+(typename memory_view_iterator<T>::difference_type p_difference, const memory_view_iterator<T> &p_memory) -> memory_view_iterator<T>
    {
        return p_memory + p_difference;
    }

    template <typename T>
    auto operator-(typename memory_view_iterator<T>::difference_type p_difference, const memory_view_iterator<T> &p_memory) -> memory_view_iterator<T>
    {
        return p_memory - p_difference;
    }

    template <typename T>
    concept packable_iterator = requires {
        requires std::is_convertible_v<decltype(std::get<0>(std::declval<std::iter_value_t<T>>())), key_type>;
        requires std::is_convertible_v<decltype(std::get<1>(std::declval<std::iter_value_t<T>>())), content_type>;
    };

    template <typename T>
    concept packable = requires {
        requires packable_iterator<decltype(std::declval<T>().begin())>;
        requires packable_iterator<decltype(std::declval<T>().end())>;
    };

    template <typename = void>
    auto operator<<(std::basic_ostream<unit_type> &p_stream, const unit_view_type &p_units) -> std::basic_ostream<unit_type> &
    {
        for (const auto &e : p_units)
            p_stream << e;

        return p_stream;
    }

    template <unique_object_representations T>
    auto serialize(std::basic_ostream<unit_type> &p_stream, const T &p_object) -> std::basic_ostream<unit_type> &
    {
        const auto object_unit_pointer = reinterpret_cast<const unit_type *>(&p_object);
        const auto object_end_unit_pointer = object_unit_pointer + sizeof(p_object);

        for (auto it = object_unit_pointer; it != object_end_unit_pointer; ++it)
            p_stream << *it;

        return p_stream;
    }

    template <typename = void>
    auto operator<<(std::basic_ostream<unit_type> &p_stream, const metadata &p_metadata) -> std::basic_ostream<unit_type> &
    {
        return serialize(p_stream, p_metadata);
    }

    template <typename = void>
    auto operator<<(std::basic_ostream<unit_type> &p_stream, const slice &p_slice) -> std::basic_ostream<unit_type> &
    {
        return serialize(p_stream, p_slice);
    }

    template <typename T>
    concept slice_iterator = std::same_as<std::iter_value_t<T>, slice>;

    template <typename T>
    concept slice_container = requires {
        requires slice_iterator<decltype(std::declval<T>().begin())>;
        requires slice_iterator<decltype(std::declval<T>().end())>;
    };

    template <slice_container C>
    auto operator<<(std::basic_ostream<unit_type> &p_stream, const C &p_slices) -> std::basic_ostream<unit_type> &
    {
        for (const auto &e : p_slices)
            p_stream << e;

        return p_stream;
    }

    template <typename = void>
    auto pack(const packable auto &p_pairs, std::basic_ostream<unit_type> &p_output) -> void
    {
        std::vector<slice> indices;
        size_type current_byte_offset = 0;

        for (const auto &[raw_key, raw_content] : p_pairs)
        {
            const auto key = key_type(raw_key);
            const auto content = content_type(raw_content);

            p_output << key << null_unit << content;

            const size_type byte_count = (key.size()) * sizeof(typename decltype(key)::value_type) + sizeof(null_unit) + content.size() * sizeof(typename decltype(content)::value_type);
            const auto index = slice(current_byte_offset, byte_count);
            indices.push_back(index);
            current_byte_offset += byte_count;
        }

        p_output << indices;

        const size_type indices_byte_count = indices.size() * sizeof(typename decltype(indices)::value_type);
        const metadata data = metadata(identifier_mark, slice(current_byte_offset, indices_byte_count));
        p_output << data;
    }

    using attribute_type = std::pair<key_type, slice>;

    template <typename T>
    concept attribute_container = requires {
        std::declval<std::insert_iterator<T>>() = std::declval<attribute_type>();
    };

    template <attribute_container C>
    auto get_attributes(const bag_type &p_bag, std::insert_iterator<C> p_output) -> void
    {
        const auto bag_unit_pointer = static_cast<const unit_type *>(p_bag.data());
        const auto bag_byte_count = p_bag.size_bytes();

        // Get metadata.
        const metadata *data = reinterpret_cast<const metadata *>(bag_unit_pointer + bag_byte_count - sizeof(metadata));
        if (data->mark != identifier_mark)
            throw std::runtime_error("Invalid marking.");

        static_assert(std::is_lvalue_reference_v<std::iter_reference_t<memory_view_iterator<slice>>>);

        // Get indices.
        const auto indices = slice_view_type(
            memory_view_iterator(reinterpret_cast<const slice *>(bag_unit_pointer + data->index_page.byte_offset)),
            memory_view_iterator(reinterpret_cast<const slice *>(bag_unit_pointer + data->index_page.byte_offset + data->index_page.byte_count)));

        // Get attributes.
        for (const auto &index : indices)
        {
            if (index.byte_offset >= bag_byte_count)
                throw std::runtime_error("Invalid byte offset.");
            if ((index.byte_offset + index.byte_count) >= bag_byte_count)
                throw std::runtime_error("Invalid byte count.");

            const auto key_unit_pointer = bag_unit_pointer + index.byte_offset;
            p_output = attribute_type(key_type(key_unit_pointer), index);
        }
    }

    using item_type = std::pair<key_type, content_type>;

    template <typename T>
    concept item_container = requires {
        std::declval<std::insert_iterator<T>>() = std::declval<item_type>();
    };

    template <typename F>
    concept unpack_filter_predicate = std::predicate<F, attribute_type>;

    template <item_container C, unpack_filter_predicate F>
    auto unpack(const bag_type &p_bag, F p_filter_predicate, std::insert_iterator<C> p_output) -> void
    {
        std::vector<attribute_type> attributes;
        get_attributes(p_bag, std::inserter(attributes, attributes.end()));

        const auto bag_unit_pointer = static_cast<const unit_type *>(p_bag.data());
        const auto bag_end_unit_pointer = bag_unit_pointer + p_bag.size_bytes();
        for (const auto &attribute : attributes)
        {
            if (!p_filter_predicate(attribute))
                continue;

            const auto &[key, index] = attribute;
            const auto item_unit_pointer = bag_unit_pointer + index.byte_offset;
            const auto key_unit_pointer = item_unit_pointer;

            if (key != key_type(key_unit_pointer))
                throw std::logic_error("Contrasting key.");

            const auto content_unit_pointer = std::find(item_unit_pointer, bag_end_unit_pointer, null_unit) + sizeof(null_unit);
            const auto content_end_unit_pointer = content_unit_pointer + index.byte_count - (key.size()) * sizeof(typename decltype(key)::value_type) - sizeof(null_unit);
            p_output = item_type(key, content_type(
                                          memory_view_iterator(content_unit_pointer),
                                          memory_view_iterator(content_end_unit_pointer)));
        }
    }

    template <item_container C>
    auto unpack_all(const bag_type &p_bag, std::insert_iterator<C> p_output) -> void
    {
        unpack(p_bag, [](const attribute_type &p_attribute)
               { return true; }, p_output);
    }

} // namespace libbag

#endif // LIBBAG_HPP