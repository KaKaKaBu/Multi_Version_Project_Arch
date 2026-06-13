import { createSSRApp } from 'vue'
import App from './App.vue'
import { setupFeatures } from './features'

export function createApp() {
  const app = createSSRApp(App)
  setupFeatures(app)
  return { app }
}
