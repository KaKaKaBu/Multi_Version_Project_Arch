#include "bluetooth_spp_channel.h"

#include <devguid.h>
#include <setupapi.h>
#include <windows.h>

#include <atomic>
#include <cctype>
#include <cstdint>
#include <cwctype>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

#include <flutter/encodable_value.h>
#include <flutter/event_channel.h>
#include <flutter/event_sink.h>
#include <flutter/event_stream_handler_functions.h>
#include <flutter/method_channel.h>
#include <flutter/standard_method_codec.h>

namespace {

using flutter::EncodableMap;
using flutter::EncodableValue;

std::wstring ToWide(const std::string& value) {
  if (value.empty()) {
    return L"";
  }
  const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
  if (size <= 0) {
    return L"";
  }
  std::wstring result(static_cast<size_t>(size - 1), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, result.data(), size);
  return result;
}

std::string ToUtf8(const std::wstring& value) {
  if (value.empty()) {
    return "";
  }
  const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
  if (size <= 0) {
    return "";
  }
  std::string result(static_cast<size_t>(size - 1), '\0');
  WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), size, nullptr, nullptr);
  return result;
}

std::wstring ToLower(std::wstring value) {
  for (auto& ch : value) {
    ch = static_cast<wchar_t>(towlower(ch));
  }
  return value;
}

bool ContainsInsensitive(const std::wstring& text, const std::wstring& needle) {
  if (needle.empty()) {
    return true;
  }
  return ToLower(text).find(ToLower(needle)) != std::wstring::npos;
}

std::wstring ExtractComPort(const std::wstring& text) {
  for (size_t i = 0; i + 3 < text.size(); ++i) {
    if (towupper(text[i]) != L'C' || towupper(text[i + 1]) != L'O' ||
        towupper(text[i + 2]) != L'M' || !iswdigit(text[i + 3])) {
      continue;
    }
    size_t end = i + 4;
    while (end < text.size() && iswdigit(text[end])) {
      ++end;
    }
    return text.substr(i, end - i);
  }
  return L"";
}

std::string LastErrorMessage() {
  const DWORD error = GetLastError();
  if (error == ERROR_SUCCESS) {
    return "Windows API error";
  }

  LPWSTR buffer = nullptr;
  const DWORD size = FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
  if (size == 0 || buffer == nullptr) {
    return "Windows API error " + std::to_string(error);
  }

  std::wstring message(buffer, size);
  LocalFree(buffer);
  while (!message.empty() &&
         (message.back() == L'\r' || message.back() == L'\n' || message.back() == L' ')) {
    message.pop_back();
  }
  return ToUtf8(message);
}

std::string GetStringArg(const EncodableMap* args, const char* key, const char* default_value) {
  if (args == nullptr) {
    return default_value;
  }
  const auto it = args->find(EncodableValue(key));
  if (it == args->end()) {
    return default_value;
  }
  const auto* value = std::get_if<std::string>(&it->second);
  if (value == nullptr) {
    return default_value;
  }
  return *value;
}

int GetIntArg(const EncodableMap* args, const char* key, int default_value) {
  if (args == nullptr) {
    return default_value;
  }
  const auto it = args->find(EncodableValue(key));
  if (it == args->end()) {
    return default_value;
  }
  if (const auto* value = std::get_if<int>(&it->second)) {
    return *value;
  }
  if (const auto* value = std::get_if<int64_t>(&it->second)) {
    return static_cast<int>(*value);
  }
  return default_value;
}

std::wstring FindComPortByPrefix(const std::wstring& name_prefix) {
  HDEVINFO devices = SetupDiGetClassDevsW(&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT);
  if (devices == INVALID_HANDLE_VALUE) {
    return L"";
  }

  std::wstring matched_port;
  SP_DEVINFO_DATA data{};
  data.cbSize = sizeof(data);
  for (DWORD index = 0; SetupDiEnumDeviceInfo(devices, index, &data); ++index) {
    wchar_t friendly_name[256] = {};
    wchar_t description[256] = {};
    SetupDiGetDeviceRegistryPropertyW(devices, &data, SPDRP_FRIENDLYNAME, nullptr,
                                      reinterpret_cast<PBYTE>(friendly_name),
                                      sizeof(friendly_name), nullptr);
    SetupDiGetDeviceRegistryPropertyW(devices, &data, SPDRP_DEVICEDESC, nullptr,
                                      reinterpret_cast<PBYTE>(description),
                                      sizeof(description), nullptr);

    const std::wstring friendly(friendly_name);
    const std::wstring desc(description);
    const std::wstring text = friendly + L" " + desc;
    const std::wstring port = ExtractComPort(text);
    if (port.empty()) {
      continue;
    }
    if (ContainsInsensitive(text, name_prefix)) {
      matched_port = port;
      break;
    }
    if (matched_port.empty() && name_prefix.empty()) {
      matched_port = port;
    }
  }

  SetupDiDestroyDeviceInfoList(devices);
  return matched_port;
}

std::string TrimLine(std::string value) {
  while (!value.empty() && (value.back() == '\r' || value.back() == '\n' ||
                            value.back() == ' ' || value.back() == '\t')) {
    value.pop_back();
  }
  size_t start = 0;
  while (start < value.size() &&
         (value[start] == '\r' || value[start] == '\n' || value[start] == ' ' ||
          value[start] == '\t')) {
    ++start;
  }
  return value.substr(start);
}

class BluetoothSppChannel {
 public:
  BluetoothSppChannel(flutter::FlutterEngine* engine, std::string method_name,
                      std::string event_name)
      : method_name_(std::move(method_name)), event_name_(std::move(event_name)) {
    const auto* codec = &flutter::StandardMethodCodec::GetInstance();
    method_channel_ =
        std::make_unique<flutter::MethodChannel<EncodableValue>>(engine->messenger(), method_name_, codec);
    method_channel_->SetMethodCallHandler(
        [this](const flutter::MethodCall<EncodableValue>& call,
               std::unique_ptr<flutter::MethodResult<EncodableValue>> result) {
          HandleMethodCall(call, std::move(result));
        });

    event_channel_ =
        std::make_unique<flutter::EventChannel<EncodableValue>>(engine->messenger(), event_name_, codec);
    event_channel_->SetStreamHandler(
        std::make_unique<flutter::StreamHandlerFunctions<EncodableValue>>(
            [this](const EncodableValue*,
                   std::unique_ptr<flutter::EventSink<EncodableValue>>&& events)
                -> std::unique_ptr<flutter::StreamHandlerError<EncodableValue>> {
              std::lock_guard<std::mutex> lock(event_mutex_);
              event_sink_ = std::move(events);
              return nullptr;
            },
            [this](const EncodableValue*)
                -> std::unique_ptr<flutter::StreamHandlerError<EncodableValue>> {
              std::lock_guard<std::mutex> lock(event_mutex_);
              event_sink_.reset();
              return nullptr;
            }));
  }

  ~BluetoothSppChannel() {
    CloseConnection(true);
  }

 private:
  void HandleMethodCall(const flutter::MethodCall<EncodableValue>& call,
                        std::unique_ptr<flutter::MethodResult<EncodableValue>> result) {
    if (call.method_name() == "connect") {
      Connect(call, std::move(result));
    } else if (call.method_name() == "write") {
      Write(call, std::move(result));
    } else if (call.method_name() == "disconnect") {
      CloseConnection(true);
      result->Success(EncodableValue(EncodableMap{
          {EncodableValue("connected"), EncodableValue(false)},
      }));
    } else if (call.method_name() == "isConnected") {
      result->Success(EncodableValue(EncodableMap{
          {EncodableValue("connected"), EncodableValue(connected_.load())},
      }));
    } else {
      result->NotImplemented();
    }
  }

  void Connect(const flutter::MethodCall<EncodableValue>& call,
               std::unique_ptr<flutter::MethodResult<EncodableValue>> result) {
    const auto* args = std::get_if<EncodableMap>(call.arguments());
    const std::string address = GetStringArg(args, "address", "");
    const std::string configured_port = GetStringArg(args, "port", "");
    const std::string name_prefix = GetStringArg(args, "namePrefix", "JDY");
    const int baud_rate = GetIntArg(args, "baudRate", 9600);

    std::wstring port = ExtractComPort(ToWide(configured_port));
    if (port.empty()) {
      port = ExtractComPort(ToWide(address));
    }
    if (port.empty()) {
      port = FindComPortByPrefix(ToWide(name_prefix));
    }
    if (port.empty()) {
      EmitStatus("disconnected");
      result->Error("BLUETOOTH_DEVICE_NOT_FOUND",
                    "未找到已配对蓝牙串口。请先在 Windows 蓝牙设置中配对设备，并在输入框填写出站 COM 口，例如 COM5。");
      return;
    }

    CloseConnection(true);
    const std::wstring path = L"\\\\.\\" + port;
    HANDLE handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
      EmitStatus("disconnected");
      result->Error("BLUETOOTH_CONNECT_FAILED",
                    "打开 " + ToUtf8(port) + " 失败: " + LastErrorMessage());
      return;
    }

    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(handle, &dcb)) {
      CloseHandle(handle);
      EmitStatus("disconnected");
      result->Error("BLUETOOTH_CONNECT_FAILED", "读取串口参数失败: " + LastErrorMessage());
      return;
    }
    dcb.BaudRate = static_cast<DWORD>(baud_rate);
    dcb.fBinary = TRUE;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    if (!SetCommState(handle, &dcb)) {
      CloseHandle(handle);
      EmitStatus("disconnected");
      result->Error("BLUETOOTH_CONNECT_FAILED", "设置串口参数失败: " + LastErrorMessage());
      return;
    }

    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = 20;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 5;
    timeouts.WriteTotalTimeoutConstant = 500;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(handle, &timeouts);
    PurgeComm(handle, PURGE_RXCLEAR | PURGE_TXCLEAR);

    {
      std::lock_guard<std::mutex> lock(handle_mutex_);
      serial_handle_ = handle;
      connected_ = true;
      intentional_disconnect_ = false;
    }
    read_thread_ = std::thread([this]() { ReadLoop(); });
    EmitStatus("connected");
    result->Success(EncodableValue(EncodableMap{
        {EncodableValue("connected"), EncodableValue(true)},
        {EncodableValue("port"), EncodableValue(ToUtf8(port))},
    }));
  }

  void Write(const flutter::MethodCall<EncodableValue>& call,
             std::unique_ptr<flutter::MethodResult<EncodableValue>> result) {
    const auto* args = std::get_if<EncodableMap>(call.arguments());
    const std::string data = GetStringArg(args, "data", "");
    HANDLE handle = INVALID_HANDLE_VALUE;
    {
      std::lock_guard<std::mutex> lock(handle_mutex_);
      handle = serial_handle_;
    }
    if (!connected_.load() || handle == INVALID_HANDLE_VALUE) {
      result->Error("BLUETOOTH_DISCONNECTED", "蓝牙串口未连接");
      return;
    }

    DWORD written = 0;
    const BOOL ok = WriteFile(handle, data.data(), static_cast<DWORD>(data.size()), &written, nullptr);
    if (!ok || written != static_cast<DWORD>(data.size())) {
      EmitError("蓝牙发送失败: " + LastErrorMessage());
      result->Error("BLUETOOTH_WRITE_FAILED", "蓝牙发送失败: " + LastErrorMessage());
      CloseConnection(false);
      return;
    }
    result->Success(EncodableValue(EncodableMap{
        {EncodableValue("written"), EncodableValue(true)},
    }));
  }

  void ReadLoop() {
    std::string buffer;
    char chunk[128] = {};
    while (connected_.load()) {
      HANDLE handle = INVALID_HANDLE_VALUE;
      {
        std::lock_guard<std::mutex> lock(handle_mutex_);
        handle = serial_handle_;
      }
      if (handle == INVALID_HANDLE_VALUE) {
        break;
      }

      DWORD read = 0;
      const BOOL ok = ReadFile(handle, chunk, sizeof(chunk), &read, nullptr);
      if (!ok) {
        if (!intentional_disconnect_.load()) {
          EmitError("蓝牙接收失败: " + LastErrorMessage());
        }
        break;
      }
      if (read == 0) {
        Sleep(10);
        continue;
      }

      buffer.append(chunk, chunk + read);
      size_t newline = buffer.find('\n');
      while (newline != std::string::npos) {
        const std::string line = TrimLine(buffer.substr(0, newline));
        buffer.erase(0, newline + 1);
        if (!line.empty()) {
          EmitData(line);
        }
        newline = buffer.find('\n');
      }
    }

    if (!intentional_disconnect_.load()) {
      CloseConnection(false);
    }
  }

  void CloseConnection(bool intentional) {
    intentional_disconnect_ = intentional;
    connected_ = false;
    HANDLE handle = INVALID_HANDLE_VALUE;
    {
      std::lock_guard<std::mutex> lock(handle_mutex_);
      handle = serial_handle_;
      serial_handle_ = INVALID_HANDLE_VALUE;
    }
    if (handle != INVALID_HANDLE_VALUE) {
      CloseHandle(handle);
    }
    if (read_thread_.joinable() && read_thread_.get_id() != std::this_thread::get_id()) {
      read_thread_.join();
    }
    EmitStatus("disconnected");
  }

  void EmitStatus(const std::string& state) {
    EmitEvent(EncodableMap{
        {EncodableValue("type"), EncodableValue("status")},
        {EncodableValue("state"), EncodableValue(state)},
    });
  }

  void EmitData(const std::string& data) {
    EmitEvent(EncodableMap{
        {EncodableValue("type"), EncodableValue("data")},
        {EncodableValue("data"), EncodableValue(data)},
    });
  }

  void EmitError(const std::string& message) {
    EmitEvent(EncodableMap{
        {EncodableValue("type"), EncodableValue("error")},
        {EncodableValue("message"), EncodableValue(message)},
    });
  }

  void EmitEvent(const EncodableMap& event) {
    std::lock_guard<std::mutex> lock(event_mutex_);
    if (event_sink_) {
      event_sink_->Success(EncodableValue(event));
    }
  }

  std::string method_name_;
  std::string event_name_;
  std::unique_ptr<flutter::MethodChannel<EncodableValue>> method_channel_;
  std::unique_ptr<flutter::EventChannel<EncodableValue>> event_channel_;
  std::unique_ptr<flutter::EventSink<EncodableValue>> event_sink_;
  std::mutex event_mutex_;
  std::mutex handle_mutex_;
  HANDLE serial_handle_ = INVALID_HANDLE_VALUE;
  std::thread read_thread_;
  std::atomic_bool connected_{false};
  std::atomic_bool intentional_disconnect_{false};
};

std::vector<std::shared_ptr<BluetoothSppChannel>>& ChannelInstances() {
  static std::vector<std::shared_ptr<BluetoothSppChannel>> instances;
  return instances;
}

void RegisterChannel(flutter::FlutterEngine* engine, const std::string& method_name,
                     const std::string& event_name) {
  ChannelInstances().push_back(std::make_shared<BluetoothSppChannel>(engine, method_name, event_name));
}

}  // namespace

void RegisterBluetoothSppChannels(flutter::FlutterEngine* engine) {
  RegisterChannel(engine, "com.jbltech.common/bluetooth_spp",
                  "com.jbltech.common/bluetooth_spp_events");
  RegisterChannel(engine, "com.jbltech.dcld001/bluetooth_spp",
                  "com.jbltech.dcld001/bluetooth_spp_events");
}
