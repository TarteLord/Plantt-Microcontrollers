#include "API.h"

/// @brief Constructs an instance of the API class with the specified identity and secret.
///        It initializes the access token and checks if the login is successful.
/// @param pIdentity The identity used for authentication.
/// @param pSecret The secret used for authentication.
API::API(const char *pIdentity, const char *pSecret) : _accessToken(""), _expireEpoch(0), _identity(pIdentity), _secret(pSecret)
{
	if (SetAccessToken())
	{
		PrintLn("Succesfully logged in");
	}
}

/// @brief Sets the access token by validating the login JSON string obtained from GetAccessToken().
/// @return `true` if the access token is successfully set, `false` otherwise.
bool API::SetAccessToken()
{
	return API::ValidateLoginJson(GetAccessToken());
}

/// @brief Checks if the access token is still valid based on the current epoch time.
/// @return `true` if the access token is still valid, `false` otherwise.
bool API::AccessTokenValid()
{
	TimeRTC *timertc = TimeRTC::GetInstance();
	PrintLn("Get Epoch Time:");
	PrintLn(timertc->GetEpochTime());
	PrintLn("Get _expireEpoch Time:");
	PrintLn(_expireEpoch);

	if (timertc->GetEpochTime() >= (_expireEpoch))
	{
		PrintLn("AccessTokenValid false");
		return false;
	}
	PrintLn("AccessTokenValid true");
	return true;
}

/// @brief Validates the login JSON string and extracts the expire and accessToken values.
/// @param jsonString The JSON string to be validated.
/// @return `true` if the validation and extraction are successful, `false` otherwise.
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

/// @brief Generates a JSON-formatted string representing the provided SensorData.
/// @param reading The SensorData object to be formatted as JSON.
/// @return A dynamically allocated char array containing the JSON-formatted string.
char *API::getJsonFormattedSensorData(SensorData reading) 
{
	char* body = new char[125]; //should be the max size of a object.
	
	/// Construct the JSON-formatted string.
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

	PrintLn("body:");
	PrintLn(body);
	
	return body;
}

/// @brief Frees the dynamically allocated memory of a char array.
/// @param strArr The char array to be freed.
void API::freeString(char* strArr)
{
    delete[] strArr; // Free the dynamically allocated memory
}

/// @brief Posts a single reading to the API.
/// @param reading The SensorData object representing the reading to be posted.
/// @return The HTTP response code received from the API.
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

	http.addHeader("Content-Type", "application/json"); // Specify content-type header
	char bearer[408] = "Bearer "; 
	strcat(bearer, _accessToken);
	http.addHeader("Authorization", bearer); // Specify bearer Authorization
	httpResponseCode = http.POST(body);

	http.end(); // Free resources
	freeString(body);
	delete wifi;

	return httpResponseCode;
}

/// @brief Posts multiple readings to the API.
/// @param readings An array of SensorData objects representing the readings to be posted.
/// @param readingsAmount The number of readings in the array.
/// @return The HTTP response code received from the API.
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
	}
	strcat(body, "]");

	PrintLn("Post array body:");
	PrintLn(body);
	
	WiFiClient *wifi = new WiFiClientFixed();
	HTTPClient http;
	http.begin(*wifi, hostHttp);
	http.addHeader("Content-Type", "application/json"); // Specify content-type header

	char bearer[408] = "Bearer "; 
	strcat(bearer, _accessToken);
	http.addHeader("Authorization", bearer);  // Specify bearer Authorization
	httpResponseCode = http.POST(body);

	http.end(); // Free resources
	delete wifi;

	return httpResponseCode;
}

int API::AddSensor() 
{
	if (!AccessTokenValid())
	{
		SetAccessToken();
	}

	int httpResponseCode = 0;
	char hostHttp[43] = "http://www.plantt.dk/api/v1/hub/new-sensor";

	char* body = "";
	String response;

	WiFiClient *wifi = new WiFiClientFixed();
	HTTPClient http;
	http.begin(*wifi, hostHttp);

	http.addHeader("Content-Type", "application/json"); // Specify content-type header
	char bearer[408] = "Bearer "; 
	strcat(bearer, _accessToken);
	http.addHeader("Authorization", bearer); // Specify bearer Authorization
	httpResponseCode = http.POST(body);

	if (httpResponseCode > 0)
	{
		PrintLn("httpResponseCode:");
		PrintLn(httpResponseCode); // Print return code

		if ((httpResponseCode >= 200 && httpResponseCode <= 204) || httpResponseCode == 307)
		{
			response = http.getString();
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

	if (response == "")
	{
		return 0;
	}
	
	return response.toInt();
}
