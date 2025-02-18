var PpIPI = {
"$schema": "https://vega.github.io/schema/vega-lite/v5.json",
"data": {
  "name":"myData"
   },
   "width":220,
   "height":220,
   "transform": [
     {"filter":"datum.CPU >= 0 && datum.CPU <= 7"},
//	   {"filter":"datum.Exp == 'Custom1' || datum.Exp == 'RFS' || datum.Exp == 'RPS'"},
	     {"calculate": "(datum['RPS Interrupts'] != 0) ? datum.Processed / datum['RPS Interrupts'] : 0", "as": "Packets Per IPI"},
		   {"filter":"datum.Processed != 0"},
		     
		{"calculate": "(datum.Exp == 'Custom1') ? 'IAPS+RFS':datum.Exp", "as":"Scheme"},

		{"calculate": "(datum.Scheme == 'Custom2') ? 'IAPS+LB':datum.Scheme", "as":"Scheme"}
			   ],
			   "mark": {"type":"bar","tooltip":true},
			   "encoding": {
			     "x": {"field": "Conns", "type": "ordinal", "sort":[], "title":"Connections", "axis":{"labelFontSize":15, "titleFontSize":15, "labelAngle":360}},
				   "xOffset": {"field": "Scheme", "sort":"['RPS', 'RFS', 'IAPS']"},
				     "y": {"aggregate":"mean","field": "Packets Per IPI", "type": "quantitative", "title":"Packet Per IPI", "axis":{"labelFontSize":15, "titleFontSize":15}},
					   "color": {"field": "Scheme", "type": "nominal", "legend": {"labelFontSize":15, "titleFontSize":15}}
					   }
					   }

