import csv
from pylab import *

rc('text', usetex=True)
rc('font', family='serif')

# This is 20 seconds
xaxis_mul = 20;
step = 1000000000.0 * xaxis_mul

file_names = ["att-pint-generation-overhead-delay-Cr160-NOPINT-CACHE",
              "att-pint-generation-overhead-delay-Cr160-PINT-CACHE",
              "att-pint-generation-overhead-delay-Cr320-NOPINT-CACHE",
              "att-pint-generation-overhead-delay-Cr320-PINT-CACHE",
              "att-pint-generation-overhead-delay-Cr640-NOPINT-CACHE",
              "att-pint-generation-overhead-delay-Cr640-PINT-CACHE",
              "att-pint-generation-overhead-delay-Cr1280-NOPINT-CACHE",
              "att-pint-generation-overhead-delay-Cr1280-PINT-CACHE"]

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
legend([r"No {\em pInt}, $Cr = 160$",
        "{\em pInt}, $Cr = 160$",
        "No {\em pInt}, $Cr = 320$",
        "{\em pInt}, $Cr = 320$",
        "No {\em pInt}, $Cr = 640$",
        "{\em pInt}, $Cr = 640$",
        "No {\em pInt}, $Cr = 1280$",
        "{\em pInt}, $Cr = 1280$"], loc=2)
grid(True)
show()
