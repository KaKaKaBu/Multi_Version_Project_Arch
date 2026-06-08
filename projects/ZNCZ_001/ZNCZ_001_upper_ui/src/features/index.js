import { setupCommonFeature } from "./common";
import { setupWifiFeature } from "./wifi";
import { setupBleFeature } from "./ble";

const registry = {
    common: setupCommonFeature,
    wifi: setupWifiFeature,
    ble: setupBleFeature,
};

function resolveFlags() {
    if (typeof __UPPER_FEATURE_FLAGS__ !== "undefined") {
        return __UPPER_FEATURE_FLAGS__;
    }
    return {};
}

export function setupFeatures(app) {
    const flags = resolveFlags();
    Object.entries(registry).forEach(([key, installer]) => {
        if (flags[key]) {
            installer(app);
        }
    });
}
