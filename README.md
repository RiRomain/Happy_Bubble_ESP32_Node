# Happy Bubble ESP32 Node
An ESP_32 based node for an Happy Bubbles presence server.


Please set your wifi and mqtt settings in Settings.h
Edit the PubSubClient max packet size in C:\Users\$user\Documents\Arduino\libraries\PubSubClient\src\PubSubClient.h, set MQTT_MAX_PACKET_SIZE to 512 or more. Otherwise the mqtt publish won't work as the default max size is to low.
