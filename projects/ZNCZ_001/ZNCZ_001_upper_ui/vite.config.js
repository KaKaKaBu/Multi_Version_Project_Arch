import { defineConfig } from 'vite'
import uni from '@dcloudio/vite-plugin-uni'

function resolveFeatureContext() {
  const list = (process.env.UPPER_FEATURES || '')
    .split(',')
    .map((item) => item.trim())
    .filter(Boolean)
  const flags = {}
  for (const key of list) {
    flags[key] = true
  }
  return { list, flags }
}

const ctx = resolveFeatureContext()

export default defineConfig({
  plugins: [
    uni(),
  ],
  define: {
    __UPPER_VERSION__: JSON.stringify(process.env.UPPER_VERSION || ''),
    __UPPER_FEATURES__: JSON.stringify(ctx.list),
    __UPPER_FEATURE_FLAGS__: JSON.stringify(ctx.flags),
  },
})
