#pragma once

#include "TaskScheduler.h"

namespace vkd {
    
    class HostScheduler {
    public:
        HostScheduler() = default;
        ~HostScheduler() = default;
        HostScheduler(HostScheduler&&) = delete;
        HostScheduler(const HostScheduler&) = delete;

        void init() {
            _task_scheduler = std::make_unique<enki::TaskScheduler>();
            _task_scheduler->Initialize();
        }

        enki::TaskScheduler& ts() { return *_task_scheduler; }

        enki::TaskSet * add(std::unique_ptr<enki::TaskSet> task) {
            enki::TaskSet * ptr = task.get();

            std::scoped_lock<std::mutex> lock(_task_mutex);
            _active_tasks.emplace(ptr, std::move(task));
            ts().AddTaskSetToPipe(ptr);
            return ptr;
        }

        void wait(enki::TaskSet * task) {
            std::scoped_lock lock(_task_mutex);
            auto search = _active_tasks.find(task);
            if (search == _active_tasks.end()) {
                return; // no task whatever
            }

            _task_scheduler->WaitforTask(task);
        }

        bool is_complete(enki::TaskSet * task) {
            std::scoped_lock lock(_task_mutex);
            auto search = _active_tasks.find(task);
            if (search == _active_tasks.end()) {
                return true; // no task whatever
            }

            return task->GetIsComplete();
        }

        void cleanup() {
            std::scoped_lock lock(_task_mutex);

            std::vector<enki::TaskSet *> to_erase;
            for (auto&& task : _active_tasks) {
                if (task.first->GetIsComplete()) {
                    to_erase.push_back(task.first);
                }
            }
            for (auto&& t : to_erase) {
                _active_tasks.erase(t);
            }
        }

    private:
        std::unique_ptr<enki::TaskScheduler> _task_scheduler = nullptr;

        std::mutex _task_mutex;
        std::map<enki::TaskSet *, std::unique_ptr<enki::TaskSet>> _active_tasks;
    };
}