#pragma once


#include "i_queue.h"

#include <stdint.h>
#include <memory>


extern std::shared_ptr<Queue<uint16_t> >(*make_queue)(size_t depth);
