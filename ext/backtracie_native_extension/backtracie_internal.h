#ifndef __BACKTRACIE_INTERNAL_H
#define __BACKTRACIE_INTERNAL_H

#include "ruby_shards.h"

VALUE frame_from_location(raw_location *the_location);
VALUE qualified_method_name_for_location(raw_location *the_location);

#endif
