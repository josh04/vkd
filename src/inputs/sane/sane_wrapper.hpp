#pragma once

#include <string>
#include <vector>
#ifndef _WIN32
#include <dlfcn.h>
#endif
#include "sane.h"
#include "sane_formats.hpp"

using FN_SANE_INIT = void (*)(SANE_Int *, SANE_Auth_Callback);
using FN_SANE_EXIT = void (*)();
using FN_SANE_GET_DEVICES = SANE_Status (*)(const SANE_Device ***, SANE_Bool);
using FN_SANE_OPEN = SANE_Status (*)(SANE_String_Const, SANE_Handle *);
using FN_SANE_CLOSE = void (*)(SANE_Handle);
using FN_SANE_GET_OPTION_DESCRIPTOR = const SANE_Option_Descriptor * (*)(SANE_Handle, SANE_Int);
using FN_SANE_CONTROL_OPTION = SANE_Status (*)(SANE_Handle, SANE_Int, SANE_Action, void *, SANE_Int *);
using FN_SANE_GET_PARAMETERS = SANE_Status (*)(SANE_Handle, SANE_Parameters *);
using FN_SANE_START = SANE_Status (*)(SANE_Handle);
using FN_SANE_READ = SANE_Status (*)(SANE_Handle, SANE_Byte *, SANE_Int, SANE_Int *);
using FN_SANE_CANCEL = void (*)(SANE_Handle);
using FN_SANE_SET_IO_MODE = SANE_Status (*)(SANE_Handle, SANE_Bool);
using FN_SANE_GET_SELECT_FD = SANE_Status (*)(SANE_Handle, SANE_Int *);
using FN_SANE_STRSTATUS = SANE_String_Const (*)(SANE_Status);

struct SanePtrs {
    FN_SANE_INIT fn_sane_init = nullptr;
    FN_SANE_EXIT fn_sane_exit = nullptr;
    FN_SANE_GET_DEVICES fn_sane_get_devices = nullptr;
    FN_SANE_OPEN fn_sane_open = nullptr;
    FN_SANE_CLOSE fn_sane_close = nullptr;
    FN_SANE_GET_OPTION_DESCRIPTOR fn_sane_get_option_descriptor = nullptr;
    FN_SANE_CONTROL_OPTION fn_sane_control_option = nullptr;
    FN_SANE_GET_PARAMETERS fn_sane_get_parameters = nullptr;
    FN_SANE_START fn_sane_start = nullptr;
    FN_SANE_READ fn_sane_read = nullptr;
    FN_SANE_CANCEL fn_sane_cancel = nullptr;
    FN_SANE_SET_IO_MODE fn_sane_set_io_mode = nullptr;
    FN_SANE_GET_SELECT_FD fn_sane_get_select_fd = nullptr;
    FN_SANE_STRSTATUS fn_sane_strstatus = nullptr;
};

namespace sane_wrapper {
    extern SanePtrs library;
    void init();

    struct SaneDevice {
        std::string name;
        std::string vendor;
        std::string model;
        std::string type;
    };


    enum class SaneOptionType {
        TBool,
        TInt,
        TFixed,
        TString,
        TButton,
        TGroup
    };

    enum class SaneUnit {
        None,
        Pixel,
        Bit,
        Mm,
        DPI,
        Percent,
        Microsecond,
    };

    enum class SaneConstraint {
        None,
        Range,
        IntList,
        StringList
    };

    struct SaneOption {
        int32_t index;
        std::string name;
        std::string title;
        std::string desc;
        SaneOptionType type;
        SaneUnit unit;
        int32_t size;
        int32_t capabilities;

        bool read_only;

        int32_t i_value = 0;
        std::string s_value = "";

        SaneConstraint constraint_type;
        int32_t i_minimum = 0;
        int32_t i_maximum = 255;

        std::vector<int32_t> int_value_list;
        std::vector<std::string> string_value_list;
    };

    bool initialised();

    const std::vector<SaneDevice>& devices();

    bool set_current_device(int32_t index, const std::string& device_name);
    int32_t current_index();

    void clear_current_device();

    bool current_device_valid();

    std::vector<SaneOption> options();
    void set_option(int32_t index, const std::string& option_name, const void * value);
    vkd::sane::SaneFormat format();

    void read_image(void * ptr, size_t sz);

    void set_sane_library_location(const std::string& new_path);
}
