#pragma once 

#include "cereal/cereal.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/set.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/atomic.hpp"
#include "cereal/archives/binary.hpp"

#include <memory>
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
        p_frame,
        p_bool
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
    static bool operator!=(const Frame& lhs, const Frame& rhs) {
        return !(lhs.index == rhs.index);
    }
    
    template<class Archive>
    void serialize(Archive & archive, vkd::Frame& vec, const uint32_t version) {
        if (version == 0) {
            archive( vec.index );
        }
    }

    template<typename P>
    class Parameter;

    class ParameterInterface;
    using ParamPtr = std::shared_ptr<ParameterInterface>;

    class ParameterInterface : public std::enable_shared_from_this<ParameterInterface> {
    public:
        virtual ~ParameterInterface() {}

        template<typename R>
        const Parameter<R>& as() const { return *dynamic_cast<const Parameter<R> *>(this); }
        template<typename R>
        Parameter<R>& as() { return *dynamic_cast<Parameter<R> *>(this); }
        template<typename R>
        std::shared_ptr<Parameter<R>> as_ptr() { return std::dynamic_pointer_cast<Parameter<R>>(shared_from_this()); }

        virtual ParameterType type() const = 0;
        virtual void * data() = 0;
        virtual size_t size() = 0;
        virtual size_t offset() = 0;
        virtual std::string name() const = 0;

        int order() const { return _order; }
        void order(int i) { _order = i; };

        virtual void set_from(const std::shared_ptr<ParameterInterface>& rhs) = 0;

        void set_changed() { _version_index = _execution_index + 1; }
        bool changed() const { return _version_index > _execution_index; }
        bool changed_last() const { return _version_index == _execution_index; }
        void reset_changed() { _execution_index++; }

        virtual const std::set<std::string>& tags() const = 0;
        virtual void tags(const std::set<std::string>& t) = 0;
        virtual void tag(const std::string& t) = 0;

        virtual void enum_names(std::vector<std::string> names) = 0;
        virtual const std::vector<std::string>& enum_names() const = 0;

        virtual void enum_values(std::vector<int> values) = 0;
        virtual const std::vector<int32_t>& enum_values() const = 0;
        
        auto&& ui_changed_last_tick() { return _ui_changed_last_tick; }

        template <class Archive>
        void serialize(Archive& ar, const uint32_t version)
        {
            if (version >= 0) {
                bool changed = false;
                ar(changed);
            }
            if (version >= 1) {
                ar(_order);
            }
            if (version >= 2) {
                ar(_version_index, _execution_index);
            }
        }
    protected:
        //bool _changed = true;
        int64_t _version_index = 0;
        int64_t _execution_index = 0;

        bool _ui_changed_last_tick = false;

        int _order = 0;
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
            } else if constexpr(std::is_same<P, bool>::value) {
                _type = Type::p_bool;
                min(false);
                max(true);
                set(false);
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
            _data.resize(sizeof(P)); // so that _data is always the correct size
            if (_value != p) {
                _value = p;
                memcpy(_data.data(), &p, sizeof(P));
                set_changed();
            }
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
                set_force(p);
                _default = p;
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
        P get_default() const { return _default; }

        P min() const { return _min; }
        P max() const { return _max; }

        void min(P m) { _min = m; }
        void max(P m) { _max = m; }

        void soft_min(P m) { _min = glm::min(m, _min); }
        void soft_max(P m) { _max = glm::max(m, _max); }

        Type type() const override { return _type; }
        void * data() override { return _data.data(); }
        size_t size() override { return _data.size(); }

        void offset(size_t off) { _offset = off; }
        size_t offset() override { return _offset; }

        void name(std::string name) { _name = name; }
        std::string name() const override { return _name; }

        const std::set<std::string>& tags() const override { return _tags; }
        void tags(const std::set<std::string>& t) override { _tags.insert(t.begin(), t.end()); } 
        void tag(const std::string& t) override { _tags.insert(t); } 

        void enum_names(std::vector<std::string> names) override { if constexpr (std::is_same<P, int>::value || std::is_same<P, unsigned int>::value) { _enum_names = std::move(names); } }
        const std::vector<std::string>& enum_names() const override { return _enum_names; }

        void enum_values(std::vector<int32_t> values) override { if constexpr (std::is_same<P, int>::value || std::is_same<P, unsigned int>::value) { _enum_values = std::move(values); } }
        const std::vector<int32_t>& enum_values() const override { return _enum_values; }
        
        template <class Archive>
        void serialize(Archive& ar, const uint32_t version)
        {
            if (version >= 0) {
                ar(cereal::base_class<ParameterInterface>(this), _type, _value, _min, _max, _data, _offset, _name, _tags, _set_default);
            }
            if (version >= 1) {
                ar(_enum_names);
            }
            if (version >= 2) {
                ar(_enum_values);
            }
        }
    private:
        Type _type;
        P _value;
        P _min;
        P _max;
        P _default;
        std::vector<uint8_t> _data;
        size_t _offset;
        std::string _name;
        std::set<std::string> _tags;

        std::vector<std::string> _enum_names;
        std::vector<int32_t> _enum_values;

        std::atomic_bool _set_default = false;
    };

    class ParameterCache {
    public:
        ~ParameterCache() = default;

        static void add(ParameterType p, const std::string& name, const std::shared_ptr<ParameterInterface>& param);
        static bool has(ParameterType p, const std::string& name);
        static std::shared_ptr<ParameterInterface> get(ParameterType p, const std::string& name);
        static bool remove(ParameterType p, const std::string& name);

        static void reset_changed() { _singleton->_reset_changed(); }

        static std::string make_hash(ParameterType p, const std::string& name);
    private:
        static void _make();
        void _add(const std::string& hash, const std::shared_ptr<ParameterInterface>& param);
        bool _has(const std::string& hash);
        std::shared_ptr<ParameterInterface> _get(const std::string& hash);
        bool _remove(const std::string& hash);

        void _reset_changed() { for (auto&& pair : _params) { pair.second->reset_changed(); } }
    
        ParameterCache() = default;

        static std::unique_ptr<ParameterCache> _singleton;
        static std::mutex _param_mutex;
        std::map<std::string, std::shared_ptr<ParameterInterface>> _params;
    };
}