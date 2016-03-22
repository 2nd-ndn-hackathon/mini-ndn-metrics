#!/usr/bin/env python

import tornado.httpserver
import tornado.websocket
import tornado.ioloop
from tornado.ioloop import PeriodicCallback
import tornado.web
from random import randint #Random generator

from sys import argv

#Config
port = 9000 #Websocket Port
timeInterval = 2000 #Milliseconds
fileNameLinks = "links.txt"
fileNameStat = "stat"

class WSHandler(tornado.websocket.WebSocketHandler):

	#check_origin fixes an error 403 with Tornado
	#http://stackoverflow.com/questions/24851207/tornado-403-get-warning-when-opening-websocket
	def check_origin(self, origin):
		return True

	def open(self):
		self.sendNames()

	def sendNames(self):
		with open(fileNameLinks) as f:
			content = f.readlines()
			msg = ''
			for line in content:
				links = line.split(' ')
				if len(links) == 4 :
					linkId = links[0]
					name = links[1] + "->" + links[3]
					if "stat" not in name :
						msg += 'name:' + linkId + ':' + name.rstrip() + ';'
			f.close()
			#print(msg)
			self.write_message(msg)


			#Send message periodic via socket upon a time interval
			self.callback = PeriodicCallback(self.send_values, timeInterval)
			self.callback.start()


	def send_values(self):
		with open(fileNameStat) as f:
			content = f.readlines()
			msg = ''
			for line in content:
				stats = line.split('-')
				if len(stats) == 4 :
					for el in stats:
						e = el.split(':')
						if e[0] == "LI" :
							linkId = e[1]
						elif e[0] == "TX" :
							bytesOut = e[1]
						elif e[0] == "RX" :
							bytesIn = e[1]

					msg += 'id:' + linkId + ':' + bytesOut + ':' + bytesIn.rstrip() + ';'
			f.close()
			#print(msg)
			self.write_message(msg)

	def on_message(self, message):
		pass

	def on_close(self):
		self.callback.stop()

application = tornado.web.Application([
	(r'/', WSHandler),
])

if __name__ == "__main__":
	http_server = tornado.httpserver.HTTPServer(application)
	http_server.listen(port)
	tornado.ioloop.IOLoop.instance().start()