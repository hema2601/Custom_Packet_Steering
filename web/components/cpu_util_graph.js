var CPUUtil = {
	"$schema": "https://vega.github.io/schema/vega-lite/v5.json",
	"description": "A simple bar chart with embedded data.",
	"data": {
		"name":"myData"

	},
	"width":820,
	"height":220,
	"transform": [
		{"filter":"datum.CPU >= 0 && datum.CPU <= 7"},
		{"calculate":"datum.User + datum.Nice + datum.System + datum.Idle + datum.IOWait + datum.Irq + datum.Softirq + datum.Steal + datum.Guest + datum.Guest_Nice", "as":"Total"},
		{"filter":"datum.Total < 40000"},
		{"calculate":"datum.User / datum.Total", "as":"UP"},
		{"calculate":"datum.System / datum.Total", "as":"SyP"},
		{"calculate":"datum.Softirq / datum.Total", "as":"SoP"},
		{"calculate":"datum.Idle / datum.Total", "as":"IP"},
		{"calculate":"(datum.Nice+ datum.IOWait + datum.Irq+ datum.Steal + datum.Guest + datum.Guest_Nice) / datum.Total", "as":"Others"},
		{"fold": ["UP", "SyP", "SoP", "IP", "Others"]}
	],
	"mark": "bar",
	"encoding": {
		"column": {"field": "Exp",  "type": "nominal"},
		"row":{"field":"Rep"},
		"x": {"field": "Conns", "type": "nominal", "sort":[]},
		"xOffset":{"field":"CPU"},
		"y": {"aggregate":"mean","field": "value", "type": "quantitative"},
		"color": {"field": "key", "type": "nominal"}
	}


}

