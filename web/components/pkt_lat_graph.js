var Pkt_Lat_histo = {
"$schema": "https://vega.github.io/schema/vega-lite/v5.json",
"data": {
		"name":"myData"
},
		"transform": [
				{
						"joinaggregate": [{
								"op": "sum",
								"field": "count",
								"as": "total"
						}],
						"groupby": ["Exp", "Conns", "Rep"]
				},
				{"calculate":"datum.count / datum.total", "as":"percent"},
				{"calculate":"datum.end - datum.start", "as":"step"}
		],
		"facet": {"row": {"field": "Exp"}, "column": {"field": "Conns", "sort": []}},
		"spec": {
				"mark": {"type": "bar", "tooltip": true},
				"encoding": {
						"x": {"bin": {"binned": true, "step": 50}, "field": "start"},
						"x2": {"field": "end"},
						"y": {
								"field": "percent",
								"type": "quantitative",
								"aggregate": "mean",
								"axis": {"format": "%"}
						}
				}
		}
}
