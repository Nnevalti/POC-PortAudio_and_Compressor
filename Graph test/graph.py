import matplotlib.pyplot as plt
import json

# dictionary = json.load(open('graph_no_compression_lin.json', 'r'))
# dictionary = json.load(open('graph_compression_no_knee_lin.json', 'r'))
dictionary = json.load(open('graph_compression_with_knee_lin.json', 'r'))
xAxis = dictionary['x']
yAxis = dictionary['y']

# xAxis = [key for key, value in dictionary.items()]
# yAxis = [value for key, value in dictionary.items()]
plt.grid(True)

## LINE GRAPH ##
plt.plot(xAxis,yAxis, color='maroon', marker='o')
plt.xlabel('variable')
plt.ylabel('value')

# ## BAR GRAPH ##
# fig = plt.figure()
# plt.bar(xAxis,yAxis, color='maroon')
# plt.xlabel('variable')
# plt.ylabel('value')

plt.show()