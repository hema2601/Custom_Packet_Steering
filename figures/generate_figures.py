import altair as alt
import json

with open('../data/Thesis_Merged/summaries/summary_softnet.json') as f:
    data = json.load(f)
    data_dict = {"values":data}
    

with open('vis/thesis_2025/Figure3.json') as f:
    spec = json.load(f)
    spec["data"]=data_dict


chart = alt.Chart.from_dict(spec)

chart.save('chart.png')
