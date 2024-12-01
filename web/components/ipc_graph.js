var ipc = {
	"$schema": "https://vega.github.io/schema/vega-lite/v5.json",
	"description": "A simple bar chart with embedded data.",
	"width":220,
	"data": {
		"name":"myData"
	},
	"transform":[
		{"calculate":"datum.instructions/datum.cycles", "as":"IPC"},
		{"calculate": "(datum.Exp == 'Custom1') ? 'IAPS+RFS':datum.Exp", "as":"Scheme"},

		{"calculate": "(datum.Scheme == 'Custom2') ? 'IAPS+LB':datum.Scheme", "as":"Scheme"}
	],
	"mark": "bar",
	"encoding": {
		"column": {"field":"Type"},
		"x": {"field": "Conns", "type": "ordinal", "sort":[], "axis": {"labelAngle": 45}},
		"xOffset":{"field":"Scheme", "type":"nominal"},
		"y": {"aggregate":"mean", "field": "IPC", "type": "quantitative"},
		"color":{"field":"Scheme"}
	}
}

