var InputQData = {
 "$schema": "https://vega.github.io/schema/vega-lite/v5.json",
  "data": {
  "name":"myData"
   },
    "transform": [
	  {"filter":"datum['RPS Interrupts'] != 0"},
	    {"calculate":"datum['Input Pkts'] / datum['InputQ Dequeued']", "as":"InputQ Length"},
		  {"calculate":"datum['InputQ Dequeued'] / datum['RPS Interrupts']", "as":"Dequeue per IPI"},
		{"calculate": "(datum.Exp == 'Custom1') ? 'IAPS+RFS':datum.Exp", "as":"Scheme"},

		{"calculate": "(datum.Scheme == 'Custom2') ? 'IAPS+LB':datum.Scheme", "as":"Scheme"}

		    ],
			 "repeat":["InputQ Length", "Dequeue per IPI", "Dropped"],
			  "spec":{
			   "mark": {"type":"bar","tooltip":true},
			    "encoding": {
				   "x": {"field": "Conns", "type": "ordinal", "sort":[]},
				      "xOffset": {"field": "Scheme",},
					     "y": {"aggregate":"mean","field": {"repeat": "repeat"}, "type": "quantitative"},
						    "color": {"field": "Scheme", "type": "nominal"}
							 }
							  }
							  }

