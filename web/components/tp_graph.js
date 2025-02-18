var Throughput = {
"$schema": "https://vega.github.io/schema/vega-lite/v5.json",
"data": {
	"name":"myData"
},
	"width":220,
	"height":220,
	"transform":[
		{"calculate": "floor(datum.t)", "as":"time"},
		{"filter":"datum.time >= 4 && datum.time <= 8"},
		//{"filter":"datum.Exp == 'Custom1' || datum.Exp == 'RPS' || datum.Exp == 'RFS' || datum.Exp == 'RSS'"},
		{
			"aggregate": [{
				"op": "mean",
				"field": "bps",
				"as": "mean_bps_per_conn"
			}],
			"groupby": [ "Exp", "Conns", "Socket"]
		},
		{
			"aggregate": [{
				"op": "sum",
				"field": "mean_bps_per_conn",
				"as": "sum_of_conns"
			}],
			"groupby": [ "Exp", "Conns"]
		},
		{"calculate": "datum.sum_of_conns / 1000000000 ", "as":"Gbps"},
		{"extent":"Gbps", "param":"extent_gbps"},
		{"calculate": "(datum.Exp == 'Custom1') ? 'IAPS+RFS':datum.Exp", "as":"Scheme"},

		{"calculate": "(datum.Scheme == 'Custom2') ? 'IAPS+LB':datum.Scheme", "as":"Scheme"}
	],
	"mark": {
		"type": "line",
		"point": true,
		"tooltip": true
	},
	"encoding": {
		"x": {"field": "Conns", "type":"ordinal", "sort":[], "title":"Connections", "axis":{"labelFontSize":15, "titleFontSize":15, "labelAngle":360}},
		"y": {"aggregate":"mean", "field": "Gbps", "type": "quantitative",
			"scale": {"domain":{"signal":"extent_gbps"}}, "title":"Gbps", "axis":{"labelFontSize":15, "titleFontSize":15}},
		"color": {"field": "Scheme", "type": "nominal", "legend": {"labelFontSize":15, "titleFontSize":15}}
	}
}

