import sys
import os.path

def write_header(path, snakename, classname):
    filename = path + snakename.lower() + '.hpp'
    if not os.path.isfile(filename):
        header = open(filename, 'w')
        header.write("""#pragma once
        
#include <memory>

namespace vkd {
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
    
def write_impl(path, snakename, classname):
    filename = path + snakename.lower() + '.cpp'
    if not os.path.isfile(filename):
        header = open(filename, 'w')
        header.write("""#include \"""" + classname.lower() + """.hpp"

namespace vkd {
    
}""")
        print("wrote impl " + filename)
    else:
        print("impl already exists at " +filename)
    

temp = sys.argv[1].split('_')
clname = ''.join(ele.title() for ele in temp[0:])
write_header("./", sys.argv[1], clname)
write_impl("./", sys.argv[1], clname)
