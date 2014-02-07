/*global window: false */
/*global locationWatcher: true */
var d = new Date();
var hr = d.getHours();
var decimal_min = d.getMinutes() / 60.0;
var three_hr_index = Math.floor(parseFloat(hr) / 3);
console.log(d, hr + decimal_min, three_hr_index);
var area_long = 0;
var area_lat = 0;
var req = new XMLHttpRequest();
var index = 0;
var len = 0;
var units = ["in","mph","\u00B0F"];

function fetchNearby(latitude, longitude) {
	var response;
	req.open('GET', "http://api.worldweatheronline.com/free/v1/search.ashx?q="+latitude+","+longitude+"&format=json&num_of_results=10&wct=Ski&key=ez7khhsws227hk6ckeryvawq");
	req.onload = function(e) {
		if (req.readyState == 4) {
			if(req.status == 200) {
				//		console.log(req.responseText)
				response = JSON.parse(req.responseText);
				len = response.search_api.result.length;
				var skiResult = response.search_api.result;
				if (skiResult) {
					//			console.log("Index: " + index);
					var area_name = skiResult[index].areaName[0].value;
					area_long = skiResult[index].longitude;
					area_lat = skiResult[index].latitude;
					console.log("Area Name: " + area_name + "\n");
					Pebble.sendAppMessage({
						"area_name": area_name
						});
				}
			} 
			else {
				console.log("HTTP " + req.status + " error" + "\n");
			}
		}
	};
	req.send();
	req.onloadend = function() {
		fetchSkiReport(area_lat, area_long);
	};
}

function fetchSkiReport(latitude, longitude) {
	var response;
	req.open('GET', "http://api.worldweatheronline.com/free/v1/ski.ashx?q="+latitude+","+longitude+"&format=json&includelocation=yes&key=ez7khhsws227hk6ckeryvawq");
	req.onload = function(e) {
		if (req.readyState == 4) {
			if(req.status == 200) {
				//		console.log(req.responseText);
				response = JSON.parse(req.responseText);
				var skiReport = response;
				if (skiReport) {
					var weather_desc = skiReport.data.weather[0].hourly[three_hr_index].bottom[0].weatherDesc[0].value;
					var minTemp = skiReport.data.weather[0].hourly[three_hr_index].top[0].tempF;
					var maxTemp = skiReport.data.weather[0].hourly[three_hr_index].bottom[0].tempF;
					if (units[0] === "cm") {
						minTemp = skiReport.data.weather[0].hourly[three_hr_index].top[0].tempC;
						maxTemp = skiReport.data.weather[0].hourly[three_hr_index].bottom[0].tempC;
					}
					var area_temps = "Temp(" + units[2] + "): " + minTemp + " to " + maxTemp;
					var wind_speed = skiReport.data.weather[0].hourly[three_hr_index].bottom[0].windspeedMiles;
					if (units[1] === "kph") {
						wind_speed = skiReport.data.weather[0].hourly[three_hr_index].bottom[0].windspeedKmph;
					}
					var wind_dir = skiReport.data.weather[0].hourly[three_hr_index].bottom[0].winddir16Point;
					var chance_of_snow = skiReport.data.weather[0].chanceofsnow;
					var area_snowfall = skiReport.data.weather[0].totalSnowfall_cm;
					if (units[0] === "cm") {
						area_snowfall = parseFloat(area_snowfall / 2.54).toFixed(2);
					}
					console.log("Weather Description: " + weather_desc + "\n");
					console.log("Wind: " + wind_dir + " " + wind_speed + " " + units[1] + "\n");
					console.log("Snow Chance: " + chance_of_snow + "%" + "\n");	
					console.log("Snowfall:" + area_snowfall + " " + units[0] + "\n");
					console.log("Area Temps: " + area_temps + "\n");
					var test_weather = /snow/g;
					if (test_weather.test(weather_desc) === true) {
						Pebble.sendAppMessage({"weather_desc": weather_desc});
					}
					else {
						Pebble.sendAppMessage({"weather_desc": "Chance of Snow: " + chance_of_snow + "%"});
					}
					
					Pebble.sendAppMessage({
						"area_snowfall": "Snow(24h): " + area_snowfall + " " + units[0],
						"area_temps": area_temps,
						"area_wind": "Wind: " + wind_dir + " " + wind_speed + " " + units[1],
						"update_time": "Updated @ " + three_hr_index * 3 + ":00"});
				}
			} 
			else {
				console.log("HTTP " + req.status + " error" + "\n");
			}
		}
	};
	req.send(null);
	req.onloadend = function() {
	};
}

function locationSuccess(pos) {
	console.warn("Location Found!");
	var coordinates = pos.coords;
	fetchNearby(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
	console.warn("location error (" + err.code + "): " + err.message);
	
	if (err.code == 3) {
		console.log("Haven't moved.\n");
	}
	
	else if (err.code == 1) {
		Pebble.sendAppMessage({
			"area_name":"DENIED",
			});
	}
	
	else {
		Pebble.sendAppMessage({
			"area_name":"Loc Unavailable",
			});
	}
}

var locationOptions = {"timeout": 15000, "maximumAge": 60000 }; 


Pebble.addEventListener("ready",
						function(e) {
							console.log("connect!" + e.ready);
							locationWatcher = window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
							console.log(e.type);
						});

Pebble.addEventListener("appmessage",
						function(e) {
							console.log("Message!");
							console.log(e.type);
							//							console.log("Index: " + e.payload.index);
							if (e.payload.index !== null) {
								index = e.payload.index;
							}
							if (e.payload.units === 1) {
								units = ["cm","kph","\u00B0C"];
							}
							else {
								units = ["in","mph","\u00B0F"];
							}
							window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
						});

Pebble.addEventListener("webviewclosed",
						function(e) {
							console.log("webview closed");
							console.log(e.type);
							console.log(e.response);
						});

