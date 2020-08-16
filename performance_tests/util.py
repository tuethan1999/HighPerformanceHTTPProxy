import requests
import difflib
import numpy as np
import logging
import collections 
import sys
import os
import statistics 
from threading import Thread
import time
import matplotlib.pyplot as plt
  

def setup_logger(name, log_file=None, console=False, level=logging.INFO):
	try: 
		# add the logger
		l = logging.getLogger(name)
		l.setLevel(level)
		if console:
			handlerConsole = logging.StreamHandler()
			handlerConsole.setLevel(logging.INFO)
			l.addHandler(handlerConsole)

	except Exception as e:
		logging.error(
			'line:{} type:{}, message:{}'.format(sys.exc_info()[-1].tb_lineno, type(e).__name__, e.message))
	finally:
		return l


def get_request(url, proxyDict=None):
	
	if proxyDict is None:
		r = requests.get(url, timeout=2)
	else:
		r = requests.get(url, proxies=proxyDict, timeout=2)

	if r.status_code != 200: 
		raise Exception('{} Response Code'.format(r.status_code))
	return r


def get_data_size(url):
	res = get_request(url)	
	return len(res.content)