import os
import sys
from tabulate import tabulate
import time

#Performance (e.g., latency of small transfers, throughput of large transfers); 


port = sys.argv[1]
proxy = "comp112-02.cs.tufts.edu" # CHANGE THIS AS NECESSARY
url0 = "http://portquiz.net:8080/" # Small web transfer - 50 bytes
url1 = "https://www.hq.nasa.gov/alsj/a17/A17_FlightPlan.pdf" # Large web transfer - 20 MB
url2 = "www.cs.tufts.edu/comp/160/"

noPTimeShort = 0 # no Proxy Time short transfer
pTimeShort = 0
noPTimeLong = 0
pTimeLong = 0
runs = 20
shortTransferIncrease = 0
longTransferIncrease = 0


def findLatency(curlString):
	time = 0
	for x in range(runs):
		# Print the total time into temp file noProxy
		os.system(curlString)
		# Read the time printed into the file
		with open("time") as file: 
		   time += float(file.read())

	# Average out the times
	return time / runs

def convertThroughputToString(throughput):

	if (throughput > 1000 and throughput < 1000000):
		t = int (throughput/1000) # Convert into a integer
		s = str(t) + "kbps"
	elif (throughput > 1000000):
		t = int(throughput/1000000)
		s = str(t) + "mbps"
	else:
		t = int (throughput)
		s = str(t) + "bps"
	return s

# BENCHMARK 1: Latency of small transfers via proxy and no proxy

def runSmallLatency():
	global noPTimeShort
	global pTimeShort
	global shortTransferIncrease

	# Find average latency of small web HTTP transfer without a proxy
	noPTimeShort = findLatency("curl -s -w '%{time_total}' -o /dev/null " + url0 + " > time")


	# Find average latency of small web HTTP transfer with a proxy
	pTimeShort = findLatency("curl -s -w '%{time_total}' -o /dev/null -x " + proxy + ":" + port + " " + url0 + " > time")


	# Calculate proxy % performance increase
	shortTransferIncrease = ((noPTimeShort - pTimeShort) / noPTimeShort) * 100


	print("--- Benchmark 1: Latency of Small Web HTTP Transfer (50 bytes) Completed ---\n")



# Benchmark 2: Latency of small transfers via proxy and no proxy


def runLargeLatency():
	global noPTimeLong
	global pTimeLong
	global longTransferIncrease

	noPTimeLong = findLatency("curl -s -w '%{time_total}' -o /dev/null " + url1 + " > time")
	pTimeLong = findLatency("curl -s -w '%{time_total}' -o /dev/null -x " + proxy + ":" + port + " " + url1 + " > time")

	# Calculate proxy % performance increase
	longTransferIncrease = ((noPTimeLong - pTimeLong) / noPTimeLong) * 100

	print("--- Benchmark 2: Latency of Large Web HTTPS Transfer (20M bytes) Completed ---\n")

def calcThroughputAndPrintResults():
	# Throughput = size (bits) / time (s)
	throughput1 = 50 * 8 / noPTimeShort
	throughput2 = 50 * 8 / pTimeShort
	throughput3 = 20000000 * 8 / noPTimeLong
	throughput4 = 20000000 * 8 / pTimeLong

	shortTPIncrease = ((noPTimeShort - pTimeShort) / noPTimeShort) * 100
	longTPIncrease = ((noPTimeLong - pTimeLong) / noPTimeLong)


	# Remove the temp file
	os.system("rm time")

	print(tabulate([['No Proxy', str(noPTimeShort) + "s", str(noPTimeLong) + "s", convertThroughputToString(throughput1), convertThroughputToString(throughput3)], 
					['Proxy', str(pTimeShort) + "s", str(pTimeLong) + "s", convertThroughputToString(throughput2), convertThroughputToString(throughput4)], 
					['Proxy % Increase', str(shortTransferIncrease)+"%", str(longTransferIncrease)+"%", str(shortTPIncrease)+"%", str(longTPIncrease)+"%"]], 

					headers=[' ', 'Avg Latency of Small Cached Transfer (50B)', 'Avg Latency of Large Uncached Transfer (20MB)', 'Throughput of Small Transfer', 'Throughput of Large Transfer']))


# Running latency benchmarks
print("Running benchmark tests for latency...")
print("Note : All results will be printed at the end\n")
runSmallLatency()
runLargeLatency()
calcThroughputAndPrintResults()
