var PpIPI_err = {
"$schema": "https://vega.github.io/schema/vega-lite/v5.json",
"data": {
	"name":"myData"
},
	"width": 420,
	"height": 420,
	"transform": [
		{"filter": "datum.CPU >= 0 && datum.CPU <= 7"},
		{
			"calculate": "(datum['RPS Interrupts'] != 0) ? datum.Processed / datum['RPS Interrupts'] : 0",
			"as": "Packets Per IPI"
		},
		{"filter": "datum.Processed != 0"},
		{
			"aggregate": [
				{"op": "mean", "field": "Packets Per IPI", "as": "mean"},
				{"op": "stdev", "field": "Packets Per IPI", "as": "stdev"}
			],
			"groupby": ["Exp", "Conns"]
		},
		{"calculate": "datum.mean-datum.stdev", "as": "lower"},
		{"calculate": "datum.mean+datum.stdev", "as": "upper"},
		{
			"calculate": "(datum.Exp == 'Custom1') ? 'IAPS': datum.Exp",
			"as": "Scheme"
		}
	],
	"layer": [

		{
			"mark": {"type":"bar", "tooltip":true},
			"encoding": {
				"x": {
					"field": "Conns",
					"type": "ordinal",
					"sort": [],
					"title": "Connections",
					"axis": {"labelFontSize": 15, "titleFontSize": 15, "labelAngle": 360}
				},
				"xOffset": {"field": "Scheme", "sort": "['RPS', 'RFS', 'IAPS']"},
				"y": {
					"field": "mean",
					"type": "quantitative",
					"title": "Packet Per IPI",
					"axis": {"labelFontSize": 15, "titleFontSize": 15}
				},
				"color": {
					"field": "Scheme",
					"type": "nominal",
					"legend": {"labelFontSize": 15, "titleFontSize": 15}
				}
			}
		},
		{
			"mark": "rule",
			"encoding": {
				"y": {
					"field": "upper",
					"type": "quantitative"
				},
				"y2": {
					"field": "lower",
					"type": "quantitative"
				},
				"x": {
					"field": "Conns",
					"type": "ordinal",
					"sort": [],
					"axis": {"labelFontSize": 15, "titleFontSize": 15, "labelAngle": 360}
				},
				"xOffset": {"field": "Scheme", "sort": ["RPS", "RFS", "IAPS"]},
				"color": {"value":"black"}
			}
		}
	]
}
