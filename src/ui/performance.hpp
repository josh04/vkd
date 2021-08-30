#pragma once
        
#include "cereal/types/deque.hpp"

#include <memory>
#include <vector>
#include <string>
#include <deque>

namespace vkd {
    class Performance {
    public:
        Performance() = default;
        ~Performance() = default;
        Performance(Performance&&) = delete;
        Performance(const Performance&) = delete;

        void open(bool set) { _open = set; }
        void add(std::string type, int64_t duration_ms, std::string extra) { _reports.push_back({_index++, type, duration_ms, extra}); }

        struct Report {
            int64_t index;
            std::string type;
            int64_t duration_ms;
            std::string extra;
            
            template <class Archive>
            void serialize(Archive & ar, const uint32_t version) {
                if (version == 0) {
                    ar(index, type, duration_ms, extra);
                }
            }

        };

        void draw();

        template <class Archive>
        void serialize(Archive & ar, const uint32_t version) {
            if (version == 0) {
                ar(_reports, _reports_to_keep, _index, _open);
            }
        }
    private:
        std::deque<Report> _reports;
        int _reports_to_keep = 10;
        int64_t _index = 0;

        bool _open = false;
    };
}