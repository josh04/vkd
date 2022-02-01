#pragma once

#include "parameter.hpp"
#include "engine_node.hpp"

namespace vkd {
    template<ParameterType T> struct _ptype;
    template<> struct _ptype<ParameterType::p_float> { using type = float; };
    template<> struct _ptype<ParameterType::p_int> { using type = int; };
    template<> struct _ptype<ParameterType::p_uint> { using type = unsigned int; };
    template<> struct _ptype<ParameterType::p_vec2> { using type = glm::vec2; };
    template<> struct _ptype<ParameterType::p_vec4> { using type = glm::vec4; };
    template<> struct _ptype<ParameterType::p_ivec2> { using type = glm::ivec2; };
    template<> struct _ptype<ParameterType::p_ivec4> { using type = glm::ivec4; };
    template<> struct _ptype<ParameterType::p_uvec2> { using type = glm::uvec2; };
    template<> struct _ptype<ParameterType::p_uvec4> { using type = glm::uvec4; };
    template<> struct _ptype<ParameterType::p_string> { using type = std::string; };
    template<> struct _ptype<ParameterType::p_frame> { using type = Frame; };
    template<> struct _ptype<ParameterType::p_bool> { using type = bool; };

    template<typename T> struct _qtype;
    template<> struct _qtype<float> { static constexpr ParameterType type = ParameterType::p_float; };
    template<> struct _qtype<int> { static constexpr ParameterType type = ParameterType::p_int; };
    template<> struct _qtype<unsigned int> { static constexpr ParameterType type = ParameterType::p_uint; };
    template<> struct _qtype<glm::vec2> { static constexpr ParameterType type = ParameterType::p_vec2; };
    template<> struct _qtype<glm::vec4> { static constexpr ParameterType type = ParameterType::p_vec4; };
    template<> struct _qtype<glm::ivec2> { static constexpr ParameterType type = ParameterType::p_ivec2; };
    template<> struct _qtype<glm::ivec4> { static constexpr ParameterType type = ParameterType::p_ivec4; };
    template<> struct _qtype<glm::uvec2> { static constexpr ParameterType type = ParameterType::p_uvec2; };
    template<> struct _qtype<glm::uvec4> { static constexpr ParameterType type = ParameterType::p_uvec4; };
    template<> struct _qtype<std::string> { static constexpr ParameterType type = ParameterType::p_string; };
    template<> struct _qtype<Frame> { static constexpr ParameterType type = ParameterType::p_frame; };
    template<> struct _qtype<bool> { static constexpr ParameterType type = ParameterType::p_bool; };

    template<ParameterType T> 
    std::shared_ptr<ParameterInterface> make_param(const std::string& hash, const std::string& name, size_t offset, std::set<std::string> tags = {}) { 
        if (ParameterCache::has(T, hash + name)) {
            auto param = ParameterCache::get(T, hash + name);
            param->set_changed();
            param->tags(tags);
            return param;
        }
        auto param = std::make_shared<Parameter<typename _ptype<T>::type>>(); 
        param->name(name);
        param->offset(offset);
        param->tags(tags);
        ParameterCache::add(T, hash + name, param);
        return param;
    }

    template<typename T> 
    std::shared_ptr<ParameterInterface> make_param(const std::string& hash, const std::string& name, size_t offset, std::set<std::string> tags = {}) { 
        return make_param<_qtype<T>::type>(hash, name, offset, tags);
    }
    template<ParameterType T> 
    std::shared_ptr<ParameterInterface> make_param(EngineNode& node, const std::string& name, size_t offset, std::set<std::string> tags = {}) { 
        auto param = make_param<T>(node.param_hash_name(), name, offset, tags);
        node.register_non_kernel_param(param);
        return param;
    }

    template<typename T> 
    std::shared_ptr<ParameterInterface> make_param(EngineNode& node, const std::string& name, size_t offset, std::set<std::string> tags = {}) { 
        return make_param<_qtype<T>::type>(node, name, offset, tags);
    }
}