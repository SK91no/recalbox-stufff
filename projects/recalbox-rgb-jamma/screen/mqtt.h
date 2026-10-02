#include <mosquitto.h>

int mqtt_shutdown();
int mqtt_init(struct mosquitto **mosq,void (*on_message)(struct mosquitto *, void *, const struct mosquitto_message *));
int mqtt_reconnect(struct mosquitto *mosq);