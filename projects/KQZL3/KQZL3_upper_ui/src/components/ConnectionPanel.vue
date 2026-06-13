<template>
  <view class="connection-panel">
    <view class="connection-row">
      <view>
        <text class="connection-title">{{ title }}</text>
        <text class="connection-desc">{{ description }}</text>
      </view>
      <text class="connection-state" :class="state">{{ stateLabel }}</text>
    </view>
    <view class="connection-actions">
      <slot />
    </view>
  </view>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  title: { type: String, required: true },
  description: { type: String, default: '' },
  state: { type: String, default: 'disconnected' },
})

const stateLabel = computed(() => {
  const labels = {
    connected: '已连接',
    connecting: '连接中',
    scanning: '扫描中',
    disconnected: '未连接',
    unsupported: '不支持',
  }
  return labels[props.state] || props.state
})
</script>

<style scoped>
.connection-panel {
  padding: 22rpx;
  border-radius: 22rpx;
  background: #f6fafc;
  border: 1rpx solid #d9e8ef;
}

.connection-row {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 18rpx;
}

.connection-title {
  display: block;
  color: #102a43;
  font-size: 28rpx;
  font-weight: 700;
}

.connection-desc {
  display: block;
  margin-top: 8rpx;
  color: #697586;
  font-size: 22rpx;
  line-height: 1.5;
}

.connection-state {
  flex-shrink: 0;
  padding: 6rpx 14rpx;
  border-radius: 999rpx;
  color: #697586;
  background: #e6eef3;
  font-size: 22rpx;
}

.connection-state.connected {
  color: #0f8f70;
  background: #dff7ef;
}

.connection-state.connecting,
.connection-state.scanning {
  color: #996f00;
  background: #fff3c4;
}

.connection-state.unsupported {
  color: #b42318;
  background: #ffe3df;
}

.connection-actions {
  margin-top: 18rpx;
}
</style>
