// Auto-synced with projects/ZNCZ_001/app/board_config.h.
// Update BOARD_ESP8266_* macros first, then keep this file consistent.
export const boardMqttConfig = {
  brokerHost: "47.94.89.46",
  brokerPort: 1883,
  username: "yskj",
  password: "yskj@123",
  deviceClientId: "yskj_zncz_001",
  panelClientIdPrefix: "yskj_zncz_001_panel",
  commandTopic: "ZNCZ_001/web", // BOARD_ESP8266_MQTT_SUB_TOPIC
  telemetryTopic: "ZNCZ_001", // BOARD_ESP8266_MQTT_PUB_TOPIC
  keepalive: 60,
  useTLS: false,
  wsEndpoint: "ws://47.94.89.46:8083/mqtt",
};

export function buildClientId(suffix = "web") {
  const base = boardMqttConfig.panelClientIdPrefix || boardMqttConfig.deviceClientId;
  return `${base}_${suffix}_${Date.now().toString(16)}`;
}
