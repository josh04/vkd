#include "host_cache.hpp"
#include "image.hpp"


namespace vkd {
    std::unique_ptr<StaticHostImage> StaticHostImage::make(int32_t width, int32_t height, int32_t channels, int32_t element_size) {
        auto ptr = std::make_unique<StaticHostImage>();
        ptr->create_image(width, height, channels, element_size);
        return std::move(ptr);
    }

    void StaticHostImage::create_image(int32_t width, int32_t height, int32_t channels, int32_t element_size) {
        _channels = channels;
        _element_size = element_size;
        _data.resize(width * height * channels * element_size);
        _dim = {width, height};
    }

    bool HostCache::add(const Hash& name, std::unique_ptr<StaticHostImage> image) {
        bool was_new = true;
        if (_cache.find(name) != _cache.end()) {
            was_new = false;
        }
        _cache.erase(name);
        _cache.emplace(name, std::move(image));
        return was_new;
    }

    bool HostCache::remove(const Hash& name) {
        if (_cache.find(name) == _cache.end()) {
            return false;
        }
        _cache.erase(name);
        return true;
    }

    StaticHostImage * HostCache::get(const Hash& name) {
        auto search = _cache.find(name);
        if (search != _cache.end()) {
            _least_recent_used.erase(std::remove(std::begin(_least_recent_used), std::end(_least_recent_used), name), std::end(_least_recent_used));
            _least_recent_used.push_back(name);
            return search->second.get();
        }

        return nullptr;
    }

    void HostCache::trim() {

    }
}