/**
 * @file gm_registry.h
 * @brief A Sub-GHz protocol registry that adds Genie to the firmware's set.
 *
 * The firmware's own registry (used everywhere else in GarageMate) has no
 * Genie entry, so a receiver built from it can never decode a Genie remote.
 * This registry lists Genie plus the stock protocols, and is installed on the
 * dedicated environment the capture screen uses.
 */
#pragma once

#include <lib/subghz/registry.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @return a registry containing Genie alongside the firmware protocols. */
const SubGhzProtocolRegistry* gm_registry_get(void);

#ifdef __cplusplus
}
#endif
