#ifndef HA_DISCOVERY_H
#define HA_DISCOVERY_H

#include <PubSubClient.h>

class HADiscovery
{
public:
  static void publish(PubSubClient& client);
};

#endif /* HA_DISCOVERY_H */
