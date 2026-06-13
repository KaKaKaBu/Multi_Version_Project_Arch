<template>
  <scroll-view class="page" scroll-y>
    <view class="hero-card">
      <view class="hero-header">
        <view>
          <text class="hero-title">DCLD_001 倒车雷达</text>
          <text class="hero-subtitle">{{ versionDescription }}</text>
        </view>
        <view class="hero-tags">
          <text class="chip" :class="alarmActive ? 'chip-danger' : 'chip-on'">{{ alarmActive ? '报警' : '安全' }}</text>
          <text class="chip chip-neutral">{{ modeLabel }}</text>
          <text class="chip" :class="connectionChipClass">{{ connectionLabel }}</text>
        </view>
      </view>
      <view class="hero-metrics">
        <view class="hero-metric">
          <text class="metric-value">{{ telemetry.distance_cm ?? '--' }}</text>
          <text class="metric-label">当前距离 cm</text>
        </view>
        <view class="hero-metric">
          <text class="metric-value">{{ thresholds.distance }}</text>
          <text class="metric-label">报警阈值 cm</text>
        </view>
        <view class="hero-metric">
          <text class="metric-value">{{ lastSyncDisplay }}</text>
          <text class="metric-label">最近同步</text>
        </view>
      </view>
      <view class="hero-actions">
        <button class="ghost-btn" :loading="pending" @click="refreshStatus">刷新状态</button>
      </view>
    </view>

    <view class="card">
      <view class="card-header">
        <text class="card-title">通信连接</text>
      </view>
      <ConnectionPanel :title="connectionTitle" :description="connectionDescription" :state="connectionPanelState">
        <view v-if="featureFlags.wifi" class="connection-form">
          <input class="input" v-model="connection.wsEndpoint" placeholder="MQTT WebSocket Endpoint" />
          <view class="form-row">
            <input class="input" v-model="connection.host" placeholder="Broker Host" />
            <input class="input small" type="number" v-model="connection.port" placeholder="Port" />
          </view>
          <view class="form-row">
            <input class="input" v-model="connection.commandTopic" placeholder="命令 Topic" />
            <input class="input" v-model="connection.telemetryTopic" placeholder="遥测 Topic" />
          </view>
        </view>
        <view v-if="featureFlags.ble" class="ble-note">
          <text>Android App 真机可连接已配对 JDY-31 经典蓝牙 SPP 设备，使用透明串口 JSON 协议。</text>
        </view>
        <view class="control-row">
          <button class="primary-btn" :disabled="!canConnect" :loading="connecting" @click="connectTransport">连接</button>
          <button class="ghost-btn" @click="disconnectTransport">断开</button>
        </view>
      </ConnectionPanel>
    </view>

    <view class="card">
      <view class="card-header">
        <text class="card-title">实时数据</text>
        <text class="badge" :class="alarmActive ? 'badge-danger' : 'badge-on'">{{ alarmActive ? '低于阈值' : '距离安全' }}</text>
      </view>
      <view class="sensor-grid">
        <SensorCard
          v-for="metric in supportedMetrics"
          :key="metric.key"
          :label="metric.label"
          :value="telemetry[metric.key]"
          :unit="metric.unit"
          :threshold="metric.thresholdKey ? thresholds[metric.thresholdKey] : '--'"
          :precision="metric.precision"
        />
      </view>
    </view>

    <view class="card">
      <view class="card-header">
        <text class="card-title">阈值设置</text>
        <text class="badge">Key1切换 / Key2+ / Key3-</text>
      </view>
      <ThresholdEditor
        v-for="threshold in supportedThresholds"
        :key="threshold.key"
        v-model="thresholds[threshold.key]"
        :label="threshold.label"
        :unit="threshold.unit"
        :step="threshold.step"
        :pending="pendingThreshold === threshold.key"
        @apply="() => applyThreshold(threshold.key)"
      />
    </view>

    <view v-if="featureFlags.camera" class="card">
      <view class="card-header">
        <text class="card-title">ESP32-CAM 视频</text>
        <text class="badge">DCLD-007</text>
      </view>
      <input class="input" v-model="cameraUrl" placeholder="ESP32-CAM Stream URL" />
      <view class="camera-box">
        <image v-if="cameraUrl" class="camera-stream" :src="cameraUrl" mode="aspectFit" />
        <text v-else class="empty">等待固件遥测 camera_url，或手动输入视频流地址。</text>
      </view>
    </view>

    <view v-if="showCommunicationLog" class="card">
      <view class="card-header">
        <text class="card-title">通信日志</text>
        <button class="ghost-btn mini" size="mini" @click="copyLogs">复制</button>
      </view>
      <view v-if="logs.length === 0" class="empty">暂无日志，连接设备后会显示 TX/RX JSON。</view>
      <view v-else class="log-list">
        <view v-for="item in logs" :key="item.id" class="log-item">
          <text class="log-direction" :class="item.direction === 'TX' ? 'tx' : item.direction === 'ERR' ? 'err' : 'rx'">{{ item.direction }}</text>
          <text class="log-text">{{ item.payload }}</text>
        </view>
      </view>
    </view>
  </scroll-view>
</template>

<script setup>
import { computed, onUnmounted, reactive, ref, watch } from 'vue'
import ConnectionPanel from '@/components/ConnectionPanel.vue'
import SensorCard from '@/components/SensorCard.vue'
import ThresholdEditor from '@/components/ThresholdEditor.vue'
import { boardMqttConfig, buildClientId } from '@/config/mqtt'
import {
  describeVersion,
  getSupportedMetrics,
  getSupportedThresholds,
  modeOptions,
  resolveUpperFeatures,
  resolveUpperVersion,
} from '@/config/versionCapabilities'
import { createBleClient } from '@/services/bleClient'
import {
  buildGetStatusCommand,
  buildSetThresholdCommand,
  calculateAlarm,
  createDefaultTelemetry,
  createDefaultThresholds,
  normalizeTelemetry,
  normalizeThresholds,
} from '@/services/dcldProtocol'
import { createMqttClient } from '@/services/mqttClient'

const version = resolveUpperVersion()
const featureContext = resolveUpperFeatures(version)
const featureFlags = featureContext.flags
const showCommunicationLog = __UPPER_UI_DEBUG__
const telemetry = reactive(createDefaultTelemetry(featureFlags))
const thresholds = reactive(createDefaultThresholds())
const logs = ref([])
const lastSync = ref('')
const pending = ref(false)
const connecting = ref(false)
const pendingThreshold = ref('')
const transportState = ref('disconnected')
const cameraUrl = ref('')
let transportBridge = null

const connection = reactive({
  host: boardMqttConfig.brokerHost,
  port: boardMqttConfig.brokerPort,
  wsEndpoint: boardMqttConfig.wsEndpoint,
  username: boardMqttConfig.username,
  password: boardMqttConfig.password,
  commandTopic: boardMqttConfig.commandTopic,
  telemetryTopic: boardMqttConfig.telemetryTopic,
  useTLS: boardMqttConfig.useTLS,
})

const supportedMetrics = computed(() => getSupportedMetrics(featureFlags))
const supportedThresholds = computed(() => getSupportedThresholds())
const versionDescription = computed(() => describeVersion(version, featureFlags))
const alarmActive = computed(() => Number(telemetry.alarm) || calculateAlarm(telemetry, thresholds))
const modeLabel = computed(() => (modeOptions.find((item) => item.key === telemetry.mode) || modeOptions[0]).label)
const lastSyncDisplay = computed(() => lastSync.value || '尚未同步')
const connectionLabel = computed(() => transportState.value === 'connected' ? '已连接' : transportState.value === 'connecting' ? '连接中' : '未连接')
const connectionChipClass = computed(() => transportState.value === 'connected' ? 'chip-on' : 'chip-off')
const connectionPanelState = computed(() => {
  if (!featureFlags.remote) return 'unsupported'
  return transportState.value
})
const connectionTitle = computed(() => {
  if (featureFlags.wifi) return 'WiFi MQTT'
  if (featureFlags.ble) return 'JDY-31 蓝牙'
  return '无远程通信'
})
const connectionDescription = computed(() => {
  if (!featureFlags.remote) return '当前版本不包含远程通信模块。'
  if (featureFlags.wifi) return `${connection.wsEndpoint || `${connection.host}:${connection.port}`} · TX ${connection.commandTopic} · RX ${connection.telemetryTopic}`
  if (featureFlags.ble) return '通过 JDY-31 透明串口收发 JSON 命令。'
  return '当前版本未配置可用通信方式。'
})
const canConnect = computed(() => featureFlags.remote)

watch(() => telemetry.camera_url, (value) => {
  if (value) cameraUrl.value = value
})


onUnmounted(() => {
  disconnectTransport()
})

function connectTransport() {
  if (featureFlags.wifi) {
    connectMqtt()
    return
  }
  if (featureFlags.ble) {
    connectBle()
    return
  }
  uni.showToast({ title: '当前版本无远程通信', icon: 'none' })
}

function connectMqtt() {
  disconnectTransport()
  connecting.value = true
  try {
    transportBridge = createMqttClient({
      clientId: buildClientId(`v${version}`),
      username: connection.username,
      password: connection.password,
      telemetryTopic: connection.telemetryTopic,
      commandTopic: connection.commandTopic,
      keepalive: boardMqttConfig.keepalive,
      wsEndpoint: connection.wsEndpoint,
      host: connection.host,
      port: Number(connection.port),
      useTLS: connection.useTLS,
      onTelemetry: handleTelemetry,
      onStatus: (state) => {
        transportState.value = state
        connecting.value = state === 'connecting'
      },
      onError: (error) => {
        connecting.value = false
        pushLog('ERR', error.message || String(error))
      },
    })
  } catch (error) {
    connecting.value = false
    pushLog('ERR', error.message || String(error))
  }
}

async function connectBle() {
  disconnectTransport()
  connecting.value = true
  transportState.value = 'connecting'
  transportBridge = createBleClient({
    onTelemetry: handleTelemetry,
    onStatus: (state) => {
      transportState.value = state
      connecting.value = state === 'connecting' || state === 'scanning'
    },
    onError: (error) => {
      connecting.value = false
      pushLog('ERR', error.message || JSON.stringify(error))
    },
  })
  try {
    if (transportBridge.connect) {
      await transportBridge.connect({ namePrefix: 'JDY' })
    }
  } catch (error) {
    connecting.value = false
    transportState.value = 'disconnected'
    pushLog('ERR', error.message || String(error))
  }
}

function disconnectTransport() {
  if (transportBridge && transportBridge.dispose) {
    transportBridge.dispose()
  }
  transportBridge = null
  connecting.value = false
  transportState.value = 'disconnected'
}

function handleTelemetry(payload) {
  const { telemetry: next, raw } = normalizeTelemetry(payload, featureFlags, telemetry, thresholds)
  Object.assign(telemetry, next)
  Object.assign(thresholds, normalizeThresholds(raw, thresholds))
  lastSync.value = new Date().toLocaleTimeString()
  pushLog('RX', JSON.stringify(raw))
}

function publishCommand(command) {
  const text = JSON.stringify(command)
  if (!featureFlags.remote) {
    uni.showToast({ title: '当前版本无远程通信', icon: 'none' })
    return
  }
  if (!transportBridge || !transportBridge.publishCommand) {
    uni.showToast({ title: '通信未连接', icon: 'none' })
    return
  }
  pushLog('TX', text)
  transportBridge.publishCommand(command)
}

function refreshStatus() {
  publishCommand(buildGetStatusCommand())
}

function applyThreshold(key) {
  pendingThreshold.value = key
  publishCommand(buildSetThresholdCommand(thresholds[key]))
  setTimeout(() => {
    pendingThreshold.value = ''
  }, 300)
}

function pushLog(direction, payload) {
  if (!showCommunicationLog) return
  logs.value.unshift({
    id: `${Date.now()}_${Math.random()}`,
    direction,
    payload,
  })
  logs.value = logs.value.slice(0, 20)
}

function copyLogs() {
  const text = logs.value.map((item) => `[${item.direction}] ${item.payload}`).join('\n')
  uni.setClipboardData({ data: text })
}
</script>

<style scoped>
.page {
  min-height: 100vh;
  padding: 28rpx;
  box-sizing: border-box;
  background: #eef4f7;
}

.hero-card,
.card {
  margin-bottom: 24rpx;
  padding: 28rpx;
  border-radius: 28rpx;
  background: #ffffff;
  box-shadow: 0 16rpx 36rpx rgba(15, 77, 94, 0.08);
}

.hero-card {
  color: #ffffff;
  background: linear-gradient(135deg, #1d4ed8, #06b6d4);
}

.hero-header,
.card-header,
.hero-metrics,
.hero-actions,
.form-row,
.control-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-wrap: wrap;
  gap: 18rpx;
}

.hero-title,
.card-title {
  display: block;
  font-size: 34rpx;
  font-weight: 800;
}

.hero-subtitle {
  display: block;
  margin-top: 10rpx;
  max-width: 100%;
  color: rgba(255, 255, 255, 0.78);
  font-size: 23rpx;
  line-height: 1.5;
}

.hero-tags {
  display: flex;
  flex-wrap: wrap;
  justify-content: flex-end;
  gap: 10rpx;
}

.chip,
.badge {
  padding: 6rpx 14rpx;
  border-radius: 999rpx;
  font-size: 22rpx;
}

.chip {
  color: #ffffff;
  background: rgba(255, 255, 255, 0.18);
}

.chip-on,
.badge-on {
  color: #0f8f70;
  background: #dff7ef;
}

.chip-danger,
.badge-danger {
  color: #b42318;
  background: #ffe3df;
}

.chip-neutral {
  color: #334e68;
  background: #e6eef3;
}

.chip-off {
  color: #697586;
  background: #e6eef3;
}

.hero-metrics {
  margin-top: 34rpx;
}

.hero-metric {
  flex: 1 1 220rpx;
  min-width: 0;
  padding: 20rpx;
  border-radius: 22rpx;
  background: rgba(255, 255, 255, 0.16);
}

.metric-value {
  display: block;
  font-size: 42rpx;
  font-weight: 800;
}

.metric-label {
  display: block;
  margin-top: 8rpx;
  color: rgba(255, 255, 255, 0.75);
  font-size: 22rpx;
}

.hero-actions,
.control-row {
  margin-top: 24rpx;
  justify-content: flex-start;
}

.primary-btn,
.ghost-btn {
  min-width: 170rpx;
  height: 70rpx;
  line-height: 70rpx;
  border-radius: 18rpx;
  font-size: 26rpx;
}

.primary-btn {
  color: #ffffff;
  background: #2563eb;
}

.ghost-btn {
  color: #2563eb;
  background: #e0ecff;
}

.hero-card .ghost-btn,
.hero-card .primary-btn {
  background: rgba(255, 255, 255, 0.22);
  color: #ffffff;
}

.mini {
  min-width: 100rpx;
  height: 56rpx;
  line-height: 56rpx;
  font-size: 22rpx;
}

.card-title {
  color: #102a43;
  font-size: 30rpx;
}

.badge {
  color: #697586;
  background: #e6eef3;
}

.connection-form,
.ble-note {
  margin-top: 14rpx;
}

.input {
  width: 100%;
  height: 72rpx;
  margin-top: 14rpx;
  padding: 0 18rpx;
  border-radius: 18rpx;
  box-sizing: border-box;
  background: #f6fafc;
  color: #102a43;
  font-size: 25rpx;
}

.input.small {
  max-width: none;
  flex: 1 1 160rpx;
}

.ble-note {
  color: #697586;
  font-size: 24rpx;
  line-height: 1.6;
}

.sensor-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(240rpx, 1fr));
  gap: 18rpx;
  margin-top: 20rpx;
}

.camera-box {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 360rpx;
  margin-top: 18rpx;
  border-radius: 24rpx;
  background: #102a43;
  overflow: hidden;
}

.camera-stream {
  width: 100%;
  height: 360rpx;
}

.empty {
  padding: 22rpx;
  color: #8a97a7;
  font-size: 24rpx;
  line-height: 1.6;
}

.log-list {
  margin-top: 14rpx;
}

.log-item {
  min-width: 0;
  display: flex;
  gap: 12rpx;
  padding: 14rpx 0;
  border-bottom: 1rpx solid #e6eef3;
}

.log-direction {
  flex-shrink: 0;
  width: 70rpx;
  color: #0f8f70;
  font-weight: 700;
  font-size: 22rpx;
}

.log-direction.tx {
  color: #2563eb;
}

.log-direction.err {
  color: #b42318;
}

.log-text {
  min-width: 0;
  flex: 1;
  color: #334e68;
  font-size: 22rpx;
  overflow-wrap: anywhere;
  word-break: break-all;
}
</style>
