#pragma once
        
#include <memory>
#include <string>
#include <deque>

namespace vkd {
    class Preferences {
    public:
        Preferences();
        ~Preferences() = default;
        Preferences(Preferences&&) = delete;
        Preferences(const Preferences&) = delete;

        static std::string Path();

        void draw();
        void open(bool set) { _open = set; }
        bool open() const { return _open; }

        auto& last_opened_project() { return _last_opened_project; }
        const auto& last_opened_project() const { return _last_opened_project; }

        auto& photo_project() { return _photo_project; }
        const auto photo_project() const { return _photo_project; }

        auto& working_space() { return _working_space; }
        const auto working_space() const { return _working_space; }

        auto& default_input_space() { return _default_input_space; }
        const auto default_input_space() const { return _default_input_space; }

        auto& display_space() { return _display_space; }
        const auto display_space() const { return _display_space; }

        auto& scan_space() { return _scan_space; }
        const auto scan_space() const { return _scan_space; }

        auto& screenshot_space() { return _screenshot_space; }
        const auto screenshot_space() const { return _screenshot_space; }
        
        auto& sane_library() { return _sane_library; }
        const auto sane_library() const { return _sane_library; }

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

        void loaded();
        void saved() const {}

        template <class Archive>
        void serialize(Archive& ar, const uint32_t version) {
            if (version >= 0) {
                ar(_last_opened_project);
            }
            if (version >= 1) {
                ar(_recently_opened);
            }
            if (version >= 2) {
                ar(_photo_project);
            }
            if (version >= 3) {
                ar(_open, _working_space, _default_input_space, _display_space);
            }
            if (version >= 4) {
                ar(_sane_library);
            }
            if (version >= 5) {
                ar(_scan_space, _screenshot_space);
            }
        }
    private:
        std::string _last_opened_project = "";
        std::string _photo_project = "";

        std::deque<std::string> _recently_opened;

        std::string _default_input_space = ""; 
        std::string _working_space = ""; 
        std::string _display_space = ""; 
        std::string _scan_space = ""; 
        std::string _screenshot_space = ""; 

        std::string _sane_library = ""; 

        bool _open = false;

    };
}