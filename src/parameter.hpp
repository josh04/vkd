#pragma once 

#include "cereal/cereal.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/set.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/atomic.hpp"
#include "cereal/archives/binary.hpp"

#include <mutex>

#include "glm/glm.hpp"

namespace glm {
    template<class Archive>
    void serialize(Archive & archive, glm::vec2& vec, const uint32_t version) {
        if (version == 0) {
            archive( vec.x, vec.y );
        }
    }
    template<class Archive>
    void serialize(Archive & archive, glm::vec4& vec, const uint32_t version) {
        if (version == 0) {
        archive( vec.x, vec.y, vec.z, vec.w );
        }
    }
    template<class Archive>
    void serialize(Archive & archive, glm::ivec2& vec, const uint32_t version) {
        if (version == 0) {
        archive( vec.x, vec.y );
        }
    }
    template<class Archive>
    void serialize(Archive & archive, glm::ivec4& vec, const uint32_t version) {
        if (version == 0) {
        archive( vec.x, vec.y, vec.z, vec.w );
        }
    }
    template<class Archive>
    void serialize(Archive & archive, glm::uvec2& vec, const uint32_t version) {
        if (version == 0) {
            archive( vec.x, vec.y );
        }
    }
    template<class Archive>
    void serialize(Archive & archive, glm::uvec4& vec, const uint32_t version) {
        if (version == 0) {
            archive( vec.x, vec.y, vec.z, vec.w );
        }
    }
}

namespace vkd {
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
        p_string,
        p_frame
    };

    struct Frame {
        int64_t index;
    };

    static bool operator<(const Frame& lhs, const Frame& rhs) {
        return lhs.index < rhs.index;
    }
    static bool operator==(const Frame& lhs, const Frame& rhs) {
        return lhs.index == rhs.index;
    }
    
    template<class Archive>
    void serialize(Archive & archive, vkd::Frame& vec, const uint32_t version) {
        if (version == 0) {
            archive( vec.index );
        }
    }

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

        virtual void set_from(const std::shared_ptr<ParameterInterface>& rhs) = 0;

        void set_changed() { _changed = true; }
        bool changed() { auto ch = _changed; _changed = false; return ch; }

        virtual const std::set<std::string>& tags() const = 0;
        virtual void tag(const std::string& t) = 0;

        template <class Archive>
        void serialize(Archive& ar, const uint32_t version)
        {
            if (version == 0) {
                ar(_changed);
            }
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
            } else if constexpr(std::is_same<P, std::string>::value) {
                _type = Type::p_string;
                set("placeholder");
            } else if constexpr(std::is_same<P, Frame>::value) {
                _type = Type::p_frame;
                set(Frame{0});
            } 
        }
        ~Parameter() = default;
        Parameter(Parameter&&) = delete;
        Parameter(const Parameter&) = delete;

        template<typename Q>
        static constexpr bool is_numeric() {
            if constexpr(std::is_same<Q, std::string>::value) {
                return false;
            } else if constexpr (std::is_same<Q, Frame>::value) {
                return false;
            }
            return true;
        }

        void set_force(P p) {
            if constexpr (is_numeric<P>()) {
                min(glm::min(p, min()));
                max(glm::max(p, max()));
            }
            _value = p;
            _data.resize(sizeof(P));
            memcpy(_data.data(), &p, sizeof(P));
            _changed = true;
        }

        void set(P p) {
            if constexpr (is_numeric<P>()) {
                p = glm::min(glm::max(p, min()), max());
            }
            set_force(p);
        }

        void set_default(P p) {
            if constexpr (std::is_same<P, Frame>::value) {
                static_assert("Cannot set default on frame parameter!");
            }
            if (!_set_default) {
                set(p);
                _set_default = true;
            }
        }

        void set_from(const Parameter<P>& rhs) {
            set(rhs.get());
            min(rhs.min());
            max(rhs.max());
        }

        void set_from(const std::shared_ptr<ParameterInterface>& rhs) override { set_from(rhs->as<P>()); }

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
        void serialize(Archive& ar, const uint32_t version)
        {
            if (version == 0) {
                ar(cereal::base_class<ParameterInterface>(this), _type, _value, _min, _max, _data, _offset, _name, _tags, _set_default);
            }
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

        std::atomic_bool _set_default = false;
    };

    class ParameterCache {
    public:
        ~ParameterCache() = default;

        static void add(ParameterType p, const std::string& name, const std::shared_ptr<ParameterInterface>& param);
        static bool has(ParameterType p, const std::string& name);
        static std::shared_ptr<ParameterInterface> get(ParameterType p, const std::string& name);
        static bool remove(ParameterType p, const std::string& name);

        static std::string make_hash(ParameterType p, const std::string& name);
    private:
        static void _make();
        void _add(const std::string& hash, const std::shared_ptr<ParameterInterface>& param);
        bool _has(const std::string& hash);
        std::shared_ptr<ParameterInterface> _get(const std::string& hash);
        bool _remove(const std::string& hash);
    
        ParameterCache() = default;

        static std::unique_ptr<ParameterCache> _singleton;
        static std::mutex _param_mutex;
        std::map<std::string, std::shared_ptr<ParameterInterface>> _params;
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
    template<> struct _ptype<ParameterType::p_frame> { using type = Frame; };

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

    template<ParameterType T> 
    std::shared_ptr<ParameterInterface> make_param(const std::string& hash, const std::string& name, size_t offset) { 
        if (ParameterCache::has(T, hash + name)) {
            auto param = ParameterCache::get(T, hash + name);
            param->set_changed();
            return param;
        }
        auto param = std::make_shared<Parameter<typename _ptype<T>::type>>(); 
        param->name(name);
        param->offset(offset);
        ParameterCache::add(T, hash + name, param);
        return param;
    }
    template<typename T> 
    std::shared_ptr<ParameterInterface> make_param(const std::string& hash, const std::string& name, size_t offset) { 
        return make_param<_qtype<T>::type>(hash, name, offset);
    }
}
/*
CEREAL_REGISTER_TYPE(vkd::ParameterInterface)
CEREAL_REGISTER_TYPE(vkd::Parameter<float>)
CEREAL_REGISTER_TYPE(vkd::Parameter<int>)
CEREAL_REGISTER_TYPE(vkd::Parameter<unsigned int>)
CEREAL_REGISTER_TYPE(vkd::Parameter<glm::vec2>)
CEREAL_REGISTER_TYPE(vkd::Parameter<glm::vec4>)
CEREAL_REGISTER_TYPE(vkd::Parameter<glm::ivec2>)
CEREAL_REGISTER_TYPE(vkd::Parameter<glm::ivec4>)
CEREAL_REGISTER_TYPE(vkd::Parameter<glm::uvec2>)
CEREAL_REGISTER_TYPE(vkd::Parameter<glm::uvec4>)
CEREAL_REGISTER_TYPE(vkd::Parameter<std::string>)
CEREAL_REGISTER_TYPE(vkd::Parameter<vkd::Frame>)
*/
/*
CEREAL_REGISTER_POLYMORPHIC_RELATION(vkd::ParameterInterface, vkd::Parameter<float>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vkd::ParameterInterface, vkd::Parameter<int>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vkd::ParameterInterface, vkd::Parameter<unsigned int>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vkd::ParameterInterface, vkd::Parameter<glm::vec2>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vkd::ParameterInterface, vkd::Parameter<glm::vec4>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vkd::ParameterInterface, vkd::Parameter<glm::ivec2>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vkd::ParameterInterface, vkd::Parameter<glm::ivec4>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vkd::ParameterInterface, vkd::Parameter<glm::uvec2>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vkd::ParameterInterface, vkd::Parameter<glm::uvec4>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(vkd::ParameterInterface, vkd::Parameter<std::string>)
*/