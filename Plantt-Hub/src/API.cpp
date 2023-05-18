
#include "API.h"


API::API(const char *pIdentity, const char *pSecret) : _expireTS(NULL), _accessToken(""), _loggedIn(false)
{
	if (setAccessToken(pIdentity, pSecret))
	{
		PrintLn("Succesfully logged in");
		_loggedIn = true;
	}
	
}

API::~API()
{
}

bool API::setAccessToken(const char *pIdentity, const char *pSecret) {
	return API::validateLoginJson(getAccessToken(pIdentity, pSecret));
}

bool API::checkAccessToken() {
	if (1 == 2)
	{
		/* code */
	}
	
}

bool API::validateLoginJson(const char *jsonString)
{
	const char *expireTSStart = strstr(jsonString, "\"expireTs\":\"");
	if (expireTSStart == nullptr)
	{
		PrintLn("expireTs not found.");
		return false;
	}
	expireTSStart += strlen("\"expireTs\":\""); // Move to the start of the value

	const char *expireTsEnd = strchr(expireTSStart, '\"');
	if (expireTsEnd == nullptr)
	{
		PrintLn("Invalid expireTs value.");
		return false;
	}

	size_t expireTSLength = expireTsEnd - expireTSStart;
	strncpy(expireTS, expireTSStart, expireTSLength); // Add the value to our result
	expireTS[expireTSLength] = '\0';	// Null-terminate the string
	
	//TODO: VALIDATE THIS WORKS; WHEN WE GET EPOCH TIME.

	char* endPtr;
	_expireTS = strtoul(expireTS, &endPtr, 10);

	if (*endPtr != '\0') {
        // Error occurred during conversion
        PrintLn("Error: Invalid number format!");
    }


	const char *accessTokenStart = strstr(jsonString, "\"accessToken\":\"");
	if (accessTokenStart == nullptr)
	{
		PrintLn("accessToken not found.");
		return false;
	}
	accessTokenStart += strlen("\"accessToken\":\""); // Move to the start of the value

	const char *accessTokenEnd = strchr(accessTokenStart, '\"');
	if (accessTokenEnd == nullptr)
	{
		PrintLn("Invalid accessToken value.");
		return false;
	}

	size_t accessTokenLength = accessTokenEnd - accessTokenStart;
	strncpy(_accessToken, accessTokenStart, accessTokenLength); // Add the value to our result
	_accessToken[accessTokenLength] = '\0';	// Null-terminate the string

	return true;
}

const char* API::getAccessToken(const char *pIdentity, const char *pSecret) // maybe return char array
{
	bool result = false;
	char hostHttp[38] = "http://www.plantt.dk/api/v1/hub/login";

	char body[280] = "{\"identity\":";
	strcat(body, "\"");
	strcat(body, pIdentity);
	strcat(body, "\"");
	strcat(body, ",\"secret\":");
	strcat(body, "\"");
	strcat(body, pSecret);
	strcat(body, "\"");
	strcat(body, "}");
	strcat(body, "");
	PrintLn(body);

	HTTPClient http;
	http.begin(hostHttp);								// Specify destination for HTTP request
	http.addHeader("Content-Type", "application/json"); // Specify content-type header

	int httpResponseCode = http.POST(body);

	if (httpResponseCode > 0)
	{
		String response = http.getString(); // Get the response to the request

		PrintLn("httpResponseCode:");
		PrintLn(httpResponseCode); // Print return code
		PrintLn("response:");
		PrintLn(response); // Print request answer

		return response.c_str();

		/* login test = extractValuesFromLoginJson(response.c_str());
		PrintLn("expireTs:");
		PrintLn(test.expireTS);

		PrintLn("accessToken:");
		PrintLn(test.accessToken); */

		if ((httpResponseCode >= 200 && httpResponseCode <= 204) || httpResponseCode == 307)
		{
			result = true;
		}
		else if (httpResponseCode == 401)
		{
			_loggedIn = false;
			// TODO: Handle not logged in.
		}
	}
	else
	{
		PrintL("Error on sending POST: ");
		PrintLn(httpResponseCode); // Print return code
	}

	http.end(); // Free resources

}

/// @brief Post data to API, using http request.
/// @param readings
/// @return boolean if succeeded.
bool API::postReadingsAPI(Readings readings)
{
	bool result = false;
	// char hostHttp[35] = "http://www.plantt.dk/api/v1/hub/";
	char hostHttp[38] = "http://www.plantt.dk/api/v1/hub/ping";

	char body[80] = "{\"Temperature\":";
	sprintf(body + strlen(body), "%.1f", readings.temperature);
	strcat(body, ",\"Humidity\":");
	sprintf(body + strlen(body), "%.1f", readings.humidity);
	strcat(body, ",\"Lux\":");
	sprintf(body + strlen(body), "%.1f", readings.lux);
	strcat(body, ",\"Moisture\":");
	sprintf(body + strlen(body), "%d", readings.moisture);
	strcat(body, "}");
	PrintLn(body);

	// sprintf(hostHttp + strlen(hostHttp), "%d", 7);

	HTTPClient http;
	http.begin(hostHttp);																																																																																					   // Specify destination for HTTP request
	http.addHeader("Content-Type", "application/json");																																																																														   // Specify content-type header
	http.addHeader("Authorization", "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJqdGkiOiIxNGNmZTY1Ni05NTJkLTQwODMtYWE2YS00OGM2ZWY4MTM2MTgiLCJzdWIiOiIwMDJhNmE4Mi03OWVhLWVkMTEtOGFhNS0xYzg3MmM2MDRhM2IiLCJpc3MiOiJpc3N1ZXIuY29tIiwicm9sZSI6IlByZW1pdW0iLCJuYmYiOjE2ODQyMjI5MDQsImV4cCI6MTY4NDIyMzgwNCwiaWF0IjoxNjg0MjIyOTA0fQ.MO50kBLJkkbck-2-6f2BQeJ_fLFFuGFmnNJxNBB-J6U"); // Specify content-type header
	int httpResponseCode = http.POST(body);

	if (httpResponseCode > 0)
	{
		String response = http.getString(); // Get the response to the request

		PrintLn("httpResponseCode:");
		PrintL(httpResponseCode); // Print return code
		PrintLn("response:");
		PrintLn(response); // Print request answer

		if ((httpResponseCode >= 200 && httpResponseCode <= 204) || httpResponseCode == 307)
		{
			result = true;
		}
		else if (httpResponseCode == 401)
		{
			_loggedIn = false;
			// TODO: Handle not logged in.
		}
	}
	else
	{
		PrintL("Error on sending POST: ");
		PrintLn(httpResponseCode); // Print return code
	}

	http.end(); // Free resources

	return result;
}
