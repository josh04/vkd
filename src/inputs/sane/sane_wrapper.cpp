#include "sane_wrapper.hpp"
#include "console.hpp"
#include "graph_exception.hpp"

#define vkd_sane_init sane_wrapper::library.fn_sane_init
#define vkd_sane_exit sane_wrapper::library.fn_sane_exit
#define vkd_sane_get_devices sane_wrapper::library.fn_sane_get_devices
#define vkd_sane_open sane_wrapper::library.fn_sane_open
#define vkd_sane_close sane_wrapper::library.fn_sane_close
#define vkd_sane_get_option_descriptor sane_wrapper::library.fn_sane_get_option_descriptor
#define vkd_sane_control_option sane_wrapper::library.fn_sane_control_option
#define vkd_sane_get_parameters sane_wrapper::library.fn_sane_get_parameters
#define vkd_sane_start sane_wrapper::library.fn_sane_start
#define vkd_sane_read sane_wrapper::library.fn_sane_read
#define vkd_sane_cancel sane_wrapper::library.fn_sane_cancel
#define vkd_sane_set_io_mode sane_wrapper::library.fn_sane_set_io_mode
#define vkd_sane_get_select_fd sane_wrapper::library.fn_sane_get_select_fd
#define vkd_sane_strstatus sane_wrapper::library.fn_sane_strstatus

namespace sane_wrapper {
    SanePtrs library;
    std::string s_path;
    bool s_initialised;
    std::vector<SaneDevice> s_devices;
    int32_t s_current_index = -1;

    extern "C" {
        static void auth_callback(SANE_String_Const resource, SANE_Char * username, SANE_Char * password) {
        }
    }

#ifdef _WIN32
    void init() {
    }
#endif

#ifdef __APPLE__
    void init() {
        s_current_index = -1;

        void * dylib_handle = dlopen((s_path + "/lib/libsane.1.dylib").c_str(), RTLD_LAZY);
        if (!dylib_handle) {
            throw std::runtime_error(std::string("Failed to load sane library at: ") + s_path);
        }

        #define CONCAT(A, B) A ## B
        #define LIBRARYFN(B) CONCAT(library.fn_, B)
        #define SANE_LOAD_FN(NAME) LIBRARYFN(NAME) = (decltype(LIBRARYFN(NAME)))dlsym(dylib_handle, #NAME); \
        if (!LIBRARYFN(NAME)) { \
            throw std::runtime_error(std::string("Failed to load sane library function ") + #NAME + " at: " + s_path); \
        }

        SANE_LOAD_FN(sane_init);
        SANE_LOAD_FN(sane_exit);
        SANE_LOAD_FN(sane_get_devices);
        SANE_LOAD_FN(sane_open);
        SANE_LOAD_FN(sane_close);
        SANE_LOAD_FN(sane_get_option_descriptor);
        SANE_LOAD_FN(sane_control_option);
        SANE_LOAD_FN(sane_get_parameters);
        SANE_LOAD_FN(sane_start);
        SANE_LOAD_FN(sane_read);
        SANE_LOAD_FN(sane_cancel);
        SANE_LOAD_FN(sane_set_io_mode);
        SANE_LOAD_FN(sane_get_select_fd);
        SANE_LOAD_FN(sane_strstatus);

        SANE_Int version_code;
        vkd_sane_init(&version_code, auth_callback);
        
        s_initialised = true;

        const SANE_Device ** devices = nullptr;
        auto status = vkd_sane_get_devices(&devices, true);
        s_devices.clear();
        while (devices) {
            const SANE_Device * device = *devices;
            if (device == nullptr) {
                break;
            }
            s_devices.emplace_back(SaneDevice{device->name, device->vendor, device->model, device->type});
            devices++;
        }
    }
#endif

    class ScopedSaneDevice {
    public:
        ScopedSaneDevice(std::string name) : _name(name) {
            if (vkd_sane_open(_name.c_str(), &_h) != SANE_STATUS_GOOD) {
                _h = nullptr;
            }
        }

        ~ScopedSaneDevice() {
            if (_h) {
                vkd_sane_close(_h);
            }
        }
        ScopedSaneDevice(ScopedSaneDevice&&) = delete;
        ScopedSaneDevice(const ScopedSaneDevice&) = delete;

        auto handle() const { return _h; }
        bool valid() const { return _h != nullptr; }
        const auto& name() const { return _name; }
    private:
        SANE_Handle _h = nullptr;
        std::string _name;
    };

    std::unique_ptr<ScopedSaneDevice> s_current_device = nullptr;

    void set_sane_library_location(const std::string& new_path)
    {
        s_path = new_path;
    }

    const std::vector<SaneDevice>& devices() { return s_devices; }

    SaneOptionType sane_to_enum(SANE_Value_Type t) {
        switch(t) {
        case SANE_TYPE_BOOL:
            return SaneOptionType::TBool;
        case SANE_TYPE_INT:
            return SaneOptionType::TInt;
        case SANE_TYPE_FIXED:
            return SaneOptionType::TFixed;
        case SANE_TYPE_STRING:
            return SaneOptionType::TString;
        case SANE_TYPE_BUTTON:
            return SaneOptionType::TButton;
        case SANE_TYPE_GROUP:
            return SaneOptionType::TGroup;
        }

        return SaneOptionType::TBool;
    }

    SaneUnit sane_to_enum(SANE_Unit t) {
        switch(t) {
        case SANE_UNIT_NONE:
            return SaneUnit::None;
        case SANE_UNIT_PIXEL:
            return SaneUnit::Pixel;
        case SANE_UNIT_BIT:
            return SaneUnit::Bit;
        case SANE_UNIT_MM:
            return SaneUnit::Mm;
        case SANE_UNIT_DPI:
            return SaneUnit::DPI;
        case SANE_UNIT_PERCENT:
            return SaneUnit::Percent;
        case SANE_UNIT_MICROSECOND:
            return SaneUnit::Microsecond;
        }
    }

    vkd::sane::SaneChannels sane_to_enum(SANE_Frame t) {
        switch(t) {
        case SANE_FRAME_GRAY:
            return vkd::sane::SaneChannels::Gray;
        case SANE_FRAME_RGB:
            return vkd::sane::SaneChannels::RGB;
        case SANE_FRAME_RED:
            return vkd::sane::SaneChannels::Red;
        case SANE_FRAME_GREEN:
            return vkd::sane::SaneChannels::Green;
        case SANE_FRAME_BLUE:
            return vkd::sane::SaneChannels::Blue;
        }

        return vkd::sane::SaneChannels::Gray;
    }

    SaneConstraint sane_to_enum(SANE_Constraint_Type t) {
        switch(t) {
        case SANE_CONSTRAINT_NONE:
            return SaneConstraint::None;
        case SANE_CONSTRAINT_RANGE:
            return SaneConstraint::Range;
        case SANE_CONSTRAINT_WORD_LIST:
            return SaneConstraint::IntList;
        case SANE_CONSTRAINT_STRING_LIST:
            return SaneConstraint::StringList;
        }

        return SaneConstraint::None;
    }

    bool set_current_device(int32_t index, const std::string& device_name) {
        if (s_current_index != index) {
            s_current_device = nullptr; // gotta release before we can re-acquire
            s_current_index = -1;
            s_current_device = std::make_unique<ScopedSaneDevice>(device_name);
            s_current_index = index;
        }
        return s_current_device->valid();
    }

    void clear_current_device() {
        s_current_device = nullptr;
    }

    int32_t current_index() {
        return s_current_index;
    }

    bool current_device_valid() {
        return s_current_device && s_current_device->valid();
    }

    std::vector<SaneOption> options() {
        if (!s_current_device || !s_current_device->valid()) {
            throw vkd::UpdateException("Invalid SANE device in options.");
        }

        std::vector<SaneOption> options;
        const SANE_Option_Descriptor * list_desc_ptr = vkd_sane_get_option_descriptor(s_current_device->handle(), 0);
        
        int32_t option_count = 0;

        auto status = vkd_sane_control_option(s_current_device->handle(), 0, SANE_ACTION_GET_VALUE, &option_count, 0);

        for (int i = 0; i < option_count; ++i) {
            const SANE_Option_Descriptor * desc_ptr = vkd_sane_get_option_descriptor(s_current_device->handle(), i);

            int32_t i_val = 0;
            std::string s_val = "";

            if (desc_ptr->type == SANE_TYPE_STRING) {
                //s_val.resize(desc_ptr->size);
                //const char * ptr = s_val.data();
                auto str = std::unique_ptr<char>(static_cast<char*>(malloc(desc_ptr->size)));

                if (!(desc_ptr->cap & SANE_CAP_INACTIVE)) {
                    auto status = vkd_sane_control_option(s_current_device->handle(), i, SANE_ACTION_GET_VALUE, str.get(), nullptr);
                    if (status != SANE_STATUS_GOOD) {
                        throw vkd::UpdateException(std::string("Failed to get SANE device option string: ") + desc_ptr->name + " (" + desc_ptr->desc + ") status: " + vkd_sane_strstatus(status));
                    }
                    s_val = str.get();
                } else {
                    s_val = "INACTIVE";
                }
            } else if (desc_ptr->size == sizeof(int32_t) && (desc_ptr->type == SANE_TYPE_BOOL || desc_ptr->type == SANE_TYPE_INT) && !(desc_ptr->cap & SANE_CAP_INACTIVE)) {
                auto status = vkd_sane_control_option(s_current_device->handle(), i, SANE_ACTION_GET_VALUE, &i_val, nullptr);
                if (status != SANE_STATUS_GOOD) {
                    throw vkd::UpdateException(std::string("Failed to get SANE device option integer: ") + desc_ptr->name + " (" + desc_ptr->desc + ") " + std::to_string(desc_ptr->type));
                }
            }
            

            SaneConstraint ctype = sane_to_enum(desc_ptr->constraint_type);
            int32_t i_min = 0;
            int32_t i_max = std::numeric_limits<int32_t>::max();
            std::vector<int> int_value_list;
            std::vector<std::string> string_value_list;
            if (desc_ptr->constraint_type == SANE_CONSTRAINT_RANGE && desc_ptr->constraint.range) {
                i_max = desc_ptr->constraint.range->max;
                i_min = desc_ptr->constraint.range->min;
            }
            if (desc_ptr->constraint_type == SANE_CONSTRAINT_WORD_LIST && desc_ptr->constraint.word_list) {
                int32_t sz = desc_ptr->constraint.word_list[0];
                for (int i = 1; i <= sz; ++i) {
                    int_value_list.push_back(desc_ptr->constraint.word_list[i]);
                }
            }
            if (desc_ptr->constraint_type == SANE_CONSTRAINT_STRING_LIST && desc_ptr->constraint.string_list) {
                auto val = desc_ptr->constraint.string_list;
                while (*val != nullptr) {
                    string_value_list.push_back(std::string(*val));
                    val++;
                }
            }

            if (std::string(desc_ptr->name).size() > 0) {
                options.emplace_back(SaneOption{i, desc_ptr->name, 
                    desc_ptr->title, 
                    desc_ptr->desc,
                    sane_to_enum(desc_ptr->type),
                    sane_to_enum(desc_ptr->unit),
                    desc_ptr->size,
                    desc_ptr->cap,
                    !SANE_OPTION_IS_SETTABLE(desc_ptr->cap), //read-only
                    i_val,
                    s_val,
                    ctype,
                    i_min,
                    i_max,
                    int_value_list,
                    string_value_list
                });
            }
        }
        return options;
    }

    void set_option(int32_t index, const std::string& option_name, const void * value) {
        if (!s_current_device || !s_current_device->valid()) {
            throw vkd::UpdateException("Invalid SANE device in set_option.");
        }

        SANE_Int info = 0;
        SANE_Status status = vkd_sane_control_option(s_current_device->handle(), index, SANE_ACTION_SET_VALUE, const_cast<void *>(value), &info);
        if (status != SANE_STATUS_GOOD) {
            throw vkd::UpdateException(std::string("Failed to set SANE device option: ") + option_name); 
        }
    }

    vkd::sane::SaneFormat format() {
        if (!s_current_device || !s_current_device->valid()) {
            throw vkd::UpdateException("Invalid SANE device in format.");
        }

        SANE_Parameters params;
        auto status = vkd_sane_get_parameters(s_current_device->handle(), &params);
        if (status != SANE_STATUS_GOOD) {
            throw vkd::UpdateException(std::string("Failed to get SANE device parameters: ") + s_current_device->name()); 
        }

        return vkd::sane::SaneFormat{sane_to_enum(params.format), /* params.last_frame > 0, */ params.bytes_per_line, params.pixels_per_line, params.lines, params.depth};
    }

    class ScopedSaneStart {
    public:
        ScopedSaneStart(SANE_Handle h): _h(h) {
            if (vkd_sane_start(_h)) {
                _h = nullptr;
            }
        }

        ~ScopedSaneStart() {
            if (_h) {
                vkd_sane_cancel(_h);
            }
        }
        ScopedSaneStart(ScopedSaneStart&&) = delete;
        ScopedSaneStart(const ScopedSaneStart&) = delete;

        auto handle() const { return _h; }
        bool valid() const { return _h != nullptr; }
    private:
        SANE_Handle _h = nullptr;
    };

    void read_image(void * ptr, size_t sz) {
        if (!s_current_device || !s_current_device->valid()) {
            throw vkd::UpdateException("Invalid SANE device in read_image.");
        }

        ScopedSaneStart start(s_current_device->handle());
        if (!start.valid()) {
            throw vkd::UpdateException("SANE device did not start.");
        }

        int32_t length_read = 0;
        int32_t total_read = 0;
        SANE_Byte * dest = (SANE_Byte*)ptr;
        SANE_Status status = SANE_STATUS_GOOD;
        int32_t reads = 0;


        while (status == SANE_STATUS_GOOD) {
            if (sz - total_read <= 0) {
                vkd::console << "Filled upload buffer in Sane read." << std::endl;
                while (SANE_STATUS_GOOD == vkd_sane_read(s_current_device->handle(), nullptr, 0, nullptr)) {}
                break;
            }
            length_read = 0;
            status = vkd_sane_read(s_current_device->handle(), dest, (int32_t)sz - total_read, &length_read);
            dest += length_read;
            total_read += length_read;
            reads++;
            vkd::console << "Read " << length_read << " bytes for a total of " << total_read << " bytes in " << reads << " reads. " << std::endl;
        }

        vkd::console << "Scan completed." << std::endl;

        if (status != SANE_STATUS_EOF && status != SANE_STATUS_GOOD) {
            throw vkd::UpdateException(std::string("SANE error while reading: ") + vkd_sane_strstatus(status));
        }
    }

    bool initialised() {
        return s_initialised;
    }

}