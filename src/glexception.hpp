#ifndef AZURE_GLEXCEPTION_HPP
#define AZURE_GLEXCEPTION_HPP

#include <stdexcept>
#include <string>

#include "glerror.h"

namespace azure { // neccessary for a define?
	namespace exception {
		class GL : public std::runtime_error {
		public:
			GL(std::string msg) : std::runtime_error(msg) {}
		};

#ifndef NDEBUG
#define _glException() do {const char * err = _glError(); if (err != 0) {throw azure::exception::GL(err);}} while (false)
#else
#define _glException() while (false) {}
#endif

	}
}

#endif
