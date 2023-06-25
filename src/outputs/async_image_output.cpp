#include "async_image_output.hpp"
#include "graph/fake_node.hpp"

#include "ocio/ocio_functional.hpp"

#include "host_scheduler.hpp"

#include "compute/kernel.hpp"
#include "image.hpp"

namespace vkd {

    void AsyncImageOutput::init() {

    }

    bool AsyncImageOutput::update(ExecutionType type) {
        return false;
    }

    void AsyncImageOutput::commands(VkCommandBuffer buf, uint32_t width, uint32_t height) {

    }

    void AsyncImageOutput::execute(ExecutionType type, Stream& stream) {


        std::map<int, std::shared_ptr<Image>> ocio_images;        
        std::shared_ptr<Kernel> ocio_in_transform = ocio_functional::make_shader(*this, _input_node->get_output_image(), _input_node->get_output_image(), ocio_functional::working_space_index(), ocio_functional::screenshot_space_index(), ocio_images);

        command_buffer().begin();
        ocio_in_transform->dispatch(command_buffer(), _input_node->get_output_image()->dim().x, _input_node->get_output_image()->dim().y);
        //command_buffer().end();
        command_buffer().flush();

        enki::TaskSet * task = nullptr;
        auto fake_node = _input_node_e->fake_node();
        if (fake_node) {
            auto str = immediate_exr(_device, fake_node->node_name(), _format, _input_node->get_output_image(), task);
            fake_node->set_saved_as(str);
        }
        ts().wait(task);
        _promise.set_value(true);
    }

}