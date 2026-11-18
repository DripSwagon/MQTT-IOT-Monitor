#include <stdio.h>
#include <mosquitto.h>
#include <cjson/cJSON.h>
#include <string.h>

#define MAX_DEVICES 100

struct Device {
    char device_id[64];
    double temperature;
    double humidity;
    double battery;
    double timestamp;
};


struct Device devices[MAX_DEVICES];
int device_count = 0;

int find_device(const char *device_id)
{
    for (int i = 0; i < device_count; i++) {
        if (strcmp(devices[i].device_id, device_id) == 0) {
            return i;
        }
    }

    return -1;
}

int update_device(const char *device_id, double temperature, double humidity, double battery, double timestamp)
{
    int index = find_device(device_id);

    if (index == -1) {
        if (device_count >= MAX_DEVICES) {
            printf("Maximum number of devices reached!\n");
            return 0;
        }

        index = device_count;
        device_count++;
        
		if (strlen(device_id) >= sizeof(devices[index].device_id)) {
		    printf("Device ID is too long!\n");
		    return 0;
		}
        strcpy(devices[index].device_id, device_id);
    }

    devices[index].temperature = temperature;
    devices[index].humidity = humidity;
    devices[index].battery = battery;
    devices[index].timestamp = timestamp;

    return 1;
}


void publish_alert(struct mosquitto *mq, const char *device_id,  const char *alert_type, double value, double timestamp)
{
	cJSON *alert = cJSON_CreateObject();

	if (alert == NULL) {
	    printf("Failed to create cJSON alert!\n");
		return;
	}

	if (cJSON_AddStringToObject(alert, "device_id", device_id) == NULL ||
	cJSON_AddStringToObject(alert, "alert", alert_type) == NULL ||
	cJSON_AddNumberToObject(alert, "value", value) == NULL ||
	cJSON_AddNumberToObject(alert, "timestamp", timestamp) == NULL)
	{
		printf("Failed to add data to created alert JSON!");
		cJSON_Delete(alert);
		return;
		
	}


	//Byte payload
	char *payload = cJSON_PrintUnformatted(alert);

	if (payload == NULL) {
	    printf("Couldn't turn cJSON alert into byte payload!\n");
	    cJSON_Delete(alert);
		return;
	}

	char topic[128];
	int byteswritten;
	
	byteswritten = snprintf(topic, sizeof(topic), "iot/devices/%s/alert", device_id);

	if (byteswritten < 0 || (size_t)byteswritten >= sizeof(topic)) {
		printf("Failed to append/create  topic!\n");
		cJSON_Delete(alert);
		cJSON_free(payload);
		return;
	}
	
	int pbl = mosquitto_publish(mq, NULL, topic, (int)strlen(payload), payload, 0, false);

	if (pbl != MOSQ_ERR_SUCCESS){
		printf("Alert publish failed, error: %s\n", mosquitto_strerror(pbl));
	}
	else {
		printf("Alert was published successfully!\n");
		
	}
	
	cJSON_Delete(alert);
	cJSON_free(payload);

	return;
}



void message(struct mosquitto *mq, void *data, const struct mosquitto_message *msg)
{
    (void)data;

    cJSON *json = cJSON_ParseWithLength((const char *)msg->payload, msg->payloadlen);

    if (json == NULL) {
        printf("Invalid JSON!\n");
        return;
    }


    cJSON *device = cJSON_GetObjectItemCaseSensitive(json, "device_id");
    cJSON *temperature = cJSON_GetObjectItemCaseSensitive(json, "temperature");
    cJSON *humidity = cJSON_GetObjectItemCaseSensitive(json, "humidity");
    cJSON *battery = cJSON_GetObjectItemCaseSensitive(json, "battery");
    cJSON *timestamp = cJSON_GetObjectItemCaseSensitive(json, "timestamp");


    if (!cJSON_IsString(device) ||!cJSON_IsNumber(temperature) ||!cJSON_IsNumber(humidity) ||!cJSON_IsNumber(battery) ||!cJSON_IsNumber(timestamp)) {

        printf("Missing or invalid data!\n");
        cJSON_Delete(json);
        return;
    }

   
    if (strlen(device->valuestring) == 0) {
        printf("Invalid device ID!\n");
        cJSON_Delete(json);
        return;
    }

    if (temperature->valuedouble < -50 || temperature->valuedouble > 100) {
        printf("Invalid temperature!\n");
        cJSON_Delete(json);
        return;
    }

    if (humidity->valuedouble < 0 || humidity->valuedouble > 100) {
        printf("Invalid humidity!\n");
        cJSON_Delete(json);
        return;
    }

    if (battery->valuedouble < 0 || battery->valuedouble > 100) {
        printf("Invalid battery level!\n");
        cJSON_Delete(json);
        return;
    }
    
    printf("Device: %s\n", device->valuestring);
    printf("Temperature: %.2f C\n", temperature->valuedouble);
    printf("Humidity: %.2f%%\n", humidity->valuedouble);
    printf("Battery: %.1f%%\n", battery->valuedouble);
    printf("Timestamp: %.0f\n", timestamp->valuedouble);

    if(!update_device(device->valuestring,temperature->valuedouble,humidity->valuedouble,battery->valuedouble,timestamp->valuedouble))
    {
		printf("Failed to update device information!\n");
		cJSON_Delete(json);
		return;
    	
    }



	if (temperature->valuedouble > 30) {
	    printf("ALERT: HIGH_TEMPERATURE\n");
	    publish_alert(mq, device->valuestring,"HIGH_TEMPERATURE", temperature->valuedouble, timestamp->valuedouble);
	}
	
	if (humidity->valuedouble > 80) {
	    printf("ALERT: HIGH_HUMIDITY\n");
	    publish_alert(mq, device->valuestring,"HIGH_HUMIDITY", humidity->valuedouble, timestamp->valuedouble);
	}
	
	if (battery->valuedouble < 20) {
	    printf("ALERT: LOW_BATTERY\n");
	    publish_alert(mq, device->valuestring,"LOW_BATTERY", battery->valuedouble, timestamp->valuedouble);
	}
	
    cJSON_Delete(json);
}


int main(void){

	struct mosquitto *mq;

	printf("Starting MQTT IoT Device Monitor\n");

	int rc = mosquitto_lib_init();

	if (rc != MOSQ_ERR_SUCCESS){
		printf("Mosquitto start failed\n");
		return 1;
		
	}

	mq = mosquitto_new("iot_monitor",true,NULL);

	if (mq == NULL){
		printf("Failed to create the MQTT client\n");
		mosquitto_lib_cleanup();
		return 1;
	}


	mosquitto_message_callback_set(mq, message);


	int cn = mosquitto_connect(mq, "localhost", 1883, 60);
	
	if (cn != MOSQ_ERR_SUCCESS){
		printf("Failed to connect the client from application to MQTT");
		printf("Reason: %s\n", mosquitto_strerror(cn));
		mosquitto_destroy(mq);
		mosquitto_lib_cleanup();
		return 1;
		
	}

	int sb = mosquitto_subscribe(mq, NULL, "iot/devices/+/telemetry",0);
	if (sb != MOSQ_ERR_SUCCESS){
		printf("Failed to subscribe to the topic");
		mosquitto_destroy(mq);
		mosquitto_lib_cleanup();
		return 1;
			
	}


	rc = mosquitto_loop_forever(mq,-1,1);

	printf("Loop stopped: %s\n", mosquitto_strerror(rc));

	mosquitto_disconnect(mq);
	mosquitto_destroy(mq);
	mosquitto_lib_cleanup();
	
	return 0;
	
}
