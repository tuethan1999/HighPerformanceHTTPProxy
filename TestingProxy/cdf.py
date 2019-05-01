import numpy as np
import logging
import sys
import os 
import uuid
import matplotlib.pyplot as plt
from util import get_request, get_data_size
import time

class CDF(object):

	def __init__(self, proxyDict, urls):

		self.proxyDict = proxyDict
		self.urls = urls


	def run(self, numPoints):

		for url in self.urls:
			kbs = int(get_data_size(url) / 1000)
			
			pdata, pErrors = self.get_data(numPoints, url, True)
			npdata, npErrors = self.get_data(numPoints, url, False)

			title = 'GET: {}\nData Size: {} kbs'.format(url, kbs)
			plabel = 'Proxy, n={}, errors={}'.format(len(pdata), pErrors)
			nplabel = 'Non-Proxy, n={}, errors={}'.format(len(npdata), npErrors)

			fig = plt.figure()
			self.plot_data(pdata, plabel)
			self.plot_data(npdata, nplabel)
			plt.legend(loc='best', fontsize=10)
			plt.title(title, fontsize=10)
			plt.xlabel('RTT (s)', fontsize=10)
			plt.ylabel('CDF', fontsize=10)
			fig.savefig('{}.png'.format(uuid.uuid4()), dpi=300)



	def get_data(self, numPoints, url, useProxy):
		
		logging.info('Starting {} Requests to {}. Proxy? {}'.format(numPoints, url, useProxy))
		rtts = []
		errors = 0
		for i in range(numPoints):
			try:
				if useProxy: res = get_request(url, self.proxyDict)
				else: res = get_request(url)
				rtts.append(res.elapsed.total_seconds())
				if i % 10 == 0: time.sleep(5) # avoid 429s
			except Exception as e:
				errors += 1

		logging.info('Finished {} Requests to {}. Got {} errors'.format(numPoints, url, errors))		
		return (rtts, errors)


	def plot_data(self, data, label):
		binNum = max(10, len(data) / 10)
		# Use the histogram function to bin the data
		counts, bin_edges = np.histogram(data, bins=binNum)
		# Now find the cdf
		cdf = np.cumsum(counts)
		cdf = np.true_divide(cdf, len(data))
		# And finally plot the cdf
		plt.plot(bin_edges[1:], cdf, label=label)

