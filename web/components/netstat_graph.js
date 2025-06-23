var netstat = {
		"$schema": "https://vega.github.io/schema/vega-lite/v6.json",
		"description": "A simple bar chart with embedded data.",
		"data": {"name":"myData"},
		"repeat":["TCPOFOQueue", "TCPHPHits"],
		"spec":{
		"mark": {"type":"bar", "tooltip":true},
		"encoding": {
				"x": {"field": "Conns", "type": "nominal","sort":[], "axis": {"labelAngle": 0}},
				"xOffset":{"field":"Exp"},
				"y": {"field": {"repeat":"repeat"}, "type": "quantitative", "aggregate":"mean"},
				"color": {"field":"Exp"}
		}
		}
}

