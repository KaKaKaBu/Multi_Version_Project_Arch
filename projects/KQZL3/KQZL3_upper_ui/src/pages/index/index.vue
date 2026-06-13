<template>
  <scroll-view class="page" scroll-y>
    <view class="hero-card">
      <view class="hero-header">
        <view>
          <text class="hero-title">KQZL3 空气质量控制台</text>
          <text class="hero-subtitle">{{ versionDescription }}</text>
        </view>
        <view class="hero-tags">
          <text class="chip" :class="alarmActive ? 'chip-danger' : 'chip-on'">{{ alarmActive ? '报警' : '正常' }}</text>
          <text class="chip chip-neutral">{{ modeLabel }}</text>
          <text class="chip" :class="connectionChipClass">{{ connectionLabel }}</text>
        </view>
      </view>
      <view class="hero-metrics">
        <view class="hero-metric">
          <text class="metric-value">{{ telemetry.pm25 ?? '--' }}</text>
          <text class="metric-label">PM2.5 μg/m³</text>
        </view>
        <view class="hero-metric">
          <text class="metric-value">{{ enabledSensorLabels }}</text>
          <text class="metric-label">当前版本传感器</text>
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
        <text class="badge" :class="alarmActive ? 'badge-danger' : 'badge-on'">{{ alarmActive ? '阈值超限' : '空气状态正常' }}</text>
      </view>
      <view class="sensor-grid">
        <SensorCard
          v-for="sensor in supportedSensors"
          :key="sensor.key"
          :label="sensor.label"
          :value="telemetry[sensor.key]"
          :unit="sensor.unit"
          :threshold="thresholds[sensor.thresholdKey]"
          :precision="sensor.precision"
        />
      </view>
    </view>

    <view class="card">
      <view class="card-header">
        <text class="card-title">工作模式</text>
        <text class="badge">{{ modeLabel }}</text>
      </view>
      <view class="mode-grid">
        <button
          v-for="mode in modeOptions"
          :key="mode.key"
          class="mode-btn"
          :class="telemetry.mode === mode.key ? 'active' : ''"
          :loading="pendingMode === mode.key"
          @click="setMode(mode.key)"
        >
          {{ mode.label }}
        </button>
      </view>
    </view>

    <view class="card">
      <view class="card-header">
        <text class="card-title">远程控制</text>
        <text class="badge">手动模式优先</text>
      </view>
      <DeviceSwitch
        v-for="device in supportedActuators"
        :key="device.key"
        :label="device.label"
        :model-value="telemetry[device.key]"
        :disabled="pending"
        @change="(state) => setDevice(device.key, state)"
      />
    </view>

    <view class="card">
      <view class="card-header">
        <text class="card-title">阈值设置</text>
        <text class="badge">自动报警依据</text>
      </view>
      <ThresholdEditor
        v-for="sensor in supportedThresholds"
        :key="sensor.thresholdKey"
        v-model="thresholds[sensor.thresholdKey]"
        :label="sensor.label"
        :unit="sensor.unit"
        :step="sensor.step"
        :pending="pendingThreshold === sensor.thresholdKey"
        @apply="() => applyThreshold(sensor.thresholdKey)"
      />
    </view>

    <view v-if="showCommunicationLog" class="card">
      <view class="card-header">
        <text class="card-title">通信日志</text>
        <button class="ghost-btn mini" size="mini" @click="copyLogs">复制</button>
      </view>
      <view v-if="logs.length === 0" class="empty">暂无日志，连接设备后会显示 TX/RX JSON。</view>
      <view v-else class="log-list">
        <view v-for="item in logs" :key="item.id" class="log-item">
          <text class="log-direction" :class="item.direction === 'TX' ? 'tx' : 'rx'">{{ item.direction }}</text>
          <text class="log-text">{{ item.payload }}</text>
        </view>
      </view>
    </view>
  </scroll-view>
</template>

<script setup>
import { computed, onUnmounted, reactive, ref } from 'vue'
import ConnectionPanel from '@/components/ConnectionPanel.vue'
import DeviceSwitch from '@/components/DeviceSwitch.vue'
import SensorCard from '@/components/SensorCard.vue'
import ThresholdEditor from '@/components/ThresholdEditor.vue'
import { boardMqttConfig, buildClientId } from '@/config/mqtt'
import {
  actuatorCatalog,
  describeVersion,
  getSupportedActuators,
  getSupportedSensors,
  getSupportedThresholds,
  modeOptions,
  resolveUpperFeatures,
  resolveUpperVersion,
} from '@/config/versionCapabilities'
import { createBleClient } from '@/services/bleClient'
import {
  buildGetStatusCommand,
  buildSetDeviceCommand,
  buildSetModeCommand,
  buildSetThresholdCommand,
  calculateAlarm,
  createDefaultTelemetry,
  createDefaultThresholds,
  normalizeTelemetry,
  normalizeThresholds,
} from '@/services/kqzl3Protocol'
import { createMqttClient } from '@/services/mqttClient'

const version = resolveUpperVersion()
const featureContext = resolveUpperFeatures(version)
const featureFlags = featureContext.flags
const showCommunicationLog = __UPPER_UI_DEBUG__
const telemetry = reactive(createDefaultTelemetry(featureFlags))
const thresholds = reactive(createDefaultThresholds(featureFlags))
const logs = ref([])
const lastSync = ref('')
const pending = ref(false)
const connecting = ref(false)
const pendingMode = ref('')
const pendingThreshold = ref('')
const transportState = ref('disconnected')
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

const supportedSensors = computed(() => getSupportedSensors(featureFlags))
const supportedThresholds = computed(() => getSupportedThresholds(featureFlags))
const supportedActuators = computed(() => getSupportedActuators())
const versionDescription = computed(() => describeVersion(version, featureFlags))
const enabledSensorLabels = computed(() => supportedSensors.value.map((item) => item.label).join('/'))
const alarmActive = computed(() => Number(telemetry.alarm) || calculateAlarm(telemetry, thresholds, featureFlags))
const modeLabel = computed(() => (modeOptions.find((item) => item.key === telemetry.mode) || modeOptions[0]).label)
const lastSyncDisplay = computed(() => lastSync.value || '尚未同步')
const connectionLabel = computed(() => transportState.value === 'connected' ? '已连接' : transportState.value === 'connecting' ? '连接中' : '未连接')
const connectionChipClass = computed(() => transportState.value === 'connected' ? 'chip-on' : 'chip-off')
const connectionPanelState = computed(() => {
  if (!featureFlags.remote) return 'unsupported'
  return transportState.value
})
const connectionTitle = computed(() => {
  if (featureFlags.aliyun) return '阿里云 MQTT'
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
  transportBridge = createBleClient({
    onTelemetry: handleTelemetry,
    onStatus: (state) => {
      transportState.value = state
      connecting.value = state === 'scanning' || state === 'connecting'
    },
    onError: (error) => {
      connecting.value = false
      pushLog('ERR', error.message || String(error))
    },
  })
  try {
    await transportBridge.connect()
  } catch (error) {
    connecting.value = false
    pushLog('ERR', error.message || String(error))
  }
}

function disconnectTransport() {
  if (transportBridge && typeof transportBridge.dispose === 'function') {
    transportBridge.dispose()
  }
  transportBridge = null
  connecting.value = false
  transportState.value = 'disconnected'
}

async function sendCommand(command) {
  pending.value = true
  try {
    if (!featureFlags.remote) {
      throw new Error('当前版本无远程通信')
    }
    if (!transportBridge || typeof transportBridge.publishCommand !== 'function') {
      throw new Error('通信未连接')
    }
    pushLog('TX', command)
    await transportBridge.publishCommand(command)
  } catch (error) {
    pushLog('ERR', error.message || String(error))
    uni.showToast({ title: error.message || '发送失败', icon: 'none' })
  } finally {
    pending.value = false
  }
}

async function refreshStatus() {
  await sendCommand(buildGetStatusCommand())
}

async function setMode(mode) {
  pendingMode.value = mode
  await sendCommand(buildSetModeCommand(mode))
  pendingMode.value = ''
}

async function setDevice(device, state) {
  if (telemetry.mode !== 'manual') {
    await sendCommand(buildSetModeCommand('manual'))
  }
  await sendCommand(buildSetDeviceCommand(device, state))
}

async function applyThreshold(sensor) {
  pendingThreshold.value = sensor
  await sendCommand(buildSetThresholdCommand(sensor, thresholds[sensor]))
  pendingThreshold.value = ''
}

function handleTelemetry(payload) {
  const normalized = normalizeTelemetry(payload, featureFlags, telemetry, thresholds)
  Object.assign(telemetry, normalized.telemetry)
  Object.assign(thresholds, normalizeThresholds(payload, featureFlags, thresholds))
  lastSync.value = formatTime(new Date())
  pushLog('RX', normalized.raw)
}

function pushLog(direction, payload) {
  if (!showCommunicationLog) return
  const text = typeof payload === 'string' ? payload : JSON.stringify(payload)
  logs.value.unshift({ id: `${Date.now()}_${Math.random()}`, direction, payload: text })
  if (logs.value.length > 30) {
    logs.value.length = 30
  }
}

function copyLogs() {
  const data = logs.value.map((item) => `[${item.direction}] ${item.payload}`).join('\n')
  if (!data) return
  uni.setClipboardData({ data })
}

function formatTime(date) {
  const pad = (value) => String(value).padStart(2, '0')
  return `${pad(date.getHours())}:${pad(date.getMinutes())}:${pad(date.getSeconds())}`
}
</script>

<style scoped>
.page {
  min-height: 100vh;
  padding: 24rpx;
  box-sizing: border-box;
  background: linear-gradient(180deg, #e1f5ef 0%, #eef4f7 360rpx, #eef4f7 100%);
}

.hero-card,
.card {
  margin-bottom: 24rpx;
  padding: 28rpx;
  border-radius: 28rpx;
  background: rgba(255, 255, 255, 0.96);
  box-shadow: 0 18rpx 42rpx rgba(15, 77, 94, 0.1);
}

.hero-header,
.card-header,
.hero-actions,
.control-row,
.form-row {
  display: flex;
  align-items: center;
  flex-wrap: wrap;
}

.hero-header,
.card-header {
  justify-content: space-between;
  gap: 18rpx;
}

.hero-title,
.card-title {
  display: block;
  color: #102a43;
  font-weight: 800;
}

.hero-title {
  font-size: 38rpx;
}

.card-title {
  font-size: 30rpx;
}

.hero-subtitle {
  display: block;
  margin-top: 10rpx;
  color: #486581;
  font-size: 24rpx;
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

.chip-on,
.badge-on {
  color: #0f8f70;
  background: #dff7ef;
}

.chip-off {
  color: #697586;
  background: #e6eef3;
}

.chip-danger,
.badge-danger {
  color: #b42318;
  background: #ffe3df;
}

.chip-neutral,
.badge {
  color: #334e68;
  background: #e6eef3;
}

.hero-metrics {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220rpx, 1fr));
  gap: 16rpx;
  margin-top: 28rpx;
}

.hero-metric {
  padding: 22rpx;
  border-radius: 22rpx;
  background: #f6fafc;
}

.metric-value {
  display: block;
  color: #0f8f70;
  font-size: 34rpx;
  font-weight: 800;
  overflow-wrap: anywhere;
  word-break: break-all;
}

.metric-label {
  display: block;
  margin-top: 10rpx;
  color: #697586;
  font-size: 22rpx;
}

.hero-actions,
.control-row {
  gap: 16rpx;
  margin-top: 24rpx;
}

.primary-btn,
.ghost-btn {
  flex: 1;
  height: 76rpx;
  line-height: 76rpx;
  border-radius: 18rpx;
  font-size: 26rpx;
}

.primary-btn {
  color: #ffffff;
  background: #0f8f70;
}

.ghost-btn {
  color: #0f8f70;
  background: #e6f6f1;
}

.ghost-btn.mini {
  flex: 0 0 auto;
  height: 54rpx;
  line-height: 54rpx;
  padding: 0 24rpx;
  font-size: 22rpx;
}

.connection-form {
  display: flex;
  flex-direction: column;
  gap: 14rpx;
}

.form-row {
  gap: 14rpx;
}

.input {
  flex: 1;
  min-width: 0;
  height: 68rpx;
  padding: 0 18rpx;
  border-radius: 16rpx;
  background: #ffffff;
  border: 1rpx solid #d9e8ef;
  color: #102a43;
  font-size: 24rpx;
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
  margin-top: 24rpx;
}

.mode-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220rpx, 1fr));
  gap: 14rpx;
  margin-top: 24rpx;
}

.mode-btn {
  height: 72rpx;
  line-height: 72rpx;
  border-radius: 18rpx;
  color: #334e68;
  background: #e6eef3;
  font-size: 24rpx;
}

.mode-btn.active {
  color: #ffffff;
  background: #0f8f70;
}

.empty {
  padding: 36rpx 0;
  color: #8a97a7;
  text-align: center;
  font-size: 24rpx;
}

.log-list {
  margin-top: 18rpx;
}

.log-item {
  min-width: 0;
  display: flex;
  align-items: flex-start;
  gap: 12rpx;
  padding: 16rpx 0;
  border-bottom: 1rpx solid #e6eef3;
}

.log-direction {
  flex-shrink: 0;
  width: 64rpx;
  color: #697586;
  font-size: 22rpx;
  font-weight: 700;
}

.log-direction.tx {
  color: #0f8f70;
}

.log-direction.rx {
  color: #2563eb;
}

.log-text {
  min-width: 0;
  flex: 1;
  color: #334e68;
  font-size: 22rpx;
  line-height: 1.5;
  word-break: break-all;
}

@media (max-width: 480px) {
  .hero-metrics,
  .sensor-grid,
  .mode-grid {
    grid-template-columns: 1fr;
  }
}
</style>
