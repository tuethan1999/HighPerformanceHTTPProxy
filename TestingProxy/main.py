import json
import os
from load import Load
from cdf import CDF
from util import setup_logger


with open ('config.json', 'r') as f:
	cfg = json.load(f)

proxy = '{}:{}'.format(cfg['proxyIp'], str(cfg['proxyPort']))

PROXYDICT = { 
			  "http"  : proxy, 
			  "https" : proxy
			  }

urls = cfg['urls']

setup_logger('', console=True)

load = Load(cfg['numClients'], cfg['reqsPerClient'], cfg['iterations'], PROXYDICT, urls)
load.run()

cdf = CDF(PROXYDICT, urls)
cdf.run(cfg['numRequests'])





