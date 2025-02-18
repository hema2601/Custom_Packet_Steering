var Busy_Histo = {
"$schema": "https://vega.github.io/schema/vega-lite/v5.json",
"data": {
	"name":"myData"
},
	"transform":[
		{"filter":"datum.Busy_CPUs < 6"}
	],
	"mark": {"type":"bar", "tooltip": true},
	"encoding": {
		"x": {"field": "Busy_CPUs", "type": "ordinal"},
		"y": {"field": "Count", "type": "quantitative", "aggregate":"mean"},
		"row":{"field":"Exp"},
		"column":{"field":"Conns", "sort":[]}
	}
}
