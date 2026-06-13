package com.jbltech.kqzl3

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothSocket
import android.os.Build
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import android.content.pm.PackageManager
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.EventChannel
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import java.io.BufferedReader
import java.io.IOException
import java.io.InputStream
import java.io.InputStreamReader
import java.io.OutputStream
import java.nio.charset.StandardCharsets
import java.util.UUID
import java.util.concurrent.Executors

class MainActivity : FlutterActivity(), MethodChannel.MethodCallHandler, EventChannel.StreamHandler {
    private val methodChannelName = "com.jbltech.kqzl3/bluetooth_spp"
    private val eventChannelName = "com.jbltech.kqzl3/bluetooth_spp_events"
    private val defaultNamePrefix = "JDY"
    private val defaultSppUuid = "00001101-0000-1000-8000-00805F9B34FB"
    private val permissionRequestCode = 3131
    private val connectExecutor = Executors.newSingleThreadExecutor()
    private val writeExecutor = Executors.newSingleThreadExecutor()
    private val socketLock = Any()
    private var events: EventChannel.EventSink? = null
    private var pendingConnect: MethodChannel.Result? = null
    private var pendingArgs: Map<String, Any?> = emptyMap()
    private var socket: BluetoothSocket? = null
    private var inputStream: InputStream? = null
    private var outputStream: OutputStream? = null
    @Volatile private var connected = false
    @Volatile private var intentionalDisconnect = false

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, methodChannelName).setMethodCallHandler(this)
        EventChannel(flutterEngine.dartExecutor.binaryMessenger, eventChannelName).setStreamHandler(this)
    }

    override fun onListen(arguments: Any?, eventSink: EventChannel.EventSink?) {
        events = eventSink
    }

    override fun onCancel(arguments: Any?) {
        events = null
    }

    override fun onMethodCall(call: MethodCall, result: MethodChannel.Result) {
        when (call.method) {
            "connect" -> connect(call, result)
            "write" -> write(call, result)
            "disconnect" -> {
                closeConnection(true)
                result.success(mapOf("connected" to false))
            }
            "isConnected" -> result.success(mapOf("connected" to connected))
            else -> result.notImplemented()
        }
    }

    private fun connect(call: MethodCall, result: MethodChannel.Result) {
        val args = call.arguments as? Map<String, Any?> ?: emptyMap()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S && ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            pendingConnect = result
            pendingArgs = args
            ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.BLUETOOTH_CONNECT), permissionRequestCode)
            return
        }
        connectWithPermission(args, result)
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode != permissionRequestCode) return
        val result = pendingConnect ?: return
        val args = pendingArgs
        pendingConnect = null
        pendingArgs = emptyMap()
        if (grantResults.isEmpty() || grantResults[0] != PackageManager.PERMISSION_GRANTED) {
            result.error("BLUETOOTH_PERMISSION", "未授予蓝牙连接权限", null)
            return
        }
        connectWithPermission(args, result)
    }

    private fun write(call: MethodCall, result: MethodChannel.Result) {
        val data = call.argument<String>("data") ?: ""
        writeExecutor.execute {
            val stream = synchronized(socketLock) {
                if (!connected || outputStream == null) {
                    runOnUiThread { result.error("BLUETOOTH_DISCONNECTED", "蓝牙未连接", null) }
                    return@execute
                }
                outputStream
            }
            try {
                stream!!.write(data.toByteArray(StandardCharsets.UTF_8))
                stream.flush()
                runOnUiThread { result.success(mapOf("written" to true)) }
            } catch (error: IOException) {
                emitError(error.readableMessage())
                closeConnection(false)
                runOnUiThread { result.error("BLUETOOTH_WRITE_FAILED", "蓝牙发送失败: ${error.readableMessage()}", null) }
            }
        }
    }

    private fun connectWithPermission(args: Map<String, Any?>, result: MethodChannel.Result) {
        val namePrefix = args["namePrefix"] as? String ?: defaultNamePrefix
        val address = args["address"] as? String
        val uuidText = args["uuid"] as? String ?: defaultSppUuid
        connectExecutor.execute {
            emitStatus("connecting")
            val adapter = BluetoothAdapter.getDefaultAdapter()
            if (adapter == null) {
                runOnUiThread { result.error("BLUETOOTH_UNSUPPORTED", "当前设备不支持蓝牙", null) }
                emitStatus("disconnected")
                return@execute
            }
            if (!adapter.isEnabled) {
                runOnUiThread { result.error("BLUETOOTH_DISABLED", "手机蓝牙未开启", null) }
                emitStatus("disconnected")
                return@execute
            }
            try {
                val device = selectDevice(adapter, address, namePrefix)
                if (device == null) {
                    runOnUiThread { result.error("BLUETOOTH_DEVICE_NOT_FOUND", "未找到已配对的 JDY 蓝牙设备，请先在系统蓝牙设置中配对 JDY-31", null) }
                    emitStatus("disconnected")
                    return@execute
                }
                val uuid = UUID.fromString(uuidText)
                if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) adapter.cancelDiscovery()
                val nextSocket = createConnectedSocket(device, uuid)
                val nextInput = nextSocket.inputStream
                val nextOutput = nextSocket.outputStream
                synchronized(socketLock) {
                    closeSocketFields()
                    socket = nextSocket
                    inputStream = nextInput
                    outputStream = nextOutput
                    connected = true
                    intentionalDisconnect = false
                }
                runOnUiThread { result.success(mapOf("connected" to true, "device" to mapOf("name" to device.name, "address" to device.address))) }
                emitStatus("connected")
                readLoop(nextInput)
            } catch (error: IllegalArgumentException) {
                runOnUiThread { result.error("BLUETOOTH_UUID", "无效的蓝牙 UUID", null) }
                emitStatus("disconnected")
            } catch (error: Exception) {
                closeConnection(false)
                runOnUiThread { result.error("BLUETOOTH_CONNECT_FAILED", "蓝牙连接失败: ${error.readableMessage()}", null) }
            }
        }
    }

    @SuppressLint("MissingPermission")
    private fun selectDevice(adapter: BluetoothAdapter, address: String?, namePrefix: String): BluetoothDevice? {
        if (!address.isNullOrBlank()) return adapter.getRemoteDevice(address)
        val prefix = namePrefix.ifBlank { defaultNamePrefix }
        return adapter.bondedDevices?.firstOrNull { device ->
            device.name?.contains(prefix, ignoreCase = true) == true
        }
    }

    @SuppressLint("MissingPermission")
    private fun createConnectedSocket(device: BluetoothDevice, uuid: UUID): BluetoothSocket {
        val secureSocket = device.createRfcommSocketToServiceRecord(uuid)
        return try {
            secureSocket.connect()
            secureSocket
        } catch (secureError: IOException) {
            secureSocket.closeQuietly()
            val insecureSocket = device.createInsecureRfcommSocketToServiceRecord(uuid)
            try {
                insecureSocket.connect()
                insecureSocket
            } catch (insecureError: IOException) {
                insecureSocket.closeQuietly()
                throw insecureError
            }
        }
    }

    private fun readLoop(stream: InputStream) {
        try {
            BufferedReader(InputStreamReader(stream, StandardCharsets.UTF_8)).use { reader ->
                while (connected) {
                    val line = reader.readLine() ?: break
                    emitData(line)
                }
            }
        } catch (error: IOException) {
            if (!intentionalDisconnect) emitError(error.readableMessage())
        } finally {
            if (!intentionalDisconnect) closeConnection(false)
        }
    }

    private fun closeConnection(intentional: Boolean) {
        intentionalDisconnect = intentional
        synchronized(socketLock) {
            connected = false
            closeSocketFields()
        }
        emitStatus("disconnected")
    }

    private fun closeSocketFields() {
        inputStream.closeQuietly()
        outputStream.closeQuietly()
        socket.closeQuietly()
        inputStream = null
        outputStream = null
        socket = null
    }

    private fun emitStatus(state: String) {
        notifyEvent(mapOf("type" to "status", "state" to state))
    }

    private fun emitData(data: String) {
        notifyEvent(mapOf("type" to "data", "data" to data))
    }

    private fun emitError(message: String) {
        notifyEvent(mapOf("type" to "error", "message" to message))
    }

    private fun notifyEvent(payload: Map<String, Any>) {
        runOnUiThread { events?.success(payload) }
    }

    override fun onDestroy() {
        closeConnection(true)
        connectExecutor.shutdownNow()
        writeExecutor.shutdownNow()
        super.onDestroy()
    }
}

private fun AutoCloseable?.closeQuietly() {
    try {
        this?.close()
    } catch (_: Exception) {
    }
}

private fun Exception.readableMessage(): String = message?.takeIf { it.isNotBlank() } ?: javaClass.simpleName
