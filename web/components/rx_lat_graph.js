var RX_Lat_histo = {
"$schema": "https://vega.github.io/schema/vega-lite/v5.json",
"data": {
	"name":"myData"
},

"transform":[
	{"filter":{"field": "start", "valid": true}},
],
	"mark": {"type":"bar","tooltip":true},
	"encoding": {
		"x": {"bin": {"binned":true, "step":5000}, "field": "start"},
		"x2":{"field":"end"},
		"y": {"field": "count", "type": "quantitative", "aggregate":"sum"},
		"row":{"field":"Exp"},
		"column":{"field":"Conns", "sort":[]}
	}


}
