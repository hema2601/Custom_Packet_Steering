var RX_Lat_histo = 
{
		"$schema": "https://vega.github.io/schema/vega-lite/v5.json",
				"data": {
					"name":"myData"
				},
				"transform": [{
						"calculate": "datum.count / datum.total", "as": "percentage"
				},{

						"sort": [{"field": "start"}],
						"window": [{"op": "sum", "field": "percentage", "as": "Cumulative Count"}],
						"groupby": [
								"Exp", "Conns", "Rep"
						],
						"frame": [null, 0]
				}],

				"facet": {"row": {"field": "Exp"}, "column": {"field": "Conns", "sort": []}},
				"spec": {
						"encoding": {
								"x": {"bin": {"binned": true, "step": 5000}, "field": "start"},
								"x2": {"field": "end"},
								"y": {
										"field": "percentage",
										"type": "quantitative",
										"aggregate": "mean",
										"axis": {"format": "%"}
								},
						},
						"layer": [{
								"mark": {"type": "bar"},
								"encoding": {
										"y": {"field": "Cumulative Count", "type": "quantitative"}
								}
						}, {
								"mark": {"type": "bar", "color": "yellow", "opacity": 0.5},
								"encoding": {
										"y": {"field": "percentage", "type": "quantitative"}
								}
						}]
				}
}

