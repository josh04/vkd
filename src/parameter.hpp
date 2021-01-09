#pragma once 

#include "cereal/cereal.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/set.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/archives/binary.hpp"
    
#include "glm/glm.hpp"
namespace glm {
template<class Archive>
void serialize(Archive & archive, glm::vec2& vec) {
    archive( vec.x, vec.y );
}
template<class Archive>
void serialize(Archive & archive, glm::vec4& vec) {
    archive( vec.x, vec.y, vec.z, vec.w );
}
template<class Archive>
void serialize(Archive & archive, glm::ivec2& vec) {
    archive( vec.x, vec.y );
}
template<class Archive>
void serialize(Archive & archive, glm::ivec4& vec) {
    archive( vec.x, vec.y, vec.z, vec.w );
}
template<class Archive>
void serialize(Archive & archive, glm::uvec2& vec) {
    archive( vec.x, vec.y );
}
template<class Archive>
void serialize(Archive & archive, glm::uvec4& vec) {
    archive( vec.x, vec.y, vec.z, vec.w );
}
}

namespace vulkan {
    enum class ParameterType {
        p_float,
        p_int,
        p_uint,
        p_vec2,
        p_vec4,
        p_ivec2,
        p_ivec4,
        p_uvec2,
        p_uvec4,
        p_string
    };
    
    template<typename P>
    class Parameter;

    class ParameterInterface {
    public:
        virtual ~ParameterInterface() {}

        template<typename R>
        Parameter<R>& as() { return *dynamic_cast<Parameter<R> *>(this); }

        virtual ParameterType type() const = 0;
        virtual void * data() = 0;
        virtual size_t size() = 0;
        virtual size_t offset() = 0;
        virtual std::string name() const = 0;

        bool changed() { auto ch = _changed; _changed = false; return ch; }

        virtual const std::set<std::string>& tags() const = 0;
        virtual void tag(const std::string& t) = 0;

        template <class Archive>
        void serialize( Archive & ar )
        {
            ar(_changed);
        }
    protected:
        bool _changed = true;
    };

    template<typename P>
    class Parameter : public ParameterInterface {
    public:
        using Type = ParameterType;
        Parameter() {
            if constexpr(std::is_same<P, float>::value) {
                _type = Type::p_float;
                set(0.0f);
                min(-1.0f);
                max(1.0f);
            } else if constexpr(std::is_same<P, int>::value) {
                _type = Type::p_int;
                set(0);
                min(-127);
                max(128);
            } else if constexpr(std::is_same<P, unsigned int>::value) {
                _type = Type::p_uint;
                set(0);
                min(0);
                max(255);
            } else if constexpr(std::is_same<P, glm::vec2>::value) {
                _type = Type::p_vec2;
                set({0.0f, 0.0f});
                min({0.0f, 0.0f});
                max({1.0f, 1.0f});
            } else if constexpr(std::is_same<P, glm::vec4>::value) {
                _type = Type::p_vec4;
                set({0.0f, 0.0f, 0.0f, 0.0f});
                min({0.0f, 0.0f, 0.0f, 0.0f});
                max({1.0f, 1.0f, 1.0f, 1.0f});
            } else if constexpr(std::is_same<P, glm::ivec2>::value) {
                _type = Type::p_ivec2;
                set({0, 0});
                min({0, 0});
                max({255, 255});
            } else if constexpr(std::is_same<P, glm::ivec4>::value) {
                _type = Type::p_ivec4;
                set({0,0,0,0});
                min({0, 0, 0, 0});
                max({255, 255, 255, 255});
            } else if constexpr(std::is_same<P, glm::uvec2>::value) {
                _type = Type::p_uvec2;
                set({0, 0});
                min({0, 0});
                max({255, 255});
            } else if constexpr(std::is_same<P, glm::uvec4>::value) {
                _type = Type::p_uvec4;
                set({0,0,0,0});
                min({0, 0, 0, 0});
                max({255, 255, 255, 255});
            }  else if constexpr(std::is_same<P, std::string>::value) {
                _type = Type::p_string;
                set("placeholder");
            } 
        }
        ~Parameter() = default;
        Parameter(Parameter&&) = delete;
        Parameter(const Parameter&) = delete;

        template<typename Q>
        static constexpr bool is_numeric() {
            if constexpr(std::is_same<Q, std::string>::value) {
                return false;
            } 
            return true;
        }

        void set(P p) {
            if constexpr (is_numeric<P>()) {
                p = glm::min(glm::max(p, min()), max());
            }
            _value = p;
            _data.resize(sizeof(P));
            memcpy(_data.data(), &p, sizeof(P));
            _changed = true;
        }

        P get() const { return _value; }

        P min() const { return _min; }
        P max() const { return _max; }

        void min(P m) { _min = m; }
        void max(P m) { _max = m; }

        Type type() const override { return _type; }
        void * data() override { return _data.data(); }
        size_t size() override { return _data.size(); }

        void offset(size_t off) { _offset = off; }
        size_t offset() override { return _offset; }

        void name(std::string name) { _name = name; }
        std::string name() const override { return _name; }

        const std::set<std::string>& tags() const override { return _tags; }
        void tag(const std::string& t) override { _tags.insert(t); } 
        
        template <class Archive>
        void serialize( Archive & ar )
        {
            ar(cereal::base_class<ParameterInterface>(this), _type, _value, _min, _max, _data, _offset, _name, _tags);
        }
    private:
        Type _type;
        P _value;
        P _min;
        P _max;
        std::vector<uint8_t> _data;
        size_t _offset;
        std::string _name;
        std::set<std::string> _tags;
    };

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

    template<ParameterType T> 
    std::shared_ptr<ParameterInterface> make_param(const std::string& name, size_t offset) { 
        auto param = std::make_shared<Parameter<typename _ptype<T>::type>>(); 
        param->name(name);
        param->offset(offset);
        return param;
    }

}

CEREAL_REGISTER_TYPE(vulkan::ParameterInterface)
CEREAL_REGISTER_TYPE(vulkan::Parameter<float>)
CEREAL_REGISTER_TYPE(vulkan::Parameter<int>)
CEREAL_REGISTER_TYPE(vulkan::Parameter<unsigned int>)
CEREAL_REGISTER_TYPE(vulkan::Parameter<glm::vec2>)
CEREAL_REGISTER_TYPE(vulkan::Parameter<glm::vec4>)
CEREAL_REGISTER_TYPE(vulkan::Parameter<glm::ivec2>)
CEREAL_REGISTER_TYPE(vulkan::Parameter<glm::ivec4>)
CEREAL_REGISTER_TYPE(vulkan::Parameter<glm::uvec2>)
CEREAL_REGISTER_TYPE(vulkan::Parameter<glm::uvec4>)
CEREAL_REGISTER_TYPE(vulkan::Parameter<std::string>)/*
CEREAL_REGISTER_POLYMORPHIC_RELATION(vulkan::ParameterInterface, vulkan::Parameter<float>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vulkan::ParameterInterface, vulkan::Parameter<int>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vulkan::ParameterInterface, vulkan::Parameter<unsigned int>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vulkan::ParameterInterface, vulkan::Parameter<glm::vec2>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vulkan::ParameterInterface, vulkan::Parameter<glm::vec4>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vulkan::ParameterInterface, vulkan::Parameter<glm::ivec2>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vulkan::ParameterInterface, vulkan::Parameter<glm::ivec4>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vulkan::ParameterInterface, vulkan::Parameter<glm::uvec2>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vulkan::ParameterInterface, vulkan::Parameter<glm::uvec4>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vulkan::ParameterInterface, vulkan::Parameter<std::string>)
*/