var InputQData = {
 "$schema": "https://vega.github.io/schema/vega-lite/v5.json",
  "data": {
  "name":"myData"
   },
    "transform": [
	  {"filter":"datum['RPS Interrupts'] != 0"},
	    {"calculate":"datum['Input Pkts'] / datum['InputQ Dequeued']", "as":"InputQ Length"},
		  {"calculate":"datum['InputQ Dequeued'] / datum['RPS Interrupts']", "as":"Dequeue per IPI"}

		    ],
			 "repeat":["InputQ Length", "Dequeue per IPI", "Dropped"],
			  "spec":{
			   "mark": "bar",
			    "encoding": {
				   "x": {"field": "Conns", "type": "ordinal", "sort":[]},
				      "xOffset": {"field": "Exp",},
					     "y": {"aggregate":"mean","field": {"repeat": "repeat"}, "type": "quantitative"},
						    "color": {"field": "Exp", "type": "nominal"}
							 }
							  }
							  }

