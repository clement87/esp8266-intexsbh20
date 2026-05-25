#include "HADiscovery.h"
#include "common.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

static const char* HA_PREFIX = "homeassistant";
static const char* DEV_ID = "intex_spa_sbh20";
static const char* DEV_NAME = "Intex PureSpa";

static void addDevice(JsonObject doc)
{
  JsonObject dev = doc.createNestedObject("dev");
  dev["ids"] = DEV_ID;
  dev["name"] = DEV_NAME;
  dev["mf"] = "Intex";
  dev["mdl"] = "PureSpa SB-H20";
  dev["sw"] = CONFIG::WIFI_VERSION;
}

static void addAvailability(JsonObject doc)
{
  JsonArray avail = doc.createNestedArray("avty");
  JsonObject a = avail.createNestedObject();
  a["t"] = MQTT_TOPIC::STATE;
  a["pl_avail"] = "online";
  a["pl_not_avail"] = "offline";
}

static bool publishDoc(PubSubClient& client, const char* component, const char* objectId, DynamicJsonDocument& doc, char* buf, size_t bufSize)
{
  char topic[128];
  snprintf(topic, sizeof(topic), "%s/%s/%s/config", HA_PREFIX, component, objectId);
  size_t len = serializeJson(doc, buf, bufSize);
  bool ok = client.publish(topic, buf, true);
  Serial.printf("  %s (%u bytes): %s\n", topic, len, ok ? "OK" : "FAIL");
  yield();
  return ok;
}

void HADiscovery::publish(PubSubClient& client)
{
  Serial.println("HA Discovery: publishing...");
  if (!client.connected()) {
    Serial.println("HA Discovery: MQTT not connected!");
    return;
  }

  // Use heap for JSON doc and payload buffer
  DynamicJsonDocument doc(1024);
  char* buf = new char[1024];
  if (!buf) {
    Serial.println("HA Discovery: alloc failed!");
    return;
  }

  // ---- CLIMATE: thermostat ----
  {
    doc.clear();
    doc["name"] = "Chauffage Spa";
    doc["uniq_id"] = "spa_climate_heater";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["modes"][0] = "off";
    doc["modes"][1] = "heat";
    doc["mode_stat_t"] = MQTT_TOPIC::HEATER;
    doc["mode_stat_tpl"] = "{% set v={'off':'off','on':'heat','standby':'heat'} %}{{v[value]|default('off')}}";
    doc["mode_cmd_t"] = MQTT_TOPIC::CMD_HEATER;
    doc["mode_cmd_tpl"] = "{% set v={'heat':'on','off':'off'} %}{{v[value]|default('off')}}";
    doc["act_t"] = MQTT_TOPIC::HEATER;
    doc["act_tpl"] = "{% set v={'off':'off','on':'heating','standby':'idle'} %}{{v[value]|default('off')}}";
    doc["curr_temp_t"] = MQTT_TOPIC::WATER_ACT;
    doc["temp_stat_t"] = MQTT_TOPIC::WATER_SET;
    doc["temp_cmd_t"] = MQTT_TOPIC::CMD_WATER;
    doc["temp_step"] = 1;
    doc["min_temp"] = 20;
    doc["max_temp"] = 40;
    doc["temp_unit"] = "C";
    publishDoc(client, "climate", "spa_climate_heater", doc, buf, 1024);
  }

  // ---- SWITCH: power ----
  {
    doc.clear();
    doc["name"] = "Alimentation Spa";
    doc["uniq_id"] = "spa_switch_power";
    doc["icon"] = "mdi:power";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"] = MQTT_TOPIC::POWER;
    doc["cmd_t"] = MQTT_TOPIC::CMD_POWER;
    doc["pl_on"] = "on";
    doc["pl_off"] = "off";
    publishDoc(client, "switch", "spa_switch_power", doc, buf, 1024);
  }

  // ---- SWITCH: filter ----
  {
    doc.clear();
    doc["name"] = "Filtration Spa";
    doc["uniq_id"] = "spa_switch_filter";
    doc["icon"] = "mdi:air-filter";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"] = MQTT_TOPIC::FILTER;
    doc["cmd_t"] = MQTT_TOPIC::CMD_FILTER;
    doc["pl_on"] = "on";
    doc["pl_off"] = "off";
    publishDoc(client, "switch", "spa_switch_filter", doc, buf, 1024);
  }

  // ---- SWITCH: bubble ----
  {
    doc.clear();
    doc["name"] = "Bulles Spa";
    doc["uniq_id"] = "spa_switch_bubble";
    doc["icon"] = "mdi:chart-bubble";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"] = MQTT_TOPIC::BUBBLE;
    doc["cmd_t"] = MQTT_TOPIC::CMD_BUBBLE;
    doc["pl_on"] = "on";
    doc["pl_off"] = "off";
    publishDoc(client, "switch", "spa_switch_bubble", doc, buf, 1024);
  }

  // ---- BINARY SENSOR: heater active ----
  {
    doc.clear();
    doc["name"] = "Chauffage Spa Actif";
    doc["uniq_id"] = "spa_binary_heater";
    doc["dev_cla"] = "heat";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"] = MQTT_TOPIC::HEATER;
    doc["pl_on"] = "on";
    doc["pl_off"] = "standby";
    publishDoc(client, "binary_sensor", "spa_binary_heater", doc, buf, 1024);
  }

  // ---- SENSOR: water temperature ----
  {
    doc.clear();
    doc["name"] = "Temperature Eau Spa";
    doc["uniq_id"] = "spa_sensor_temp_act";
    doc["icon"] = "mdi:thermometer-water";
    doc["dev_cla"] = "temperature";
    doc["stat_cla"] = "measurement";
    doc["unit_of_meas"] = "\u00B0C";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"] = MQTT_TOPIC::WATER_ACT;
    publishDoc(client, "sensor", "spa_sensor_temp_act", doc, buf, 1024);
  }

  // ---- SENSOR: target temperature ----
  {
    doc.clear();
    doc["name"] = "Temperature Cible Spa";
    doc["uniq_id"] = "spa_sensor_temp_set";
    doc["icon"] = "mdi:thermometer-water";
    doc["dev_cla"] = "temperature";
    doc["unit_of_meas"] = "\u00B0C";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"] = MQTT_TOPIC::WATER_SET;
    publishDoc(client, "sensor", "spa_sensor_temp_set", doc, buf, 1024);
  }

  // ---- SENSOR: error ----
  {
    doc.clear();
    doc["name"] = "Erreur Spa";
    doc["uniq_id"] = "spa_sensor_error";
    doc["icon"] = "mdi:alert-circle";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"] = MQTT_TOPIC::ERROR;
    publishDoc(client, "sensor", "spa_sensor_error", doc, buf, 1024);
  }

  // ---- SENSOR: RSSI ----
  {
    doc.clear();
    doc["name"] = "WiFi RSSI Spa";
    doc["uniq_id"] = "spa_sensor_rssi";
    doc["icon"] = "mdi:wifi";
    doc["dev_cla"] = "signal_strength";
    doc["stat_cla"] = "measurement";
    doc["unit_of_meas"] = "dBm";
    doc["ent_cat"] = "diagnostic";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"] = MQTT_TOPIC::RSSI;
    publishDoc(client, "sensor", "spa_sensor_rssi", doc, buf, 1024);
  }

  // ---- SENSOR: wifi temp ----
  {
    doc.clear();
    doc["name"] = "Temperature WiFi Spa";
    doc["uniq_id"] = "spa_sensor_wifi_temp";
    doc["icon"] = "mdi:thermometer";
    doc["dev_cla"] = "temperature";
    doc["stat_cla"] = "measurement";
    doc["unit_of_meas"] = "\u00B0C";
    doc["ent_cat"] = "diagnostic";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"] = MQTT_TOPIC::WIFI_TEMP;
    publishDoc(client, "sensor", "spa_sensor_wifi_temp", doc, buf, 1024);
  }

  // ---- SENSOR: IP address ----
  {
    doc.clear();
    doc["name"] = "Adresse IP Spa";
    doc["uniq_id"] = "spa_sensor_ip";
    doc["icon"] = "mdi:ip-network";
    doc["ent_cat"] = "diagnostic";
    addDevice(doc.as<JsonObject>());
    doc["stat_t"] = MQTT_TOPIC::IP;
    publishDoc(client, "sensor", "spa_sensor_ip", doc, buf, 1024);
  }

  delete[] buf;
  Serial.println("HA Discovery: done.");
}
