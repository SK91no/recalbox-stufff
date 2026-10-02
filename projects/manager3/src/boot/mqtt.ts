/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import mqtt from 'mqtt';
import { useEmulationstationStore } from 'stores/configuration/emulationstation';
import { useMonitoringStore } from 'stores/common/monitoring';
import { Actions, EventResponse, MonitoringResponse } from 'stores/common/monitoring.d';

const options: { clientId: string } = {
  clientId: `mqttjs_${Math.random().toString(16).substring(2, 8)}`,
};

let mqttUrl = process.env.MQTT_URL;

if (process.env.MQTT_URL === '') {
  mqttUrl = `mqtt://${window.location.hostname}:18833`;
}

const client = mqtt.connect(String(mqttUrl), options);
const monitoringStore = useMonitoringStore();
const emulationStationStore = useEmulationstationStore();

// Subcriptions and VueJS store injection
client.on('connect', () => {
  emulationStationStore.fetchStatus();
  client.subscribe(String(process.env.MQTT_MONITORING_CHANNEL));
  client.subscribe(String(process.env.MQTT_ES_EVENTS_CHANNEL));
  client.subscribe(String(process.env.MQTT_VD_EVENTS_KEYBOARD_CHANNEL));
});

client.on('message', (topic, message): void => {
  if (topic === process.env.MQTT_MONITORING_CHANNEL) {
    const newMessage: MonitoringResponse = JSON.parse(new TextDecoder('utf-8').decode(message));
    monitoringStore.updateMonitoring(newMessage);
  }

  if (topic === process.env.MQTT_ES_EVENTS_CHANNEL) {
    const newMessage: EventResponse = JSON.parse(new TextDecoder('utf-8').decode(message));

    if (newMessage.Action === Actions.reboot) {
      emulationStationStore.resetCurrentSystem();
    } else if ([
      Actions.systemBrowsing,
      Actions.gamelistBrowsing,
      Actions.runGame,
      Actions.endGame,
      Actions.start,
      Actions.wakeup,
      Actions.sleep,
      Actions.runDemo,
      Actions.startGameClip,
      Actions.runKodi,
    ].includes(newMessage.Action)) {
      emulationStationStore.updateStatus(newMessage);
    }
  }
});

export default client;
