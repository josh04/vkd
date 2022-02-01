#pragma once
        
#include <memory>
#include <string>
#include <deque>

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

        const auto& recently_opened() const { return _recently_opened; }

        void add_recently_opened(std::string str) {
            
            auto iter = _recently_opened.begin();
            for (; iter != _recently_opened.end(); iter++) {
                if (*iter == str) {
                    _recently_opened.erase(iter);
                    break;
                }
            }

            _recently_opened.push_back(str);
            while (_recently_opened.size() > 10) {
                _recently_opened.pop_front();
            }
        }

        template <class Archive>
        void serialize(Archive& ar, const uint32_t version) {
            if (version >= 0) {
                ar(_last_opened_project);
            }
            if (version >= 1) {
                ar(_recently_opened);
            }
        }
    private:
        std::string _last_opened_project = "";

        std::deque<std::string> _recently_opened;

    };
}