#pragma once

#ifdef _WIN32
#	if LIBVKD_EXPORTS
#		define VKDEXPORT __declspec(dllexport)
#	else
#		define VKDEXPORT __declspec(dllimport)
#	endif
#else
#	define VKDEXPORT
#endif


