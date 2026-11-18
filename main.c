#include <stdio.h>
#include <mosquitto.h>
#include <cjson/cJSON.h>
#include <string.h>

void message(struct mosquitto *mq, void *data,
             const struct mosquitto_message *msg)
{
    (void)mq;
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



	if (temperature->valuedouble > 30) {
	    printf("ALERT: HIGH_TEMPERATURE\n");
	}
	
	if (humidity->valuedouble > 80) {
	    printf("ALERT: HIGH_HUMIDITY\n");
	}
	
	if (battery->valuedouble < 20) {
	    printf("ALERT: LOW_BATTERY\n");
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
