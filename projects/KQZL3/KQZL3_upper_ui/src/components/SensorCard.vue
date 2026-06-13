<template>
  <view class="sensor-card" :class="toneClass">
    <view class="sensor-head">
      <text class="sensor-label">{{ label }}</text>
      <text class="sensor-status">{{ statusLabel }}</text>
    </view>
    <view class="sensor-value-row">
      <text class="sensor-value">{{ displayValue }}</text>
      <text class="sensor-unit">{{ unit }}</text>
    </view>
    <view class="sensor-foot">
      <text>阈值 {{ threshold }} {{ unit }}</text>
    </view>
  </view>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  label: { type: String, required: true },
  value: { type: [Number, String, null], default: null },
  unit: { type: String, default: '' },
  threshold: { type: [Number, String], default: '--' },
  precision: { type: Number, default: 0 },
})

const numericValue = computed(() => Number(props.value))
const hasValue = computed(() => props.value !== null && props.value !== undefined && props.value !== '' && Number.isFinite(numericValue.value))
const isAlarm = computed(() => hasValue.value && Number(props.threshold) < numericValue.value)
const displayValue = computed(() => {
  if (!hasValue.value) return '--'
  return numericValue.value.toFixed(props.precision)
})
const statusLabel = computed(() => isAlarm.value ? '超限' : '正常')
const toneClass = computed(() => isAlarm.value ? 'is-alarm' : 'is-normal')
</script>

<style scoped>
.sensor-card {
  padding: 24rpx;
  border-radius: 24rpx;
  background: #ffffff;
  border: 1rpx solid #d9e8ef;
  box-shadow: 0 12rpx 28rpx rgba(15, 77, 94, 0.08);
}

.sensor-card.is-alarm {
  border-color: #ffb4a9;
  background: #fff7f5;
}

.sensor-head,
.sensor-value-row,
.sensor-foot {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.sensor-label {
  color: #334e68;
  font-size: 26rpx;
  font-weight: 600;
}

.sensor-status {
  padding: 4rpx 12rpx;
  border-radius: 999rpx;
  color: #0f8f70;
  background: #dff7ef;
  font-size: 22rpx;
}

.is-alarm .sensor-status {
  color: #b42318;
  background: #ffe3df;
}

.sensor-value-row {
  margin-top: 20rpx;
  justify-content: flex-start;
  gap: 10rpx;
}

.sensor-value {
  color: #102a43;
  font-size: 56rpx;
  font-weight: 700;
  line-height: 1;
}

.sensor-unit {
  color: #697586;
  font-size: 24rpx;
  margin-top: 18rpx;
}

.sensor-foot {
  margin-top: 18rpx;
  color: #8a97a7;
  font-size: 22rpx;
}
</style>
