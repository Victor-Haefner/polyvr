import BaseHTTPServer as server
addr=('', 8100)

class handler(server.BaseHTTPRequestHandler):
  def do_HEAD(s):
    s.send_response(200)
    s.send_header("Content-type", "text/html")
    s.end_headers()
  def do_GET(s):
    handler.do_HEAD(s)
    s.wfile.write("hello ipg" + s.path)

if __name__ == '__main__':
  httpd = server.HTTPServer(addr, handler)
  httpd.serve_forever()
