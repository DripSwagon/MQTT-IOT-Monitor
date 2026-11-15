#include <stdio.h>
#include <mosquitto.h>

void message(struct mosquitto *mq, void *data, const struct mosquitto_message *msg)
{
	(void)mq;
	(void)data;
	
	printf("RECEIVED MESSGAE: %.*s\n",msg->payloadlen, (char*)msg->payload);
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
		return 1;
	}


	mosquitto_message_callback_set(mq, message);


	int cn = mosquitto_connect(mq, "localhost", 1883, 60);
	
	if (cn != MOSQ_ERR_SUCCESS){
		printf("Failed to connect the client from application to MQTT");
		return 1;
		
	}

	int sb = mosquitto_subscribe(mq, NULL, "iot/devices/+/telemetry",0);
	if (sb != MOSQ_ERR_SUCCESS){
		printf("Failed to subscribe to the topic");
		return 1;
			
	}


	rc = mosquitto_loop_forever(mq,-1,1);

	printf("Loop stopped: %s\n", mosquitto_strerror(rc));

	mosquitto_disconnect(mq);
	mosquitto_destroy(mq);
	mosquitto_lib_cleanup();
	
	return 0;
	
}
