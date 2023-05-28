#include "API.h"

API::API(const char *pIdentity, const char *pSecret) : _accessToken(""), _expireEpoch(0), _identity(pIdentity), _secret(pSecret)
{
	if (SetAccessToken())
	{
		PrintLn("Succesfully logged in");
	}
}

API::~API()
{
}

bool API::SetAccessToken()
{
	return API::ValidateLoginJson(GetAccessToken());
}

bool API::AccessTokenValid()
{
	TimeRTC *timertc = TimeRTC::GetInstance();
	PrintLn("Get Epoch Time:");
	PrintLn(timertc->GetEpochTime());
	PrintLn("Get _expireEpoch Time:");
	PrintLn(_expireEpoch);

	if (timertc->GetEpochTime() >= (_expireEpoch /*+ 600 */))
	{
		PrintLn("AccessTokenValid false");
		return false;
	}
	PrintLn("AccessTokenValid true");
	return true;
}

bool API::ValidateLoginJson(String jsonString)
{
	const String expireKey = "\"expire\":";
	const String accessTokenKey = "\"accessToken\":\"";

	// Find the position of "expire" key
	int expireStart = jsonString.indexOf("\"expire\":");
	if (expireStart == -1)
	{
		Serial.println("Failed to find 'expire' key in JSON string");
		return false;
	}

	// Find the position of "accessToken" key
	int accessTokenStart = jsonString.indexOf(accessTokenKey);
	if (accessTokenStart == -1)
	{
		Serial.println("Failed to find 'accessToken' key in JSON string");
		return false;
	}

	// Extract the value of "expire"
	int expireValueStart = expireStart + expireKey.length();
	int expireValueEnd = jsonString.indexOf(',', expireValueStart);
	_expireEpoch = jsonString.substring(expireValueStart, expireValueEnd).toInt(); // make signed long

	// Extract the value of "accessToken"
	int accessTokenValueStart = accessTokenStart + accessTokenKey.length();
	int accessTokenValueEnd = jsonString.indexOf('"', accessTokenValueStart);
	jsonString.substring(accessTokenValueStart, accessTokenValueEnd).toCharArray(_accessToken, 400, 0);

	return true;
}

/// @brief Get our accessToken using our identity and secret.
/// @return the response
String API::GetAccessToken()
{
	String response = "";
	char hostHttp[38] = "http://www.plantt.dk/api/v1/hub/login";

	char body[280] = "{\"identity\":";
strcat(body, "\"");
	strcat(body, _identity);
	strcat(body, "\"");
	strcat(body, ",\"secret\":");
	strcat(body, "\"");
	strcat(body, _secret);
	strcat(body, "\"");
	strcat(body, "}");
	strcat(body, "");
	PrintLn(body);

	WiFiClient *wifi = new WiFiClientFixed();
	HTTPClient http;
	http.begin(*wifi, hostHttp);						// Specify destination for HTTP request
	http.addHeader("Content-Type", "application/json"); // Specify content-type header

	int httpResponseCode = http.POST(body);

	if (httpResponseCode > 0)
	{
		PrintLn("httpResponseCode:");
		PrintLn(httpResponseCode); // Print return code

		if ((httpResponseCode >= 200 && httpResponseCode <= 204) || httpResponseCode == 307)
		{
			response = http.getString().c_str();
			PrintLn("response:");
			PrintLn(response); // Print request answer
		}
	}
	else
	{
		PrintL("Error on sending POST: ");
		PrintLn(httpResponseCode); // Print return code
		response = "";
	}

	http.end(); // Free resources
	delete wifi;

	return response;
}


char *API::getJsonFormattedSensorData(SensorData reading) 
{
	//{"sensorId": 2147483647,"temperature": 23.22,"humidity": 100.20,"lux": 10000.100,"moisture": 123,"timeStamp": 1685218608}
	char* body = new char[125]; //should be the max size of a object.
	strcpy(body, "{\"sensorId\":");
	sprintf(body + strlen(body), "%d", reading.sensorID);
	strcat(body, ",\"temperature\":");
	sprintf(body + strlen(body), "%.1f", reading.temperature);
	strcat(body, ",\"humidity\":");
	sprintf(body + strlen(body), "%.1f", reading.humidity);
	strcat(body, ",\"lux\":");
	sprintf(body + strlen(body), "%.1f", reading.lux);
	strcat(body, ",\"moisture\":");
	sprintf(body + strlen(body), "%d", reading.moisture);
	strcat(body, ",\"timeStamp\":");
	sprintf(body + strlen(body), "%d", reading.epochTS);
	strcat(body, "}");
	//strcat(body, "\0"); //Do we need to null terminate the string?

	PrintLn("body:");
	PrintLn(body);
	
	return body;
}

void API::freeString(char* strArr)
{
    delete[] strArr; // Free the dynamically allocated memory
}

/// @brief Post data to API, using http request.
/// @param readings
/// @return http response code
int API::PostReadingAPI(SensorData reading)
{
	if (!AccessTokenValid())
	{
		SetAccessToken();
	}

	int httpResponseCode = 0;
	char hostHttp[38] = "http://www.plantt.dk/api/v1/hub/Data";

	char* body = getJsonFormattedSensorData(reading);

	WiFiClient *wifi = new WiFiClientFixed();
	HTTPClient http;
	http.begin(*wifi, hostHttp);

	// http.begin(hostHttp); // Specify destination for HTTP request
	http.addHeader("Content-Type", "application/json");

	char bearer[408] = "Bearer "; // Specify content-type header
	strcat(bearer, _accessToken);
	http.addHeader("Authorization", bearer); // Specify content-type header
	httpResponseCode = http.POST(body);

	http.end(); // Free resources
	freeString(body);
	delete wifi;

	return httpResponseCode;
}

int API::PostReadingsAPI(SensorData *readings, int readingsAmount)
{
	if (!AccessTokenValid())
	{
		SetAccessToken();
	}

	int httpResponseCode = 0;
	char hostHttp[41] = "http://www.plantt.dk/api/v1/hub/BulkData";
	
	PrintLn("readingsAmount:");
	PrintLn(readingsAmount);

	char body[(125 * readingsAmount) + (readingsAmount) + 2]; //reading body size * amount + commas + []

	strcpy(body, "[");
	for (int i = 0; i < readingsAmount; i++)
	{
		char* readingJson = getJsonFormattedSensorData(readings[i]);
		strcat(body, readingJson);
		freeString(readingJson);
		PrintLn("i:");
		PrintLn(i);
		if (i != readingsAmount-1)
		{
			PrintLn("Add comma");
			strcat(body, ",");
		} 
		else 
		{
			PrintLn("Add nothing");

		}
	}
	strcat(body, "]");

	PrintLn("Post array body:");
	PrintLn(body);
	
	WiFiClient *wifi = new WiFiClientFixed();
	HTTPClient http;
	http.begin(*wifi, hostHttp);
	http.addHeader("Content-Type", "application/json");

	char bearer[408] = "Bearer "; // Specify content-type header
	strcat(bearer, _accessToken);
	http.addHeader("Authorization", bearer); // Specify content-type header
	httpResponseCode = http.POST(body);

	http.end(); // Free resources
	delete wifi;

	return httpResponseCode;
}
