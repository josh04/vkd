import sys
import os.path

def write_header(path, classname):
    filename = path + classname.lower() + '.hpp'
    if not os.path.isfile(filename):
        header = open(filename, 'w')
        header.write("""#pragma once
        
#include <memory>

namespace vulkan {
    class """ + classname + """ {
    public:
        """ + classname + """() = default;
        ~""" + classname + """() = default;
        """ + classname + """(""" + classname + """&&) = delete;
        """ + classname + """(const """ + classname + """&) = delete;
    private:

    };
}""")
        print("wrote header " + filename)
    else:
        print("header already exists at " +filename)
    
def write_impl(path, classname):
    filename = path + classname.lower() + '.cpp'
    if not os.path.isfile(filename):
        header = open(filename, 'w')
        header.write("""#include \"""" + classname.lower() + """.hpp"

namespace vulkan {
    
}""")
        print("wrote impl " + filename)
    else:
        print("impl already exists at " +filename)
    
write_header("./", sys.argv[1])
write_impl("./", sys.argv[1])
