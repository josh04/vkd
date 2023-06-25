#pragma once

#include <memory>
#include <mutex>
#include <map>
#include <optional>

#include "sane_formats.hpp"
#include "parameter.hpp"

#include "host_cache.hpp"

namespace vkd {
    class EngineNode;
    namespace sane {
        class Service {
        public:
            Service() = default;
            ~Service() = default;
            Service(Service&&) = delete;
            Service(const Service&) = delete;

            void init();
            void shutdown();

            std::vector<std::string> devices();

            std::map<int32_t, ParamPtr> params(EngineNode& node, int32_t device);

            std::optional<UploadFormat> format(int32_t device, const std::map<int32_t, ParamPtr>& params);

            std::optional<std::unique_ptr<StaticHostImage>> scan(int32_t device, const std::map<int32_t, ParamPtr>& params);

            static Service& Get();
            static void Shutdown();

        private:
            static std::mutex _mutex;
            static std::unique_ptr<Service> _singleton;

            std::vector<std::string> _devices();
            std::optional<UploadFormat> _format(int32_t device, const std::map<int32_t, ParamPtr>& params);
            void set_sane_options(const std::map<int32_t, ParamPtr>& dynamic_params);

            std::mutex _sane_mutex;
        };
    }
}