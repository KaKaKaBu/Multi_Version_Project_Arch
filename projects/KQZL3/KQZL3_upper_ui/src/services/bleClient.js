import { createBluetoothSppClient, isCapacitorAndroid } from './capacitorBluetoothSpp'
import { stringToArrayBuffer } from './kqzl3Protocol'

function failUnsupported() {
  return Promise.reject(new Error('当前平台暂不支持 JDY-31 蓝牙连接，请在 App 真机环境下使用'))
}

export function createBleClient(options = {}) {
  if (isCapacitorAndroid()) {
    return createBluetoothSppClient(options)
  }

  const { onTelemetry = () => {}, onStatus = () => {}, onError = () => {} } = options
  let connected = false
  let deviceId = ''
  let serviceId = ''
  let characteristicId = ''

  function ensureUniBleApi() {
    return typeof uni !== 'undefined' && typeof uni.openBluetoothAdapter === 'function'
  }

  function connect(config = {}) {
    if (!ensureUniBleApi()) {
      return failUnsupported()
    }

    onStatus('scanning')
    return new Promise((resolve, reject) => {
      uni.openBluetoothAdapter({
        success() {
          uni.startBluetoothDevicesDiscovery({
            success() {
              setTimeout(() => {
                uni.getBluetoothDevices({
                  success(res) {
                    const target = (res.devices || []).find((item) => {
                      const name = item.name || item.localName || ''
                      return name.includes(config.namePrefix || 'JDY')
                    })
                    if (!target) {
                      onStatus('disconnected')
                      reject(new Error('未发现 JDY-31 蓝牙设备'))
                      return
                    }
                    deviceId = target.deviceId
                    uni.createBLEConnection({
                      deviceId,
                      success() {
                        connected = true
                        onStatus('connected')
                        resolve(target)
                      },
                      fail(error) {
                        onError(error)
                        reject(error)
                      },
                    })
                  },
                  fail(error) {
                    onError(error)
                    reject(error)
                  },
                })
              }, 1200)
            },
            fail(error) {
              onError(error)
              reject(error)
            },
          })
        },
        fail(error) {
          onError(error)
          reject(error)
        },
      })
    })
  }

  function publishCommand(command) {
    if (!connected || !deviceId || !serviceId || !characteristicId) {
      throw new Error('蓝牙未完成连接或特征值未配置')
    }
    const text = `${typeof command === 'string' ? command : JSON.stringify(command)}\n`
    uni.writeBLECharacteristicValue({
      deviceId,
      serviceId,
      characteristicId,
      value: stringToArrayBuffer(text),
      fail: onError,
    })
  }

  function attachCharacteristic(ids) {
    serviceId = ids.serviceId
    characteristicId = ids.characteristicId
    uni.notifyBLECharacteristicValueChange({
      deviceId,
      serviceId,
      characteristicId,
      state: true,
      fail: onError,
    })
    uni.onBLECharacteristicValueChange((event) => {
      onTelemetry(event.value)
    })
  }

  function dispose() {
    if (deviceId && typeof uni !== 'undefined' && typeof uni.closeBLEConnection === 'function') {
      uni.closeBLEConnection({ deviceId })
    }
    connected = false
    onStatus('disconnected')
  }

  return { connect, attachCharacteristic, publishCommand, dispose }
}
