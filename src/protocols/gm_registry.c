#include "gm_registry.h"
#include "gm_genie.h"

/*
 * We only need Genie in this registry -- it is used solely by the capture
 * screen, which is looking for a Genie remote. Keeping the list to one protocol
 * makes the decoder cheaper and avoids matching unrelated traffic. If other
 * decodable brands are added later, list them here too.
 */
static const SubGhzProtocol* const gm_registry_items[] = {
    &gm_protocol_genie,
};

static const SubGhzProtocolRegistry gm_registry = {
    .items = gm_registry_items,
    .size = COUNT_OF(gm_registry_items),
};

const SubGhzProtocolRegistry* gm_registry_get(void) {
    return &gm_registry;
}
