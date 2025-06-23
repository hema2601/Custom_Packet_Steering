var backup_choice = {
		"$schema": "https://vega.github.io/schema/vega-lite/v5.json",
		"repeat": [
				"PB",
				"PO",
				"PI",
				"OB",
				"OO",
				"OI",
				"Busy",
				"Overloaded",
				"Idle"
		],
		"spec": {
				"data": {"name":"myData"},
				"transform": [
						{"filter": "datum.Total != 0"},
						{"filter": "datum.CPU >= 0 && datum.CPU <= 7"},
						{
								"calculate": "datum.NoBusyAvailable + datum.AllOverloaded",
								"as": "NotAvail"
						},
						{"calculate": "datum.BC_PB / datum.NotAvail  ", "as": "PB"},
						{"calculate": "datum.BC_PO / datum.NotAvail  ", "as": "PO"},
						{"calculate": "datum.BC_PI / datum.NotAvail  ", "as": "PI"},
						{"calculate": "datum.BC_OB / datum.NotAvail  ", "as": "OB"},
						{"calculate": "datum.BC_OO / datum.NotAvail  ", "as": "OO"},
						{"calculate": "datum.BC_OI / datum.NotAvail  ", "as": "OI"},
						{"calculate": "(datum.BC_OI + datum.BC_PI) / datum.NotAvail  ", "as": "Idle"},
						{"calculate": "(datum.BC_OB + datum.BC_PB) / datum.NotAvail  ", "as": "Busy"},
						{"calculate": "(datum.BC_OO + datum.BC_PO) / datum.NotAvail  ", "as": "Overloaded"},
				],
				"mark": {"type": "bar", "tooltip": true},
				"encoding": {
						"x": {"field": "Conns", "type": "ordinal", "sort": []},
						"y": {
								"aggregate": "mean",
								"field": {"repeat": "repeat"},
								"type": "quantitative"
						},
						"color": {"field": "Exp", "type": "nominal"},
						"row": {"field": "Exp"}
				}
		}
}
