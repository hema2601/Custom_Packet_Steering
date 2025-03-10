
var PktSteer = {
	"$schema": "https://vega.github.io/schema/vega-lite/v5.json",
	"repeat": [
		"ToBusy",
		"NotAvail",
		"PreviousStillBusy",
		"CalcTotal",
		"Overloaded",
		"CalcFromOverloaded",
		"ToOverloaded",
		"AllOverloaded"
	],
	"spec": {
		"data": {
			"name":"myData"
		},
		"transform": [
			{"filter": "datum.Total != 0"},
			{"filter": "datum.CPU >= 0 && datum.CPU <= 7"},
			{"calculate": "datum.AssignedToBusy / datum.Total ", "as": "ToBusy"},
			{"calculate": "datum.NoBusyAvailable / datum.Total ", "as": "NotAvail"},
			{
				"calculate": "(datum.Total - datum.PrevInvalid - datum.PrevIdle - datum.FromOverloaded) / datum.Total ",
				"as": "PreviousStillBusy"
			},
			{"calculate": "datum.FromOverloaded / datum.IsOverloaded  ", "as": "CalcFromOverloaded"},
			{"calculate": "datum.IsOverloaded - datum.FromOverloaded  ", "as": "ToOverloaded"},
			{"calculate": "datum.IsOverloaded / datum.Total  ", "as": "Overloaded"},
			{"calculate": "datum.ToBusy + datum.NotAvail + datum.PreviousStillBusy  ", "as": "CalcTotal"},
			{
				"calculate": "(datum.Exp == 'Custom1') ? 'IAPS+RFS':datum.Exp",
				"as": "Scheme"
			},
			{
				"calculate": "(datum.Scheme == 'Custom2') ? 'IAPS+LB':datum.Scheme",
				"as": "Scheme"
			}
		],
		"mark": {"type":"bar","tooltip":true},
		"encoding": {
			"x": {"field": "Conns", "type": "ordinal", "sort": []},
			"y": {
				"aggregate": "mean",
				"field": {"repeat": "repeat"},
				"type": "quantitative"
			},
			"color": {"field": "Scheme", "type": "nominal"},
			"row":{"field":"Exp"}
		}
	}
}
