#ifndef TB_CONF_H
#define TB_CONF_H

#include "sdkconfig.h"

#ifndef CONFIG_TB_TOKEN
#define CONFIG_TB_TOKEN ""
#endif

#ifndef CONFIG_TB_DEVICE_KEY
#define CONFIG_TB_DEVICE_KEY ""
#endif

#ifndef CONFIG_TB_DEVICE_SECRET
#define CONFIG_TB_DEVICE_SECRET ""
#endif

#ifndef CONFIG_TB_TIMEOUT_MS
#define CONFIG_TB_TIMEOUT_MS 10000
#endif

#endif
