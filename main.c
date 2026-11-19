#include <stdio.h>
#include <mosquitto.h>
#include <cjson/cJSON.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_DEVICES 100

volatile sig_atomic_t stop_requested = 0;
FILE *log_file = NULL;

void handle_sigint(int signum)
{
    (void)signum;
    stop_requested = 1;
}



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
        
		if (strlen(device_id) >= sizeof(devices[0].device_id)) {
		    printf("Device ID is too long!\n");
		    return 0;
		}
		index = device_count;
		device_count++;
        strcpy(devices[index].device_id, device_id);
    }

    devices[index].temperature = temperature;
    devices[index].humidity = humidity;
    devices[index].battery = battery;
    devices[index].timestamp = timestamp;

    return 1;
}


void print_devices(void)
{
    printf("\n THERE ARE STORED DEVICES (%d) \n", device_count);

    for (int i = 0; i < device_count; i++) {
    	printf("--------------------------\n");
        printf("Device: %s\n", devices[i].device_id);
        printf("Temperature: %.2f C\n", devices[i].temperature);
        printf("Humidity: %.2f%%\n", devices[i].humidity);
        printf("Battery: %.1f%%\n", devices[i].battery);
        printf("Timestamp: %.0f\n", devices[i].timestamp);
        printf("--------------------------\n");
    }
    printf("\n---------THE END---------\n");
}


void log_message(const char *message)
{
    printf("%s", message);

    if (log_file != NULL)
    {
        fprintf(log_file, "%s", message);
        fflush(log_file);
    }
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

   
    if (device->valuestring == NULL || strlen(device->valuestring) == 0) {
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

    print_devices();



	if (temperature->valuedouble > 30) {
	    log_message("ALERT: HIGH_TEMPERATURE\n");
	    publish_alert(mq, device->valuestring,"HIGH_TEMPERATURE", temperature->valuedouble, timestamp->valuedouble);
	}
	
	if (humidity->valuedouble > 80) {
	    log_message("ALERT: HIGH_HUMIDITY\n");
	    publish_alert(mq, device->valuestring,"HIGH_HUMIDITY", humidity->valuedouble, timestamp->valuedouble);
	}
	
	if (battery->valuedouble < 20) {
	    log_message("ALERT: LOW_BATTERY\n");
	    publish_alert(mq, device->valuestring,"LOW_BATTERY", battery->valuedouble, timestamp->valuedouble);
	}
	
    cJSON_Delete(json);
}



void connect_callback(struct mosquitto *mq, void *data, int rc)
{
    (void)data;

    if (rc == 0) {
        printf("Successfully connected to MQTT broker!\n");
        int result = mosquitto_subscribe(mq, NULL, "iot/devices/+/telemetry",0);
        
        if (result != MOSQ_ERR_SUCCESS)
        {
        	printf("Could not subscribe after reconnecting: %s\n", mosquitto_strerror(result));
        }
        else { printf("Connected subscription!\n"); }
        
    } 

    else 
    {
        printf("Connection failed: %s\n", mosquitto_connack_string(rc));
    }
}


void disconnect_callback(struct mosquitto *mq, void *data, int rc)
{
    (void)mq;
    (void)data;

    if (rc == 0) {
        printf("Disconnected from MQTT broker normally.\n");
    } else {
        printf("Unexpected disconnection: %s\n", mosquitto_strerror(rc));
    }
}

int main(int argc, char *argv[]){

	char *host = "localhost";
	int port = 1883;
	char *output_file = NULL;
	
	struct mosquitto *mq;

	for (int i = 1; i < argc; i++)
	{
        if (strcmp(argv[i], "-H") == 0)
        {
            if (i + 1 >= argc)
            {
                printf("Missing host after -H!\n");
                return 1;
            }
        
            host = argv[++i];
        }
        
        else if (strcmp(argv[i], "-P") == 0)
        {
            if (i + 1 >= argc)
            {
                printf("Missing port after -P!\n");
                return 1;
            }
        
            port = atoi(argv[++i]);
        
            if (port < 1 || port > 65535)
            {
                printf("Invalid port!\n");
                return 1;
            }
        }

        else if (strcmp(argv[i], "-o") == 0)
        {
            if (i + 1 >= argc)
            {
                printf("Missing filename after -o!\n");
                return 1;
            }
        
            output_file = argv[++i];
        }
        
        else if (strcmp(argv[i], "-h") == 0)
        {
            printf("Usage: ./iot_monitor [-H HOST] [-P PORT] [-h]\n");
            return 0;
        }
	}


	if (output_file != NULL)
	{
	    log_file = fopen(output_file, "a");

	    if (log_file == NULL)
	    {
	        perror("Could not open log file");
	        return 1;
	    }
	}

	printf("Starting MQTT IoT Device Monitor\n");

	signal(SIGINT, handle_sigint);

	

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
	mosquitto_connect_callback_set(mq, connect_callback);
	mosquitto_disconnect_callback_set(mq, disconnect_callback);


	rc = mosquitto_loop_start(mq);
	
	if (rc != MOSQ_ERR_SUCCESS) {
	    printf("Failed to start MQTT thread: %s\n", mosquitto_strerror(rc));
	    mosquitto_destroy(mq);
	    mosquitto_lib_cleanup();
	    return 1;
	}

	int cn = mosquitto_connect_async(mq, host, port, 60);
	
	if (cn != MOSQ_ERR_SUCCESS){
		printf("Failed to initialize connection between the client to MQTT\n");
		printf("Reason: %s\n", mosquitto_strerror(cn));
		mosquitto_destroy(mq);
		mosquitto_lib_cleanup();
		return 1;
		
	}

	while (!stop_requested) {
	    sleep(1);
	}

	printf("Shutting down application, please wait...\n");

	mosquitto_disconnect(mq);
	mosquitto_loop_stop(mq,false);
	mosquitto_destroy(mq);
	mosquitto_lib_cleanup();

	if (log_file != NULL)
	{
	    fclose(log_file);
	}

	printf("Shutdown successful\n");
	
	return 0;
	
}
