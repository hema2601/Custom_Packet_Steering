var perf_graph = {
	"$schema": "https://vega.github.io/schema/vega-lite/v5.json",

	"data": {
		"name":"myData"
	},
	"width": 220,
	"height": 220,
	"transform": [
		{"fold": ["etc", "mm", "skb", "netdev", "data_copy", "tcp/ip", "sched", "lock"]}
	],
	"mark": "bar",
	"encoding": {
		"row": {"field":"Type", "type":"nominal"},
		"column": {"field": "Exp",  "type": "nominal"},
		"x": {"field": "key", "type": "nominal", "sort":[]},
		"xOffset":{"field":"Conns", "sort":[]},
		"y": {"aggregate":"mean", "field": "value", "type": "quantitative"},
		"color": {"field": "Conns", "type": "ordinal", "sort":[]}
	}
}
