package com.jbltech.kqzl3;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.os.Build;
import com.getcapacitor.JSObject;
import com.getcapacitor.PermissionState;
import com.getcapacitor.Plugin;
import com.getcapacitor.PluginCall;
import com.getcapacitor.PluginMethod;
import com.getcapacitor.annotation.CapacitorPlugin;
import com.getcapacitor.annotation.Permission;
import com.getcapacitor.annotation.PermissionCallback;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

@CapacitorPlugin(
    name = "BluetoothSpp",
    permissions = {
        @Permission(alias = "bluetoothConnect", strings = { Manifest.permission.BLUETOOTH_CONNECT })
    }
)
public class BluetoothSppPlugin extends Plugin {

    static final String BLUETOOTH_CONNECT_ALIAS = "bluetoothConnect";
    private static final String DEFAULT_NAME_PREFIX = "JDY";
    private static final String DEFAULT_SPP_UUID = "00001101-0000-1000-8000-00805F9B34FB";

    private final ExecutorService connectExecutor = Executors.newSingleThreadExecutor();
    private final ExecutorService writeExecutor = Executors.newSingleThreadExecutor();
    private final Object socketLock = new Object();

    private BluetoothSocket socket;
    private InputStream inputStream;
    private OutputStream outputStream;
    private volatile boolean connected;
    private volatile boolean intentionalDisconnect;

    @PluginMethod
    public void connect(PluginCall call) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S && getPermissionState(BLUETOOTH_CONNECT_ALIAS) != PermissionState.GRANTED) {
            requestPermissionForAlias(BLUETOOTH_CONNECT_ALIAS, call, "bluetoothConnectPermissionCallback");
            return;
        }

        connectWithPermission(call);
    }

    @PermissionCallback
    private void bluetoothConnectPermissionCallback(PluginCall call) {
        if (call == null) {
            return;
        }
        if (getPermissionState(BLUETOOTH_CONNECT_ALIAS) != PermissionState.GRANTED) {
            call.reject("未授予蓝牙连接权限");
            return;
        }
        connectWithPermission(call);
    }

    @PluginMethod
    public void write(PluginCall call) {
        String data = call.getString("data", "");
        writeExecutor.execute(() -> {
            OutputStream stream;
            synchronized (socketLock) {
                if (!connected || outputStream == null) {
                    rejectOnMain(call, "蓝牙未连接");
                    return;
                }
                stream = outputStream;
            }

            try {
                stream.write(data.getBytes(StandardCharsets.UTF_8));
                stream.flush();
                resolveOnMain(call, new JSObject().put("written", true));
            } catch (IOException error) {
                emitError(error.getMessage());
                closeConnection(false);
                rejectOnMain(call, "蓝牙发送失败: " + readableMessage(error));
            }
        });
    }

    @PluginMethod
    public void disconnect(PluginCall call) {
        closeConnection(true);
        call.resolve(new JSObject().put("connected", false));
    }

    @PluginMethod
    public void isConnected(PluginCall call) {
        call.resolve(new JSObject().put("connected", connected));
    }

    @Override
    protected void handleOnDestroy() {
        closeConnection(true);
        connectExecutor.shutdownNow();
        writeExecutor.shutdownNow();
        super.handleOnDestroy();
    }

    private void connectWithPermission(PluginCall call) {
        String namePrefix = call.getString("namePrefix", DEFAULT_NAME_PREFIX);
        String address = call.getString("address");
        String uuidText = call.getString("uuid", DEFAULT_SPP_UUID);

        connectExecutor.execute(() -> {
            emitStatus("connecting");

            BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
            if (adapter == null) {
                rejectOnMain(call, "当前设备不支持蓝牙");
                emitStatus("disconnected");
                return;
            }
            if (!adapter.isEnabled()) {
                rejectOnMain(call, "手机蓝牙未开启");
                emitStatus("disconnected");
                return;
            }

            try {
                BluetoothDevice device = selectDevice(adapter, address, namePrefix);
                if (device == null) {
                    rejectOnMain(call, "未找到已配对的 JDY 蓝牙设备，请先在系统蓝牙设置中配对 JDY-31");
                    emitStatus("disconnected");
                    return;
                }

                UUID uuid = UUID.fromString(uuidText);
                if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
                    adapter.cancelDiscovery();
                }
                BluetoothSocket nextSocket = createConnectedSocket(device, uuid);
                InputStream nextInput = nextSocket.getInputStream();
                OutputStream nextOutput = nextSocket.getOutputStream();

                synchronized (socketLock) {
                    closeSocketFields();
                    socket = nextSocket;
                    inputStream = nextInput;
                    outputStream = nextOutput;
                    connected = true;
                    intentionalDisconnect = false;
                }

                JSObject deviceInfo = new JSObject()
                    .put("name", device.getName())
                    .put("address", device.getAddress());
                resolveOnMain(call, new JSObject().put("connected", true).put("device", deviceInfo));
                emitStatus("connected");
                readLoop(nextInput);
            } catch (IllegalArgumentException error) {
                rejectOnMain(call, "无效的蓝牙 UUID");
                emitStatus("disconnected");
            } catch (IOException | SecurityException error) {
                closeConnection(false);
                rejectOnMain(call, "蓝牙连接失败: " + readableMessage(error));
            }
        });
    }

    @SuppressLint("MissingPermission")
    private BluetoothDevice selectDevice(BluetoothAdapter adapter, String address, String namePrefix) {
        if (address != null && !address.trim().isEmpty()) {
            return adapter.getRemoteDevice(address);
        }

        Set<BluetoothDevice> devices = adapter.getBondedDevices();
        if (devices == null || devices.isEmpty()) {
            return null;
        }

        String prefix = namePrefix == null || namePrefix.trim().isEmpty() ? DEFAULT_NAME_PREFIX : namePrefix.trim();
        for (BluetoothDevice device : devices) {
            String name = device.getName();
            if (name != null && name.toUpperCase().contains(prefix.toUpperCase())) {
                return device;
            }
        }
        return null;
    }

    @SuppressLint("MissingPermission")
    private BluetoothSocket createConnectedSocket(BluetoothDevice device, UUID uuid) throws IOException {
        BluetoothSocket secureSocket = device.createRfcommSocketToServiceRecord(uuid);
        try {
            secureSocket.connect();
            return secureSocket;
        } catch (IOException secureError) {
            closeQuietly(secureSocket);
            BluetoothSocket insecureSocket = device.createInsecureRfcommSocketToServiceRecord(uuid);
            try {
                insecureSocket.connect();
                return insecureSocket;
            } catch (IOException insecureError) {
                closeQuietly(insecureSocket);
                throw insecureError;
            }
        }
    }

    private void readLoop(InputStream stream) {
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(stream, StandardCharsets.UTF_8))) {
            String line;
            while (connected && (line = reader.readLine()) != null) {
                emitData(line);
            }
        } catch (IOException error) {
            if (!intentionalDisconnect) {
                emitError(readableMessage(error));
            }
        } finally {
            if (!intentionalDisconnect) {
                closeConnection(false);
            }
        }
    }

    private void closeConnection(boolean intentional) {
        intentionalDisconnect = intentional;
        synchronized (socketLock) {
            connected = false;
            closeSocketFields();
        }
        emitStatus("disconnected");
    }

    private void closeSocketFields() {
        closeQuietly(inputStream);
        closeQuietly(outputStream);
        closeQuietly(socket);
        inputStream = null;
        outputStream = null;
        socket = null;
    }

    private void closeQuietly(AutoCloseable closeable) {
        if (closeable == null) {
            return;
        }
        try {
            closeable.close();
        } catch (Exception ignored) {
        }
    }

    private void emitStatus(String state) {
        notifyOnMain("status", new JSObject().put("state", state));
    }

    private void emitData(String data) {
        notifyOnMain("data", new JSObject().put("data", data));
    }

    private void emitError(String message) {
        notifyOnMain("error", new JSObject().put("message", message == null ? "未知蓝牙错误" : message));
    }

    private void notifyOnMain(String eventName, JSObject data) {
        getActivity().runOnUiThread(() -> notifyListeners(eventName, data));
    }

    private void resolveOnMain(PluginCall call, JSObject data) {
        getActivity().runOnUiThread(() -> call.resolve(data));
    }

    private void rejectOnMain(PluginCall call, String message) {
        getActivity().runOnUiThread(() -> call.reject(message));
    }

    private String readableMessage(Exception error) {
        String message = error.getMessage();
        return message == null || message.trim().isEmpty() ? error.getClass().getSimpleName() : message;
    }
}
