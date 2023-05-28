#ifndef SENSORDATA_H
#define SENSORDATA_H

typedef struct
{
    int sensorID;
	int moisture;
	float lux;
	float humidity;
	float temperature;
    unsigned long epochTS;
} SensorData;

#endif // SENSORDATA_H