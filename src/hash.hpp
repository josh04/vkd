#pragma once

#include <cstdint>

#include <functional>

namespace vkd {
    class Hash;
    static bool operator<(const Hash& lhs, const Hash& rhs);
    class Hash {
    public:
        Hash() = default;

        template <typename... Args>
        explicit Hash(Args... args) {
            _hash = hash_combine(_hash, args...);
        }

        ~Hash() = default;

        Hash(Hash&& rhs) : _hash(rhs._hash) {}
        Hash(const Hash& rhs) : _hash(rhs._hash) {}

        Hash& operator=(const Hash& rhs) {
            _hash = rhs._hash;
            return *this;
        }

        bool operator==(const Hash& rhs) {
            return _hash == rhs._hash;
        }

        template <typename... Args>
        uint64_t operator()(Args... args) {
            _hash = hash_combine(s_initial_hash, args...);
            return _hash;
        }

        template <typename Arg>
        Hash& operator+(const Arg& arg) {
            _hash = hash_combine(_hash, arg);
            return *this;
        }

    private:
        template <typename T, typename... Rest>
        uint64_t hash_combine(uint64_t& seed, const T& v, Rest... rest)
        {
            std::hash<T> hasher;
            seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            (hash_combine(seed, rest), ...);
            return seed;
        }

        uint64_t _hash = 0;
        static constexpr uint64_t s_initial_hash = 0;
        friend bool operator<(const Hash& lhs, const Hash& rhs);
    };

    static bool operator<(const Hash& lhs, const Hash& rhs) {
        return lhs._hash < rhs._hash;
    }

}