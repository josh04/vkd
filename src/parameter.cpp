#include "parameter.hpp"
#include "cereal/cereal.hpp"

CEREAL_CLASS_VERSION(glm::vec2, 0);
CEREAL_CLASS_VERSION(glm::vec4, 0);
CEREAL_CLASS_VERSION(glm::ivec2, 0);
CEREAL_CLASS_VERSION(glm::ivec4, 0);
CEREAL_CLASS_VERSION(glm::uvec2, 0);
CEREAL_CLASS_VERSION(glm::uvec4, 0);
CEREAL_CLASS_VERSION(vkd::Frame, 0);
CEREAL_CLASS_VERSION(vkd::ParameterInterface, 2);

#define PARAM_MACRO(NAME, VERSION) CEREAL_REGISTER_TYPE(NAME) \
CEREAL_CLASS_VERSION(NAME, VERSION)

#define PARAMETER_VERSION 2
PARAM_MACRO(vkd::Parameter<float>, PARAMETER_VERSION);
PARAM_MACRO(vkd::Parameter<int>, PARAMETER_VERSION);
PARAM_MACRO(vkd::Parameter<unsigned int>, PARAMETER_VERSION);
PARAM_MACRO(vkd::Parameter<glm::vec2>, PARAMETER_VERSION);
PARAM_MACRO(vkd::Parameter<glm::vec4>, PARAMETER_VERSION);
PARAM_MACRO(vkd::Parameter<glm::ivec2>, PARAMETER_VERSION);
PARAM_MACRO(vkd::Parameter<glm::ivec4>, PARAMETER_VERSION);
PARAM_MACRO(vkd::Parameter<glm::uvec2>, PARAMETER_VERSION);
PARAM_MACRO(vkd::Parameter<glm::uvec4>, PARAMETER_VERSION);
PARAM_MACRO(vkd::Parameter<std::string>, PARAMETER_VERSION);
PARAM_MACRO(vkd::Parameter<vkd::Frame>, PARAMETER_VERSION);
PARAM_MACRO(vkd::Parameter<bool>, PARAMETER_VERSION);

namespace vkd {
    std::unique_ptr<ParameterCache> ParameterCache::_singleton = nullptr;
    std::mutex ParameterCache::_param_mutex;
    
    void ParameterCache::_make() {
        if (_singleton == nullptr) {
            _singleton = std::unique_ptr<ParameterCache>(new ParameterCache{});
        }
    }

    void ParameterCache::add(ParameterType p, const std::string& name, const std::shared_ptr<ParameterInterface>& param) {
        std::scoped_lock lock(_param_mutex);
        _make();
        _singleton->_add(make_hash(p, name), param);
    }

    bool ParameterCache::has(ParameterType p, const std::string& name) {
        std::scoped_lock lock(_param_mutex);
        _make();
        return _singleton->_has(make_hash(p, name));
    }

    std::shared_ptr<ParameterInterface> ParameterCache::get(ParameterType p, const std::string& name) {
        std::scoped_lock lock(_param_mutex);
        _make();
        return _singleton->_get(make_hash(p, name));
    }

    bool ParameterCache::remove(ParameterType p, const std::string& name) {
        std::scoped_lock lock(_param_mutex);
        _make();
        return _singleton->_remove(make_hash(p, name));
    }

    std::string ParameterCache::make_hash(ParameterType p, const std::string& name) {
        return std::to_string((int32_t)p) + name;
    }

    void ParameterCache::_add(const std::string& hash, const std::shared_ptr<ParameterInterface>& param) {
        _params.emplace(hash, param);
    }

    bool ParameterCache::_has(const std::string& hash) {
        return (_params.find(hash) != _params.end());
    }

    std::shared_ptr<ParameterInterface> ParameterCache::_get(const std::string& hash) {
        return _params.at(hash);
    }

    bool ParameterCache::_remove(const std::string& hash) {
        return _params.erase(hash) > 0;
    }
}