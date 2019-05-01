import requests
import difflib
import numpy as np
import logging
import collections 
import sys
import os
import statistics 
import uuid
import matplotlib.pyplot as plt
import time
from util import get_request, get_data_size
from threading import Thread
import time

workerData = collections.namedtuple('workerData',['totalTime', 'numReqs', 'numErrors'])
compounded = collections.namedtuple('compounded',['numWorkers', 'totalReqs', 'avgWorkerTime', 'totalErrors'])

class Load(object):

	def __init__(self, baseClients, baseRequests, iterations, proxyDict, urls):

		self.baseClients = baseClients
		self.baseRequests = baseRequests
		self.iterations = iterations
		self.proxyDict = proxyDict
		self.urls = urls
		self.workers = []
		self.clientPoints = []
		self.reqPoints = []
		self.waitTime = 3

	def run(self):
		for url in self.urls:

			kbs = int(get_data_size(url) / 1000)

			logging.info('Iterating on num clients, fixed reqs per client'.upper())
			fig = plt.figure() 
			dps = self.run_iterations(url, True)
			title = 'GET: {}\nData Size: {} kbs; Fixed at {} Requests Per Client'.format(url, kbs, self.baseRequests)
			self.generate_plot(title, dps, iterClients=True)
			fig.savefig('{}.png'.format(uuid.uuid4()), dpi=300)

			logging.info('Iterating on reqs/client'.upper())
			fig = plt.figure() 
			dps = self.run_iterations(url, False)
			title = 'GET: {}\nData Size: {} kbs; Fixed at {} Clients'.format(url, kbs, self.baseClients)
			self.generate_plot(title, dps, iterReqs=True)
			fig.savefig('{}.png'.format(uuid.uuid4()), dpi=300)
		
			logging.info('Iterating on Fixed Requests'.upper())
			fixedReqs = self.baseRequests * self.baseClients * self.iterations
			fig = plt.figure() 
			dps = self.run_iterations(url, True, fixedReqs)
			title = 'GET: {}\nData Size: {} kbs; Fixed at ~{} Requests'.format(url, kbs, fixedReqs)
			self.generate_plot(title, dps, fixedReqs=True)
			fig.savefig('{}.png'.format(uuid.uuid4()), dpi=300)



	def run_iterations(self, url, iterateClients, fixedRequests=None):

		data_points = []
		numClients = self.baseClients
		reqsPerClient = self.baseRequests if fixedRequests is None else fixedRequests / numClients

		for i in range(self.iterations):
			self.run_concurrent(numClients, reqsPerClient, url)
			time.sleep(self.waitTime) # To avoid 429s
			
			# All workers done
			data_points.append(self.get_data_point(numClients, reqsPerClient))
			
			if iterateClients: 
				numClients += self.baseClients
				if fixedRequests is not None:
					reqsPerClient = fixedRequests / numClients

			else: 
				reqsPerClient += self.baseRequests

		return data_points




	def generate_plot(self, title, data_points, iterClients=False, iterReqs=False, fixedReqs=False):


		rpsCalc = lambda reqs, workers, avgTime: round(reqs / float(workers * avgTime), 2)
		
		ys = [rpsCalc(p.totalReqs, p.numWorkers, p.avgWorkerTime) for p in data_points]
		plt.ylabel('Requests / Second', fontsize=10)

		if iterClients: 
			xs = [p.totalReqs for p in data_points]
			plt.xlabel('Total Requests', fontsize=10)
			bs = [p.numWorkers for p in data_points]
			cs = [round(p.avgWorkerTime, 2) for p in data_points]
			plt.plot(xs, ys, 'ro', label='# Clients, Client Perceived Time (s)')

		elif iterReqs:
			xs = [p.totalReqs for p in data_points]
			plt.xlabel('Total Requests', fontsize=10)
			bs = [p.totalReqs / p.numWorkers for p in data_points]
			cs = [round(p.avgWorkerTime, 2) for p in data_points]
			plt.plot(xs, ys, 'ro', label='Requests per Client, Client Perceived Time (s)')

		elif fixedReqs:
			xs = [p.numWorkers for p in data_points]
			plt.xlabel('Number of Clients', fontsize=10)
			bs = [p.totalReqs / p.numWorkers for p in data_points]
			cs = [round(p.avgWorkerTime, 2) for p in data_points]
			plt.plot(xs, ys, 'ro', label='Requests per Client, Client Perceived Time (s)')
		
		


		
		plt.title(title, fontsize=10)	
		plt.legend(loc='upper right', fontsize=7)
		for i in range(len(xs)):
   			plt.annotate('   {}, {}'.format(bs[i], cs[i]), (xs[i], ys[i]), fontsize=5)



	#compunded = collections.namedtuple('compounded',['numWorkers', 'totalReqs', 'avgWorkerTime', 'totalErrors'])
	def get_data_point(self, numClients, numReqs):
			
		avgWorkerTime = statistics.mean([x.totalTime for x in self.workers])
		totalErrors = int(sum([x.numErrors for x in self.workers]))
		totalReqs = numReqs * numClients
		self.workers = []
		return compounded(numClients, totalReqs, avgWorkerTime, totalErrors)


	def run_concurrent(self, clients, reqPerClient, url):

		logging.info('Starting {} Clients, each with {} Requests to {}'.format(clients, reqPerClient, url))
		threads = []
		for i in range(clients):
			t = Thread(target=self.worker, args=(reqPerClient, url))
			t.daemon = True
			t.start()
			threads.append(t)				
		for t in threads:
			t.join()


	def worker(self, nReqs, url):
		errors = 0
		start = time.time()
		for i in range(nReqs):
			try:
				res = get_request(url, self.proxyDict)
			except Exception as e:
				errors += 1
		elapsed = time.time() - start
		self.workers.append(workerData(elapsed, nReqs, errors))



			
