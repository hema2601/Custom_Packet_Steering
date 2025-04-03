var Drops = {
	"$schema": "https://vega.github.io/schema/vega-lite/v5.json",
	"repeat": ["dropped", "RX", "percentage", "sum"],
	"spec":{
		"data": {
			"name":"myData"
		},
		
		"width": 520,
		"height": 520,
		"transform": [
			{
				"calculate": "(datum.Exp == 'Custom1') ? 'IAPS+RFS': datum.Exp",
				"as": "Scheme"
			},
			
			{
				"calculate": "(datum.Exp == 'Custom2') ? 'IAPS+LB': datum.Exp",
				"as": "Scheme"
			},
			{
				"calculate": "(datum.Exp == 'Custom3') ? 'IAPS+RPS': datum.Exp",
				"as": "Scheme"
			},
			//{"filter": "datum.Type == 'dropped'"},
			{"calculate": "datum.After - datum.Before", "as": "value"},
			{"aggregate": [{
				"op": "sum",
				"field": "value",
				"as": "total_pkts"
			}],
				"groupby": ["Scheme", "Conns", "Rep", "Type"]},
			{
				"pivot": "Type",
				"value": "total_pkts",
				"groupby": ["Scheme", "Conns", "Rep"]
			},
			{"calculate": "(datum.dropped + datum.RX)", "as": "sum"},
			{"calculate": "datum.dropped / datum.sum", "as": "percentage"}
		],
		"mark": {"type":"bar","tooltip":true},
		"encoding": {
			"x": {
				"field": "Conns",
				"type": "ordinal",
				"sort": [],
				"title": "Connections",
				"axis": {"labelFontSize": 15, "titleFontSize": 15, "labelAngle": 360}                                                                                                                     },
			"xOffset": {"field": "Scheme", "sort": "['RPS', 'RFS', 'IAPS']"},
			"y": {                                                                                                                                                                                            "aggregate": "mean",
				"field": {"repeat": "repeat"},
				"type": "quantitative",
				//"title": "Dropped Packets",
				"axis": {"labelFontSize": 15, "titleFontSize": 15}
			},                                                                                                                                                                                            "color": {
				"field": "Scheme",                                                                                                                                                                            "type": "nominal",
				"legend": {"labelFontSize": 15, "titleFontSize": 15}                                                                                                                                      }
		}
	}                                                                                                                                                                                     }

