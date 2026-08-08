#pragma once

#include <cassert>
#include <cstdint>

/*
Square indexes range from 0 to 63 (a1 to h8)
255 represents the invalid square state
*/

class Square {
public:
    static constexpr std::uint8_t invalidIndex = 255;

    constexpr Square() noexcept = default;

    constexpr explicit Square(std::uint8_t index) : index_(index) {
        assert((index <= 63) || (index == invalidIndex));
    }

    constexpr explicit Square(char file, char rank) noexcept 
        : index_((rank - '1') * 8 + (file - 'a')) {
        assert(('a' <= file) && (file <= 'h'));
        assert(('1' <= rank) && (rank <= '8'));
    }

    bool is_valid() const noexcept {return index_ <= 63;}

    std::uint8_t index() const noexcept { assert(is_valid()); return index_; }
    std::uint8_t file() const noexcept { assert(is_valid()); return index_ % 8; }
    std::uint8_t rank() const noexcept { assert(is_valid()); return index_ / 8; }
    char uci_file() const noexcept { assert(is_valid()); return (index_ % 8) + 'a'; }
    char uci_rank() const noexcept { assert(is_valid()); return (index_ / 8) + '1'; }

    bool operator==(const Square& other) const = default;

    class Iterator {
    public:
        explicit Iterator(std::uint8_t index) : current_(index) {}

        Square operator*() const { return Square(current_); }
        
        Iterator& operator++() { 
            current_++; 
            return *this; 
        }
        
        bool operator!=(const Iterator& other) const { 
            return current_ != other.current_; 
        }

    private:
        std::uint8_t current_;
    };

    struct Range {
        std::uint8_t start_index;
        std::uint8_t end_index;

        Iterator begin() const { return Iterator(start_index); }
        Iterator end() const { return Iterator(end_index + 1); }
    };

    static constexpr Range all() noexcept { 
        return Range{0, 63}; 
    }

private:
    std::uint8_t index_ = invalidIndex;
};

static_assert(sizeof(Square) == 1);
