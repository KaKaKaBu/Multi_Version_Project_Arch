import mqtt from "mqtt";

function resolveUrl(options) {
  if (options.wsEndpoint) {
    return options.wsEndpoint;
  }
  const scheme = options.useTLS ? "mqtts" : "mqtt";
  return `${scheme}://${options.host}:${options.port}`;
}

export function createMqttClient(options) {
  const {
    clientId,
    username,
    password,
    telemetryTopic,
    commandTopic,
    keepalive = 60,
    onTelemetry = () => {},
    onStatus = () => {},
    onError = () => {},
    wsEndpoint = "",
    host,
    port,
    useTLS = false,
  } = options;

  const url = resolveUrl({ wsEndpoint, host, port, useTLS });
  const client = mqtt.connect(url, {
    clientId,
    username,
    password,
    keepalive,
    reconnectPeriod: 3000,
    clean: true,
  });

  onStatus("connecting");

  client.on("connect", () => {
    onStatus("connected");
    client.subscribe(telemetryTopic, { qos: 0 }, (err) => {
      if (err) {
        onError(err);
      }
    });
  });

  client.on("reconnect", () => onStatus("connecting"));
  client.on("close", () => onStatus("disconnected"));
  client.on("error", (err) => onError(err));

  client.on("message", (topic, payload) => {
    if (topic !== telemetryTopic) {
      return;
    }
    let parsed = payload;
    try {
      const text = typeof payload === "string" ? payload : payload.toString();
      parsed = JSON.parse(text);
    } catch (err) {
      parsed = { raw: payload.toString() };
    }
    onTelemetry(parsed);
  });

  function publishCommand(message) {
    if (!client.connected) {
      throw new Error("MQTT 未连接");
    }
    const text = typeof message === "string" ? message : JSON.stringify(message);
    client.publish(commandTopic, text, { qos: 0, retain: false });
  }

  function dispose() {
    client.end(true);
  }

  return { publishCommand, dispose, client };
}
