var PktSteer = {
 "$schema": "https://vega.github.io/schema/vega-lite/v5.json",
  "repeat": [
     "ToBusy",
	    "NotAvail",
		   "PreviousStillBusy"
		    ],
			 "spec": {
			  "data": {
			  	"name":"myData"
			   },
			    "transform": [
				   {"filter":"datum.Total != 0"},
				      {"filter":"datum.CPU >= 0 && datum.CPU <= 7"},
					     {"filter":"datum.Exp == 'Custom1'"},
						    {"calculate": "datum.AssignedToBusy / datum.Total ", "as": "ToBusy"},
							   {"calculate": "datum.NoBusyAvailable / datum.Total ", "as": "NotAvail"},
							      {"calculate": "(datum.Total - datum.PrevInvalid - datum.PrevIdle) / datum.Total ", "as": "PreviousStillBusy"}

								    ],
									 "mark": "bar",
									  "encoding": {
									     "x": {"field": "Conns", "type": "ordinal", "sort":[]},
										    "xOffset": {"field": "Exp", "sort":["RSS", "RPS", "RFS", "RSS+RPS", "RSS+RFS"]},
											   "y": {"aggregate":"mean","field": {"repeat": "repeat"}, "type": "quantitative"},
											      "color": {"field": "Exp", "type": "nominal"}
												   }
												    }
													}

