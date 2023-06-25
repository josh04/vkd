#pragma once
        
#include <memory>
#include <string>
#include <map>
#include <deque>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "hash.hpp"

namespace vkd {

    class StaticHostImage {
    public:
        StaticHostImage() = default;
        ~StaticHostImage() = default;
        StaticHostImage(StaticHostImage&&) = delete;
        StaticHostImage(const StaticHostImage&) = delete;

        static std::unique_ptr<StaticHostImage> make(int32_t width, int32_t height, int32_t channels, int32_t element_size);

        void create_image(int32_t width, int32_t height, int32_t channels, int32_t element_size);

        auto size() const { return _data.size(); }
        auto data() { return _data.data(); }
        auto dim() const { return _dim; }
        auto channels() const { return _channels; }
        auto element_size() const { return _element_size; }

    private:
        std::vector<uint8_t> _data;
        glm::ivec2 _dim = {0, 0};
        int32_t _channels = 0;
        int32_t _element_size = 0; 
    };

    class HostCache {
    public:
        HostCache() = default;
        ~HostCache() = default;
        HostCache(HostCache&&) = delete;
        HostCache(const HostCache&) = delete;

        bool add(const Hash& name, std::unique_ptr<StaticHostImage> image);
        bool remove(const Hash& name);
        StaticHostImage * get(const Hash& name);
        void trim();

    private:
        std::map<Hash, std::unique_ptr<StaticHostImage>> _cache;
        std::deque<Hash> _least_recent_used;
    };
}