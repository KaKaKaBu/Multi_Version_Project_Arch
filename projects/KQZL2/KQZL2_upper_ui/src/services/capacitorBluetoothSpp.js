import { Capacitor, registerPlugin } from '@capacitor/core'

const BluetoothSpp = registerPlugin('BluetoothSpp')
const SPP_UUID = '00001101-0000-1000-8000-00805F9B34FB'

export function isCapacitorAndroid() {
  return Capacitor.getPlatform() === 'android' &&
    (typeof Capacitor.isNativePlatform !== 'function' || Capacitor.isNativePlatform())
}

export function createBluetoothSppClient(options = {}) {
  const { onTelemetry = () => {}, onStatus = () => {}, onError = () => {} } = options
  let connected = false
  let listenerHandles = []

  async function clearListeners() {
    for (const handle of listenerHandles) {
      try {
        if (handle && typeof handle.remove === 'function') {
          await handle.remove()
        }
      } catch (error) {
        onError(error)
      }
    }
    listenerHandles = []
  }

  async function connect(config = {}) {
    await clearListeners()
    onStatus('connecting')

    listenerHandles = [
      await BluetoothSpp.addListener('data', ({ data }) => {
        onTelemetry(data)
      }),
      await BluetoothSpp.addListener('status', ({ state }) => {
        onStatus(state)
      }),
      await BluetoothSpp.addListener('error', (error) => {
        onError(error)
      }),
    ]

    try {
      const result = await BluetoothSpp.connect({
        namePrefix: config.namePrefix || 'JDY',
        address: config.address,
        uuid: config.uuid || SPP_UUID,
      })
      connected = true
      onStatus('connected')
      return result.device
    } catch (error) {
      await dispose()
      throw error
    }
  }

  async function publishCommand(command) {
    if (!connected) {
      throw new Error('蓝牙未连接')
    }

    const data = `${typeof command === 'string' ? command : JSON.stringify(command)}\n`
    await BluetoothSpp.write({ data })
  }

  async function dispose() {
    connected = false
    await clearListeners()
    try {
      await BluetoothSpp.disconnect()
    } catch (error) {
      onError(error)
    }
    onStatus('disconnected')
  }

  return { connect, publishCommand, dispose }
}
