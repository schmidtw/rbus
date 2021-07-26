#ifndef RBUS_HANDLE_H
#define RBUS_HANDLE_H

#include "rbus_element.h"
#include "rbus_subscriptions.h"
#include <rtConnection.h>
#include <rtVector.h>

struct _rbusHandle
{
  int                   inUse;
  char*                 componentName;
  elementNode*          elementRoot;

  /* consumer side subscriptions FIXME - 
    this needs to be an associative map instead of list/vector*/
  rtVector              eventSubs; 

  /* provider side subscriptions */
  rbusSubscriptions_t   subscriptions; 

  rtVector              messageCallbacks;
  rtConnection          connection;
};

#endif
