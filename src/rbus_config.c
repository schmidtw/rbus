#include "rbus_config.h"
#include "rbus_log.h"
#include <stdlib.h>
#include <string.h>

/* These RBUS_* defines are the list of config settings with their default values
 * Each can be overridden using an environment variable of the same name
 */
#define RBUS_TMP_DIRECTORY      "/tmp"      /*temp directory where persistent data can be stored*/
#define RBUS_SUBSCRIBE_TIMEOUT   600000     /*subscribe retry timeout in miliseconds*/
#define RBUS_SUBSCRIBE_MAXWAIT   60000      /*subscribe retry max wait between retries in miliseconds*/


#define initStr(P,N) \
{ \
    char* V = getenv(#N); \
    P=strdup((V && strlen(V)) ? V : N); \
    RBUSLOG_DEBUG(#N"=%s",P); \
}

#define initInt(P,N) \
{ \
    char* V = getenv(#N); \
    P=((V && strlen(V)) ? atoi(V) : N); \
    RBUSLOG_DEBUG(#N"=%d",P); \
}

static rbusConfig_t* gConfig = NULL;

void rbusConfig_CreateOnce()
{
    if(gConfig)
        return;

    RBUSLOG_DEBUG("%s", __FUNCTION__);

    gConfig = malloc(sizeof(struct _rbusConfig_t));

    initStr(gConfig->tmpDir,                RBUS_TMP_DIRECTORY);
    initInt(gConfig->subscribeTimeout,      RBUS_SUBSCRIBE_TIMEOUT);
    initInt(gConfig->subscribeMaxWait,      RBUS_SUBSCRIBE_MAXWAIT);
}

void rbusConfig_Destroy()
{
    if(!gConfig)
        return;

    RBUSLOG_DEBUG("%s", __FUNCTION__);

    free(gConfig->tmpDir);
    free(gConfig);
    gConfig = NULL;
}

rbusConfig_t* rbusConfig_Get()
{
    return gConfig;
}
