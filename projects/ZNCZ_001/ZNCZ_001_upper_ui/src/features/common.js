export function setupCommonFeature(app) {
    if (process.env.NODE_ENV !== "production") {
        console.info("[upper-ui] common feature enabled");
    }
}
