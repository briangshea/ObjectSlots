#ifndef _OBJECTSLOTS_HPP_
#define _OBJECTSLOTS_HPP_

#include "ObjectSlots/objectslots_config.hpp"

#if defined(ENABLE_OBJECTSLOTS_V1)
# include "ObjectSlots/ObjectSlots_v1.hpp"
#endif //ENABLE_OBJECTSLOTS_V1

#if defined(ENABLE_OBJECTSLOTS_V2)
# include "ObjectSlots/ObjectSlots_v2.hpp"
#endif //ENABLE_OBJECTSLOTS_V1
#endif