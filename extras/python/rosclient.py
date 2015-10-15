#!/usr/bin/python
"""
This script starts a web server that accepts any data string via post:
	import VR, httplib, urllib
	params = urllib.urlencode({'data' : 'myData'})
	headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}
	conn = httplib.HTTPConnection("localhost:8000")
	conn.request("POST", "", params, headers)

	The data is then published on an ros network
"""

import os, sys

def initNonBash():
    p1 = '/opt/ros/jade/lib/python2.7/dist-packages'
    sys.path.insert(0, p1)
    sys.argv = []

    os.environ['PYTHONPATH'] = p1
    os.environ['ROS_ROOT'] = '/opt/ros/jade/share/ros'
    os.environ['ROS_DISTRO'] = 'jade'
    os.environ['ROSLISP_PACKAGE_DIRECTORIES']=''
    os.environ['CMAKE_PREFIX_PATH'] = '/opt/ros/jade'
    os.environ['PKG_CONFIG_PATH'] = '/opt/ros/jade/lib/pkgconfig:/opt/ros/jade/lib/x86_64-linux-gnu/pkgconfig'
    os.environ['CPATH'] = '/opt/ros/jade/include'
    os.environ['LD_LIBRARY_PATH']='/opt/ros/jade/lib:/opt/ros/jade/lib/x86_64-linux-gnu:/usr/lib32'
    os.environ['ROS_ETC_DIR']='/opt/ros/jade/etc/ros'
    os.environ['ROS_PACKAGE_PATH']='/opt/ros/jade/share:/opt/ros/jade/stacks'
    os.environ['ROS_MASTER_URI']='http://localhost:11311'

#initNonBash()
import rospy, std_msgs
from sensor_msgs.msg import JointState

def rosCallback(data):
	print data
	return

# last step, init ros node
rospy.init_node('polyvr')
#rospy.Subscriber('/svh/joint_states', JointState, rosCallback)
pub = rospy.Publisher('/svh/polyvr_data', std_msgs.msg.String, queue_size = 10)
pub.publish(std_msgs.msg.String("foo"))

# webserver
from BaseHTTPServer import *
import urllib

class HttpHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        content_len = int(self.headers.getheader('content-length', 0))
        post_body = urllib.unquote_plus( self.rfile.read(content_len) )
        print post_body
	pub.publish(std_msgs.msg.String(post_body))

server = HTTPServer( ('127.0.0.1', 8000), HttpHandler)
server.serve_forever()
