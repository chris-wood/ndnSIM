import csv
from pylab import *

rc('text', usetex=True)
rc('font', family='serif')

# This is 20 seconds
xaxis_mul = 20;
step = 1000000000.0 * xaxis_mul

file_names = ["dfn-pint-generation-overhead-latency-Cr80-NOPINT-NOCACHE",
              "dfn-pint-generation-overhead-latency-Cr80-NOPINTENCR",
              "dfn-pint-generation-overhead-latency-Cr160-NOPINT-NOCACHE",
              "dfn-pint-generation-overhead-latency-Cr160-NOPINTENCR"
              "dfn-pint-generation-overhead-latency-Cr320-NOPINT-NOCACHE",
              "dfn-pint-generation-overhead-latency-Cr320-NOPINTENCR"
              "dfn-pint-generation-overhead-latency-Cr640-NOPINT-NOCACHE",
              "dfn-pint-generation-overhead-latency-Cr640-NOPINTENCR"]

color = ['r', 'r', 'g', 'g', 'b', 'b', 'k', 'k']
style = ['-', '--', '-', '--', '-', '--', '-', '--']

i = 0
for name in file_names:
    print "Processing '" + name + "'..."

    xaxis = []
    yaxis = []

    with open(name, 'r') as f:
        reader=csv.reader(f, delimiter='\t')
        count = 0
        sum = 0.0
        tick = 0.0
        for ts, delay in reader:
            if float(ts) > tick:
                xaxis.append((tick / step) * xaxis_mul)
                if count == 0:
                    yaxis.append(0)
                else:
                    yaxis.append((sum / count) / 1000000)

                    sum = 0
                    count = 0
                    tick = tick + step
            
            sum = sum + float(delay)
            count = count + 1

    plot(xaxis, yaxis, linestyle=style[i], color=color[i])
    hold(True)
    i = i + 1

xlabel('Simulation Time (sec)')
ylabel('Routers Pint Generation and Forwarding Delay (msec)')
legend([r"No {\em pInt}, $Cr = 80$",
        r"{\em pInt}, $Cr = 80$",
        r"No {\em pInt}, $Cr = 160$",
        r"{\em pInt}, $Cr = 160$",
        r"No {\em pInt}, $Cr = 320$",
        r"{\em pInt}, $Cr = 320$",
        r"No {\em pInt}, $Cr = 640$",
        r"{\em pInt}, $Cr = 640$",
        r"No {\em pInt}, $Cr = 1280$",
        r"{\em pInt}, $Cr = 1280$"], loc=2)
grid(True)
show()
