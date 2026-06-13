function resolveFlags() {
  if (typeof __UPPER_FEATURE_FLAGS__ !== 'undefined') {
    return __UPPER_FEATURE_FLAGS__
  }
  return { common: true }
}

export function setupFeatures(app) {
  const flags = resolveFlags()
  Object.entries(flags).forEach(([key, enabled]) => {
    if (enabled) {
      app.config.globalProperties[`$kqzl3_${key}`] = true
    }
  })
}
