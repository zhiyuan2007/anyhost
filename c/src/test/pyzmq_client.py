import zmq
import qpsmessage_pb2

c = zmq.Context()
s = c.socket(zmq.REQ)
s.connect('tcp://127.0.0.1:5558')

def echo(msg1):
    s.send(msg1, copy=False)
    msg2 = s.recv(copy=False)
    return msg2

msgs2 = echo("qps")
print msgs2
msgs2 = echo("qps default")
print msgs2
msgs2 = echo("qps default 1.com")
print msgs2
msgs2 = echo("qps default 2.com")
print msgs2
