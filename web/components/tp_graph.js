var Throughput = {
"$schema": "https://vega.github.io/schema/vega-lite/v5.json",
"data": {
	"name":"myData"
},
	
	"width": 420,
	"height": 420,
	"params": [
		{"name": "toggleErrorbars", "bind": {"input": "checkbox"}},
	],
	"transform": [
		{"calculate": "floor(datum.t)", "as": "time"},
		{"filter": "datum.time >= 4 && datum.time <= 8"},
		{
			"aggregate": [{"op": "mean", "field": "bps", "as": "mean_bps_per_conn"}],
			"groupby": ["Exp", "Conns", "Socket", "Rep"]
		},
		{
			"aggregate": [
				{"op": "sum", "field": "mean_bps_per_conn", "as": "sum_of_conns"}
			],
			"groupby": ["Exp", "Conns", "Rep"]
		},
		{"calculate": "datum.sum_of_conns / 1000000000 ", "as": "Gbps"},
		{"extent": "Gbps", "param": "extent_gbps"},
		{
			"aggregate": [
				{"op": "mean", "field": "Gbps", "as": "mean"},
				{"op": "stdev", "field": "Gbps", "as": "stdev"}
			],
			"groupby": ["Exp", "Conns"]
		},
		{
			"calculate": "datum.mean-datum.stdev",
			"as": "lower"
		},
		{
			"calculate": "datum.mean+datum.stdev",
			"as": "upper"
		},
		{
			"calculate": "(datum.Exp == 'Custom1') ? 'IAPS+RFS':datum.Exp",
			"as": "Scheme"
		},
		{
			"calculate": "(datum.Scheme == 'Custom2') ? 'IAPS+LB':datum.Scheme",
			"as": "Scheme"
		}
	],

	"layer":[
		{
			"mark": "rule",
			"encoding": {
				"y": {"field": "upper", "type": "quantitative",
					"scale": {"domain": {"signal": "extent_gbps"}}},
				"y2": {"field": "lower", "type": "quantitative",
					"scale": {"domain": {"signal": "extent_gbps"}}},
				"x": {"field": "Conns", "type": "ordinal", "sort":[],
					"axis": {"labelFontSize": 15, "titleFontSize": 15, "labelAngle": 360}
				},
				"color": {
					"condition": {
						"param": "toggleErrorbars",
						"field": "Scheme",
						"type": "nominal"
					},
					"value": "#00000000"
				}

			}
		},
		{
			"mark": {"type": "line", "point": true, "tooltip":true},
			"encoding": {
				"x": {
					"field": "Conns",
					"type": "ordinal",
					"sort": [],
					"title": "Connections",
					"axis": {"labelFontSize": 15, "titleFontSize": 15, "labelAngle": 360}
				},
				"y": {
					//"aggregate": "mean",
					"field": "mean",
					"type": "quantitative",
					"scale": {"domain": {"signal": "extent_gbps"}},
					"title": "Gbps",
					"axis": {"labelFontSize": 15, "titleFontSize": 15}
				},
				"color": {
					"field": "Scheme",
					"type": "nominal",
					"legend": {"labelFontSize": 15, "titleFontSize": 15}
				},
			}

		}
	]
}

