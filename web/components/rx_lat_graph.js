var RX_Lat_histo = {
"$schema": "https://vega.github.io/schema/vega-lite/v5.json",
"data": {
	"name":"myData"
},

"transform": [
	{"calculate":"datum.count / datum.total", "as":"percentage"}
],
	"facet": {"row": {"field": "Exp"}, "column": {"field": "Conns", "sort": []}},
	"spec": {
		"mark": {"type": "bar", "tooltip": true},
		"encoding": {
			"x": {"bin": {"binned": true, "step": 5000}, "field": "start"},
			"x2": {"field": "end"},
			"y": {

				"field": "percentage", "type": "quantitative", "aggregate": "mean",
				"axis": {"format": "%"}}
		}
	}
}


