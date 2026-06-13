<template>
  <view class="threshold-editor">
    <view class="threshold-main">
      <view>
        <text class="threshold-label">{{ label }}</text>
        <text class="threshold-desc">步进 {{ step }} {{ unit }}</text>
      </view>
      <view class="threshold-value">
        <text>{{ modelValue }}</text>
        <text class="unit">{{ unit }}</text>
      </view>
    </view>
    <view class="threshold-actions">
      <button class="mini-btn" :disabled="disabled" @click="emitValue(Number(modelValue) - Number(step))">-</button>
      <input class="threshold-input" type="number" :disabled="disabled" :value="modelValue" @input="handleInput" />
      <button class="mini-btn" :disabled="disabled" @click="emitValue(Number(modelValue) + Number(step))">+</button>
      <button class="apply-btn" :loading="pending" :disabled="disabled" @click="$emit('apply')">同步</button>
    </view>
  </view>
</template>

<script setup>
defineProps({
  label: { type: String, required: true },
  unit: { type: String, default: '' },
  step: { type: Number, default: 1 },
  modelValue: { type: [Number, String], required: true },
  disabled: { type: Boolean, default: false },
  pending: { type: Boolean, default: false },
})

const emit = defineEmits(['update:modelValue', 'apply'])

function emitValue(value) {
  emit('update:modelValue', Number.isFinite(value) ? value : 0)
}

function handleInput(event) {
  emitValue(Number(event.detail.value))
}
</script>

<style scoped>
.threshold-editor {
  padding: 22rpx 0;
  border-bottom: 1rpx solid #e6eef3;
}

.threshold-editor:last-child {
  border-bottom: none;
}

.threshold-main,
.threshold-actions {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.threshold-label {
  display: block;
  color: #102a43;
  font-size: 28rpx;
  font-weight: 600;
}

.threshold-desc {
  display: block;
  margin-top: 6rpx;
  color: #8a97a7;
  font-size: 22rpx;
}

.threshold-value {
  display: flex;
  align-items: baseline;
  gap: 6rpx;
  color: #0f8f70;
  font-size: 34rpx;
  font-weight: 700;
}

.unit {
  color: #697586;
  font-size: 22rpx;
  font-weight: 400;
}

.threshold-actions {
  margin-top: 18rpx;
  gap: 12rpx;
}

.mini-btn,
.apply-btn {
  height: 64rpx;
  line-height: 64rpx;
  border-radius: 16rpx;
  font-size: 24rpx;
}

.mini-btn {
  width: 72rpx;
  color: #0f8f70;
  background: #e6f6f1;
}

.apply-btn {
  min-width: 110rpx;
  color: #ffffff;
  background: #0f8f70;
}

.threshold-input {
  flex: 1;
  height: 64rpx;
  padding: 0 18rpx;
  border-radius: 16rpx;
  background: #f6fafc;
  color: #102a43;
  font-size: 28rpx;
}
</style>
