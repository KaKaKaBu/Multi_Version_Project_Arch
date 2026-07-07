<template>
  <view class="device-switch">
    <view>
      <text class="device-label">{{ label }}</text>
      <text class="device-state" :class="modelValue ? 'on' : 'off'">{{ modelValue ? 'ON' : 'OFF' }}</text>
    </view>
    <switch :checked="!!modelValue" :disabled="disabled" color="#0f8f70" @change="handleChange" />
  </view>
</template>

<script setup>
defineProps({
  label: { type: String, required: true },
  modelValue: { type: [Boolean, Number], default: 0 },
  disabled: { type: Boolean, default: false },
})

const emit = defineEmits(['change'])

function handleChange(event) {
  emit('change', event.detail.value ? 1 : 0)
}
</script>

<style scoped>
.device-switch {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 22rpx 0;
  border-bottom: 1rpx solid #e6eef3;
}

.device-switch:last-child {
  border-bottom: none;
}

.device-label {
  display: block;
  color: #102a43;
  font-size: 28rpx;
  font-weight: 600;
}

.device-state {
  display: block;
  margin-top: 8rpx;
  color: #8a97a7;
  font-size: 22rpx;
}

.device-state.on {
  color: #0f8f70;
}

.device-state.off {
  color: #697586;
}
</style>
