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

	if (timertc->GetEpochTime() < (_expireEpoch /*+ 600 */))
	{
		return false;
	}
	return true;
}

bool API::ValidateLoginJson(const char *jsonString)
{
	char *expireStart = strstr(jsonString, "\"expire\":");
	delay(10);
	if (expireStart == nullptr)
	{
		PrintLn("expire not found."); // TODO: why are we sometimes hitting this????
		return false;
	}
	expireStart += strlen("\"expire\":"); // Move to the start of the value

	const char *expireEnd = strchr(expireStart, ',');
	if (expireEnd == nullptr)
	{
		PrintLn("Invalid expire value.");
		return false;
	}

	size_t expiereLength = expireEnd - expireStart;
	char expireBuffer[11] = "";
	strncpy(expireBuffer, expireStart, expiereLength); // Add the value to our result
	expireBuffer[expiereLength] = '\0';				   // Null-terminate the string

	char *endPtr;
	_expireEpoch = strtoul(expireBuffer, &endPtr, 10);

	if (*endPtr != '\0')
	{
		// Error occurred during conversion
		PrintLn("Error: Invalid number format!");
	}
	PrintLn("ExpireEpoch long");
	PrintLn(_expireEpoch);

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
	_accessToken[accessTokenLength] = '\0';						// Null-terminate the string

	return true;
}

const char *API::GetAccessToken() // maybe return char array
{
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

		if ((httpResponseCode >= 200 && httpResponseCode <= 204) || httpResponseCode == 307)
		{
			// result = true; TODO: handle this
		}
	}
	else
	{
		PrintL("Error on sending POST: ");
		PrintLn(httpResponseCode); // Print return code
	}

	http.end(); // Free resources
	return "";	// TODO: do something better here
}

/// @brief Post data to API, using http request.
/// @param readings
/// @return boolean if succeeded.
bool API::PostReadingsAPI(Readings readings)
{

	if (!AccessTokenValid())
	{
		SetAccessToken();
	}

	bool result = false;
	char hostHttp[38] = "http://www.plantt.dk/api/v1/hub/ping2";

	char body[80] = "{\"Temperature\":";
	sprintf(body + strlen(body), "%.1f", readings.temperature);
	strcat(body, ",\"Humidity\":");
	sprintf(body + strlen(body), "%.1f", readings.humidity);
	strcat(body, ",\"Lux\":");
	sprintf(body + strlen(body), "%.1f", readings.lux);
	strcat(body, ",\"Moisture\":");
	sprintf(body + strlen(body), "%d", readings.moisture);
	strcat(body, "}");
	PrintLn("body:");
	PrintLn(body);

	HTTPClient http;
	http.begin(hostHttp);																																																																																					   // Specify destination for HTTP request
	http.addHeader("Content-Type", "application/json");
	
	char bearer[408] = "Bearer ";// Specify content-type header
	strcat(bearer, _accessToken);
	http.addHeader("Authorization", bearer); // Specify content-type header
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
			http.end();
			SetAccessToken();
			PostReadingsAPI(readings);
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
