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

	HTTPClient http;
	http.begin(hostHttp);								// Specify destination for HTTP request
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

	return response;
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
	http.begin(hostHttp); // Specify destination for HTTP request
	http.addHeader("Content-Type", "application/json");

	char bearer[408] = "Bearer "; // Specify content-type header
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
