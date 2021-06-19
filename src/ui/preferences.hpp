#pragma once
        
#include <memory>
#include <string>

namespace vkd {
    class Preferences {
    public:
        Preferences() = default;
        ~Preferences() = default;
        Preferences(Preferences&&) = delete;
        Preferences(const Preferences&) = delete;

        static std::string Path();

        auto& last_opened_project() { return _last_opened_project; }
        const auto& last_opened_project() const { return _last_opened_project; }

        template <class Archive>
        void serialize(Archive& ar, const uint32_t version) {
            if (version == 0) {
                ar(_last_opened_project);
            }
        }
    private:
        std::string _last_opened_project = "";

    };
}