<template>
  <scroll-view class="page" scroll-y>
    <view class="hero-card">
      <view class="hero-header">
        <view>
          <text class="hero-title">ZNCZ-001 智能插座</text>
          <text class="hero-subtitle">{{ connectionSummary }}</text>
        </view>
        <view class="hero-tags">
          <text v-for="chip in statusChips" :key="chip.label" class="chip" :class="chip.tone">{{ chip.label }} · {{ chip.value }}</text>
        </view>
      </view>
      <view class="hero-clock">
        <text class="clock-value">{{ status.time }}</text>
        <text class="clock-sub">ON {{ status.on_time }} · OFF {{ status.off_time }}</text>
        <text class="clock-sync">最近同步：{{ lastSyncDisplay }}</text>
      </view>
      <view class="hero-actions">
        <button class="ghost-btn" :loading="statusLoading" @click="refreshStatus">刷新状态</button>
        <button class="primary-btn" :loading="pending" @click="toggleRelay">
          {{ status.relay ? '关闭继电器' : '开启继电器' }}
        </button>
      </view>
    </view>

    <view class="card metrics-card">
      <view class="metrics-grid">
        <view class="metric-item" v-for="metric in dashboardMetrics" :key="metric.label">
          <text class="metric-label">{{ metric.label }}</text>
          <text class="metric-value">{{ metric.value }}</text>
          <text class="metric-desc">{{ metric.desc }}</text>
        </view>
      </view>
    </view>


    <view class="card status-card">
      <view class="card-header">
        <text class="card-title">实时状态</text>
        <text class="badge" :class="status.wifi === 'connected' ? 'badge-on' : 'badge-off'">WiFi · {{ wifiStateLabel }}</text>
      </view>
      <view class="status-grid">
        <view class="status-item">
          <text class="status-label">模式</text>
          <text class="status-value">{{ status.mode.toUpperCase() }}</text>
        </view>
        <view class="status-item">
          <text class="status-label">继电器</text>
          <text class="status-value" :class="status.relay ? 'accent' : 'muted'">{{ status.relay ? 'ON' : 'OFF' }}</text>
        </view>
        <view class="status-item">
          <text class="status-label">当前时间</text>
          <text class="status-value">{{ status.time }}</text>
        </view>
        <view class="status-item">
          <text class="status-label">开时间</text>
          <text class="status-value">{{ status.on_time }}</text>
        </view>
        <view class="status-item">
          <text class="status-label">关时间</text>
          <text class="status-value">{{ status.off_time }}</text>
        </view>
        <view class="status-item">
          <text class="status-label">上次上报</text>
          <text class="status-value">{{ lastSyncDisplay }}</text>
        </view>
      </view>
    </view>

    <view class="card control-card">
      <view class="card-header">
        <text class="card-title">手动控制</text>
      </view>
      <view class="control-row">
        <button class="primary-btn" :loading="pending" @click="() => setRelayState(1)">继电器开启</button>
        <button class="danger-btn" :loading="pending" @click="() => setRelayState(0)">继电器关闭</button>
      </view>
      <view class="control-row">
        <button class="ghost-btn" :loading="pending" @click="enterManualMode">切回手动模式</button>
        <button class="ghost-btn" :loading="pending" @click="refreshStatus">拉取一次 Telemetry</button>
      </view>
    </view>

    <view class="card schedule-card">
      <view class="card-header">
        <text class="card-title">定时配置</text>
      </view>
      <view class="form-grid">
        <view>
          <text class="label">开时间 (HH:MM:SS)</text>
          <input class="input" v-model="scheduleForm.on_time" placeholder="08:00:00" />
          <button class="ghost-btn" :loading="pending" @click="() => submitSchedule('on')">同步开启时间</button>
        </view>
        <view>
          <text class="label">关时间 (HH:MM:SS)</text>
          <input class="input" v-model="scheduleForm.off_time" placeholder="22:00:00" />
          <button class="ghost-btn" :loading="pending" @click="() => submitSchedule('off')">同步关闭时间</button>
        </view>
      </view>
    </view>

    <view class="card time-card">
      <view class="card-header">
        <text class="card-title">校时</text>
      </view>
      <view class="form-grid">
        <view>
          <text class="label">日期</text>
          <picker mode="date" :value="timeForm.date" @change="(e) => (timeForm.date = e.detail.value)">
            <view class="picker">{{ timeForm.date }}</view>
          </picker>
        </view>
        <view>
          <text class="label">时间</text>
          <picker mode="time" :value="timeForm.time" @change="(e) => (timeForm.time = e.detail.value)">
            <view class="picker">{{ timeForm.time }}</view>
          </picker>
        </view>
      </view>
      <button class="primary-btn" :loading="pending" @click="submitTime">发送 set_time 命令</button>
    </view>

    <view v-if="showCommunicationLog" class="card log-card">
      <view class="card-header">
        <text class="card-title"> MQTT 命令日志 </text>
        <button class="ghost-btn" size="mini" @click="copyLogs">复制最近日志</button>
      </view>
      <view v-if="logs.length === 0" class="empty">暂无命令，先执行一次控制试试。</view>
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
import { computed, onMounted, onUnmounted, reactive, ref } from 'vue'
import { boardMqttConfig, buildClientId } from '@/config/mqtt'
import { createMqttClient } from '@/services/mqttClient'

const showCommunicationLog = __UPPER_UI_DEBUG__

const status = reactive({
  relay: 0,
  mode: 'manual',
  time: '--:--:--',
  on_time: '08:00:00',
  off_time: '22:00:00',
  wifi: 'offline',
})

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

const scheduleForm = reactive({
  on_time: status.on_time,
  off_time: status.off_time,
})

const timeForm = reactive({
  date: formatDate(new Date()),
  time: formatTime(new Date()),
})

const logs = ref([])
const pending = ref(false)
const statusLoading = ref(false)
const lastSync = ref('')
const mqttStatus = reactive({ state: 'disconnected', lastError: '' })
let mqttBridge = null

const canUseMqtt = computed(() => Boolean(connection.host))

const connectionSummary = computed(() => canUseMqtt.value ? `${connection.host}:${connection.port}` : '未配置 Broker')

const wifiStateLabel = computed(() => {
  if (!status.wifi) return 'offline'
  return status.wifi === 'connected' ? '已连接' : status.wifi
})

const statusChips = computed(() => [
  { label: '模式', value: status.mode.toUpperCase(), tone: status.mode === 'timer' ? 'chip-accent' : 'chip-neutral' },
  { label: '继电器', value: status.relay ? 'ON' : 'OFF', tone: status.relay ? 'chip-on' : 'chip-off' },
  { label: 'WiFi', value: wifiStateLabel.value, tone: status.wifi === 'connected' ? 'chip-on' : 'chip-off' },
  { label: 'MQTT', value: mqttStatus.state === 'connected' ? '已连接' : mqttStatus.state === 'connecting' ? '连接中' : '未连接', tone: mqttStatus.state === 'connected' ? 'chip-on' : 'chip-off' },
])

const dashboardMetrics = computed(() => [
  { label: 'MQTT', value: mqttStatus.state === 'connected' ? '已连接' : mqttStatus.state === 'connecting' ? '连接中' : '未连接', desc: mqttStatus.state === 'connected' ? '实时订阅中' : '等待连接' },
  { label: 'WiFi', value: wifiStateLabel.value, desc: status.wifi === 'connected' ? '信号正常' : '离线或弱信号' },
  { label: '计划窗口', value: `${status.on_time} ~ ${status.off_time}`, desc: status.mode === 'timer' ? '定时执行中' : '当前手动模式' },
])

const lastSyncDisplay = computed(() => {
  if (!lastSync.value) return '尚未同步'
  return lastSync.value
})

onMounted(() => {
  if (canUseMqtt.value) {
    connectMqtt()
  }
})

onUnmounted(() => {
  disconnectMqtt()
})

async function connectMqtt() {
  if (!canUseMqtt.value) {
    uni.showToast({ title: '请填写 Broker Host', icon: 'none' })
    return
  }
  disconnectMqtt()
  mqttStatus.lastError = ''
  try {
    mqttBridge = createMqttClient({
      clientId: buildClientId('panel'),
      username: connection.username,
      password: connection.password,
      telemetryTopic: connection.telemetryTopic,
      commandTopic: connection.commandTopic,
      keepalive: boardMqttConfig.keepalive,
      wsEndpoint: connection.wsEndpoint,
      host: connection.host,
      port: connection.port,
      useTLS: connection.useTLS,
      onTelemetry: (payload) => {
        applyStatus(payload)
        pushLog('RX', payload)
      },
      onStatus: (state) => {
        mqttStatus.state = state
      },
      onError: (err) => {
        mqttStatus.lastError = err?.message || String(err)
      },
    })
    uni.showToast({ title: 'MQTT 连接中', icon: 'none' })
  } catch (err) {
    mqttStatus.lastError = err?.message || String(err)
    uni.showToast({ title: '连接失败', icon: 'none' })
  }
}

function disconnectMqtt() {
  if (mqttBridge) {
    mqttBridge.dispose()
    mqttBridge = null
  }
  mqttStatus.state = 'disconnected'
}

async function testConnection() {
  await refreshStatus()
}

async function refreshStatus() {
  statusLoading.value = true
  if (!canUseMqtt.value) {
    uni.showToast({ title: '请填写 Broker Host', icon: 'none' })
    statusLoading.value = false
    return
  }
  try {
    await sendCommand('get_status')
  } catch (err) {
    mqttStatus.lastError = err?.message || String(err)
    uni.showToast({ title: '读取失败', icon: 'none' })
  } finally {
    statusLoading.value = false
  }
}

async function toggleRelay() {
  await setRelayState(status.relay ? 0 : 1)
}

async function setRelayState(state) {
  await sendCommand('relay', { state })
}

async function enterManualMode() {
  await sendCommand('relay', { state: status.relay })
}

async function submitSchedule(kind) {
  const value = kind === 'on' ? scheduleForm.on_time : scheduleForm.off_time
  const parsed = normalizeTime(value)
  if (!parsed) {
    uni.showToast({ title: '时间格式应为 HH:MM:SS', icon: 'none' })
    return
  }
  await sendCommand(kind === 'on' ? 'set_on_time' : 'set_off_time', parsed)
}

async function submitTime() {
  const dateParts = timeForm.date.split('-').map((item) => Number(item))
  const timeParts = timeForm.time.split(':').map((item) => Number(item))
  if (dateParts.length !== 3 || timeParts.length < 2) {
    uni.showToast({ title: '请选择日期/时间', icon: 'none' })
    return
  }
  await sendCommand('set_time', {
    year: dateParts[0],
    month: dateParts[1],
    day: dateParts[2],
    hour: timeParts[0],
    minute: timeParts[1],
    second: timeParts[2] || 0,
  })
}

async function sendCommand(cmd, extra = {}) {
  const payload = { cmd, ...extra }
  pushLog('TX', payload)

  if (!mqttBridge) {
    throw new Error('MQTT 未连接')
  }

  pending.value = true
  try {
    mqttBridge.publishCommand(payload)
    uni.showToast({ title: '已发送', icon: 'success' })
  } catch (err) {
    mqttStatus.lastError = err?.message || String(err)
    uni.showToast({ title: '发送失败', icon: 'none' })
  } finally {
    pending.value = false
  }
}

function applyStatus(payload) {
  if (!payload) return
  const keys = ['relay', 'mode', 'time', 'on_time', 'off_time', 'wifi']
  keys.forEach((key) => {
    if (payload[key] !== undefined && payload[key] !== null) {
      status[key] = payload[key]
    }
  })
  scheduleForm.on_time = status.on_time
  scheduleForm.off_time = status.off_time
  lastSync.value = formatTimestamp(new Date())
}

function pushLog(direction, payload) {
  if (!showCommunicationLog) return
  const text = typeof payload === 'string' ? payload : JSON.stringify(payload)
  logs.value.unshift({ id: `${Date.now()}-${Math.random()}`, direction, payload: text })
  if (logs.value.length > 20) {
    logs.value.pop()
  }
}

async function copyLogs() {
  if (logs.value.length === 0) {
    uni.showToast({ title: '暂无日志', icon: 'none' })
    return
  }
  const text = logs.value.map((item) => `${item.direction}: ${item.payload}`).join('\n')
  await copyToClipboard(text)
  uni.showToast({ title: '已复制', icon: 'success' })
}

function normalizeTime(value) {
  if (!value) return null
  const parts = value.split(':').map((item) => Number(item))
  if (parts.length < 2) return null
  const [hour, minute, second = 0] = parts
  if (
    Number.isNaN(hour) ||
    Number.isNaN(minute) ||
    Number.isNaN(second) ||
    hour < 0 ||
    hour > 23 ||
    minute < 0 ||
    minute > 59 ||
    second < 0 ||
    second > 59
  ) {
    return null
  }
  return { hour, minute, second }
}

function buildTime(payload) {
  return `${pad(payload.hour)}:${pad(payload.minute)}:${pad(payload.second || 0)}`
}

function formatDate(date) {
  return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())}`
}

function formatTime(date) {
  return `${pad(date.getHours())}:${pad(date.getMinutes())}`
}

function formatTimestamp(date) {
  return `${formatDate(date)} ${pad(date.getHours())}:${pad(date.getMinutes())}:${pad(date.getSeconds())}`
}

function pad(num) {
  return String(num).padStart(2, '0')
}

async function copyToClipboard(text) {
  return new Promise((resolve, reject) => {
    uni.setClipboardData({
      data: text,
      success: resolve,
      fail: reject,
    })
  })
}
</script>

<style scoped lang="scss">

.page {
  min-height: 100vh;
  padding: 40rpx 32rpx 120rpx;
  box-sizing: border-box;
  background: linear-gradient(180deg, #f5fbff 0%, #eef5ff 40%, #ffffff 100%);
}

.hero-card {
  background: #ffffff url('data:image/svg+xml,%3Csvg width="400" height="200" viewBox="0 0 400 200" xmlns="http://www.w3.org/2000/svg"%3E%3Ccircle cx="60" cy="30" r="120" fill="%23e3f4ff"/%3E%3Ccircle cx="340" cy="160" r="120" fill="%23f0f7ff"/%3E%3C/svg%3E')
    no-repeat center/cover;
  border-radius: 36rpx;
  padding: 44rpx;
  color: #0b1f3a;
  margin-bottom: 32rpx;
  box-shadow: 0 30rpx 80rpx rgba(87, 142, 255, 0.18);
  border: 2rpx solid rgba(108, 160, 255, 0.18);
}

.hero-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  flex-wrap: wrap;
  gap: 24rpx;
}

.hero-title {
  font-size: 44rpx;
  font-weight: 700;
}

.hero-subtitle {
  font-size: 24rpx;
  color: #5b6c84;
}

.hero-tags {
  display: flex;
  flex-wrap: wrap;
  gap: 12rpx;
}

.chip {
  padding: 12rpx 26rpx;
  border-radius: 999rpx;
  font-size: 22rpx;
  background: rgba(88, 129, 255, 0.1);
  color: #4c5be5;
  border: 1rpx solid rgba(88, 129, 255, 0.25);
}

.chip-on {
  background: rgba(94, 193, 166, 0.18);
  color: #219c72;
  border-color: rgba(94, 193, 166, 0.55);
}

.chip-off {
  background: rgba(253, 157, 161, 0.2);
  color: #e35b67;
  border-color: rgba(253, 157, 161, 0.55);
}

.chip-accent {
  background: rgba(255, 210, 134, 0.24);
  color: #d68000;
  border-color: rgba(255, 210, 134, 0.6);
}

.chip-neutral {
  background: rgba(108, 160, 255, 0.12);
  color: #4867c7;
  border-color: transparent;
}

.hero-clock {
  margin-top: 32rpx;
}

.clock-value {
  font-size: 74rpx;
  font-weight: 700;
  color: #0d2962;
}

.clock-sub,
.clock-sync {
  display: block;
  margin-top: 8rpx;
  color: #5b6c84;
}

.hero-actions {
  margin-top: 36rpx;
  display: flex;
  gap: 24rpx;
}

.card {
  background: #ffffff;
  border-radius: 30rpx;
  padding: 32rpx;
  margin-bottom: 28rpx;
  color: #1f2b3d;
  box-shadow: 0 18rpx 50rpx rgba(118, 142, 196, 0.18);
  border: 1rpx solid rgba(132, 161, 208, 0.22);
}

.metrics-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200rpx, 1fr));
  gap: 24rpx;
}

.metric-item {
  background: #f4f8ff;
  border-radius: 26rpx;
  padding: 24rpx;
  border: 1rpx solid rgba(132, 161, 208, 0.3);
}

.metric-label {
  font-size: 24rpx;
  color: #6e7ea0;
}

.metric-value {
  display: block;
  font-size: 42rpx;
  font-weight: 600;
  color: #1e3d8f;
  margin: 10rpx 0;
}

.metric-desc {
  font-size: 22rpx;
  color: #8da0c1;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24rpx;
}

.card-title {
  font-size: 32rpx;
  font-weight: 600;
}

.badge {
  padding: 8rpx 18rpx;
  border-radius: 999rpx;
  font-size: 22rpx;
  background: rgba(76, 149, 255, 0.12);
  color: #4c7bfd;
}

.badge-on {
  background: rgba(103, 201, 172, 0.2);
  color: #24986d;
}

.badge-off {
  background: rgba(255, 117, 117, 0.22);
  color: #e05a5a;
}

.form-field,
.form-grid view {
  margin-bottom: 24rpx;
}

.form-grid {
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.label {
  font-size: 24rpx;
  color: #60738b;
  margin-bottom: 8rpx;
  display: block;
  font-weight: 500;
}

.input {
  background: #f4f8ff;
  border: 1rpx solid rgba(109, 143, 214, 0.35);
  border-radius: 18rpx;
  padding: 18rpx 24rpx;
  color: #1c2d50;
}

.picker {
  background: #f4f8ff;
  border-radius: 18rpx;
  padding: 18rpx 24rpx;
  color: #1c2d50;
  border: 1rpx solid rgba(109, 143, 214, 0.35);
}

.tips {
  font-size: 22rpx;
  color: #7c8da4;
  margin-bottom: 24rpx;
}

.status-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
}


.status-item {
  background: #f8fbff;
  border-radius: 22rpx;
  padding: 24rpx;
  border: 1rpx solid rgba(132, 161, 208, 0.18);
}

.status-label {
  font-size: 22rpx;
  color: #8b95aa;
}

.status-value {
  display: block;
  font-size: 32rpx;
  margin-top: 8rpx;
}

.status-value.accent {
  color: #2f7be8;
}

.status-value.muted {
  color: #8b95aa;
}

.control-row {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
  margin-bottom: 16rpx;
}

.log-list {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}


.log-item {
  display: flex;
  gap: 12rpx;
  background: #f4f8ff;
  border: 1rpx solid rgba(124, 156, 223, 0.25);
  padding: 18rpx;
  border-radius: 18rpx;
}

.log-direction {
  font-weight: 600;
}

.log-direction.tx {
  color: #2c7be3;
}

.log-direction.rx {
  color: #f09a24;
}

.log-text {
  flex: 1;
  color: #b8c0d4;
}

.empty {
  color: #667087;
  font-size: 24rpx;
}

.primary-btn,
.ghost-btn,
.danger-btn {
  border: none;
  border-radius: 999rpx;
  padding: 20rpx 28rpx;
  font-size: 28rpx;
  color: #0b111d;
  width: 100%;
}

.primary-btn {
  background: linear-gradient(120deg, #5ea8ff, #3f7bff);
  color: #ffffff;
  box-shadow: 0 10rpx 30rpx rgba(82, 140, 255, 0.35);
}

.ghost-btn {
  background: #eef4ff;
  color: #2c4ea3;
  border: 1rpx solid rgba(100, 140, 221, 0.6);
}

.danger-btn {
  background: rgba(255, 117, 117, 0.18);
  color: #d8414d;
  border: 1rpx solid rgba(255, 117, 117, 0.4);
}

button:disabled,
.primary-btn:disabled,
.ghost-btn:disabled,
.danger-btn:disabled {
  opacity: 0.5;
}

@media (min-width: 768px) {
  .form-grid {
    flex-direction: row;
  }

  .control-row {
    flex-direction: row;
  }

  .hero-actions {
    justify-content: flex-end;
  }

  .primary-btn,
  .ghost-btn,
  .danger-btn {
    width: auto;
  }
}
</style>
